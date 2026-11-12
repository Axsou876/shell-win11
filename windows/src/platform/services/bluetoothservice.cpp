#include "bluetoothservice.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>
#include <QStringList>
#include <QVariantMap>

#include <winrt/Windows.Devices.Radios.h>
#include <winrt/Windows.Foundation.Collections.h>

#include <windows.h>
#include <bluetoothapis.h>

#include <chrono>
#include <string>
#include <utility>
#include <vector>

using namespace winrt::Windows::Devices::Radios;

namespace {
constexpr int kInquiryTimeoutMultiplier = 4; // 4 * 1.28 s ≈ 5 s
constexpr int kIdlePollMs = 200;

QString formatAddress(const BLUETOOTH_ADDRESS& address)
{
    // bthprops stores the address little-endian, but users and Windows itself
    // print it most-significant byte first.
    return QStringLiteral("%1:%2:%3:%4:%5:%6")
        .arg(address.rgBytes[5], 2, 16, QLatin1Char('0'))
        .arg(address.rgBytes[4], 2, 16, QLatin1Char('0'))
        .arg(address.rgBytes[3], 2, 16, QLatin1Char('0'))
        .arg(address.rgBytes[2], 2, 16, QLatin1Char('0'))
        .arg(address.rgBytes[1], 2, 16, QLatin1Char('0'))
        .arg(address.rgBytes[0], 2, 16, QLatin1Char('0'))
        .toUpper();
}

BLUETOOTH_ADDRESS parseAddress(const QString& text)
{
    BLUETOOTH_ADDRESS address {};
    const QStringList parts = text.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    if (parts.size() != 6)
        return address;

    for (int index = 0; index < 6; ++index) {
        bool ok = false;
        const uint value = parts.at(index).toUInt(&ok, 16);
        if (!ok || value > 0xFF)
            return BLUETOOTH_ADDRESS {};
        address.rgBytes[5 - index] = static_cast<BYTE>(value);
    }
    return address;
}

QString win32Message(DWORD error)
{
    wchar_t* buffer = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, 0, reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);

    QString message;
    if (length && buffer)
        message = QString::fromWCharArray(buffer, static_cast<int>(length)).trimmed();
    if (buffer)
        LocalFree(buffer);

    if (message.isEmpty())
        message = QStringLiteral("Erreur %1").arg(error);
    return message;
}

// Owns the radio handle and the find handle for one command.
struct RadioFinder {
    HANDLE radio = nullptr;
    HBLUETOOTH_RADIO_FIND find = nullptr;

    RadioFinder()
    {
        BLUETOOTH_FIND_RADIO_PARAMS params {};
        params.dwSize = sizeof(params);
        find = BluetoothFindFirstRadio(&params, &radio);
    }

    ~RadioFinder()
    {
        if (find)
            BluetoothFindRadioClose(find);
        if (radio)
            CloseHandle(radio);
    }

    RadioFinder(const RadioFinder&) = delete;
    RadioFinder& operator=(const RadioFinder&) = delete;

    [[nodiscard]] bool valid() const
    {
        return radio != nullptr;
    }
};

QVariantList collectDevices(HANDLE radio, bool inquire)
{
    QVariantList result;
    if (!radio)
        return result;

    BLUETOOTH_DEVICE_SEARCH_PARAMS params {};
    params.dwSize = sizeof(params);
    // Remembered devices are the ones the user paired before; an inquiry adds
    // discoverable, unpaired devices so they can be paired from the shell.
    params.fReturnAuthenticated = TRUE;
    params.fReturnRemembered = TRUE;
    params.fReturnConnected = TRUE;
    params.fReturnUnknown = inquire ? TRUE : FALSE;
    params.fIssueInquiry = inquire ? TRUE : FALSE;
    params.cTimeoutMultiplier = kInquiryTimeoutMultiplier;
    params.hRadio = radio;

    BLUETOOTH_DEVICE_INFO info {};
    info.dwSize = sizeof(info);

    HBLUETOOTH_DEVICE_FIND find = BluetoothFindFirstDevice(&params, &info);
    if (!find)
        return result;

    do {
        QString name = QString::fromWCharArray(info.szName);
        if (name.isEmpty())
            name = QStringLiteral("Appareil Bluetooth");

        QVariantMap entry;
        entry.insert(QStringLiteral("address"), formatAddress(info.Address));
        entry.insert(QStringLiteral("name"), name);
        entry.insert(QStringLiteral("connected"), info.fConnected != FALSE);
        entry.insert(QStringLiteral("paired"),
            info.fRemembered != FALSE && info.fAuthenticated != FALSE);
        result.append(entry);
    } while (BluetoothFindNextDevice(find, &info));

    BluetoothFindDeviceClose(find);
    return result;
}
} // namespace

BluetoothService::BluetoothService(QObject* parent)
    : QObject(parent)
{
    m_started = true;
    m_worker = std::thread([this]() { run(); });

    if (QCoreApplication::instance())
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this,
            &BluetoothService::stop);

    refreshDevices();
}

BluetoothService::~BluetoothService()
{
    stop();
}

bool BluetoothService::present() const
{
    return m_present;
}

bool BluetoothService::enabled() const
{
    return m_enabled;
}

bool BluetoothService::known() const
{
    return m_known;
}

QVariantList BluetoothService::devices() const
{
    return m_devices;
}

bool BluetoothService::scanning() const
{
    return m_scanning;
}

void BluetoothService::setEnabled(bool enabled)
{
    post(Command { CommandType::SetRadio, enabled, QString() });
}

void BluetoothService::refreshDevices()
{
    post(Command { CommandType::Refresh, false, QString() });
}

void BluetoothService::scanDevices()
{
    post(Command { CommandType::Scan, false, QString() });
}

void BluetoothService::pairDevice(const QString& address)
{
    post(Command { CommandType::Pair, false, address });
}

void BluetoothService::unpairDevice(const QString& address)
{
    post(Command { CommandType::Unpair, false, address });
}

void BluetoothService::post(Command command)
{
    if (!m_started.load())
        return;

    {
        std::lock_guard lock(m_mutex);
        m_commands.push_back(std::move(command));
    }
    m_cv.notify_all();
}

void BluetoothService::stop()
{
    if (!m_started.exchange(false))
        return;

    m_stop = true;
    m_cv.notify_all();
    if (m_worker.joinable())
        m_worker.join();
}

void BluetoothService::run()
{
    bool apartmentInitialized = false;

    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        apartmentInitialized = true;

        const auto radios = Radio::GetRadiosAsync().get();
        std::vector<Radio> bluetoothRadios;
        std::vector<winrt::event_token> tokens;

        for (const auto& radio : radios) {
            if (radio.Kind() == RadioKind::Bluetooth)
                bluetoothRadios.push_back(radio);
        }

        if (bluetoothRadios.empty()) {
            QMetaObject::invokeMethod(
                this, [this]() { setState(false, false, true); }, Qt::QueuedConnection);
        } else {
            bool enabled = false;
            bool known = true;
            for (const auto& radio : bluetoothRadios) {
                const auto state = radio.State();
                if (state == RadioState::On)
                    enabled = true;
                if (state == RadioState::Unknown)
                    known = false;
            }
            QMetaObject::invokeMethod(
                this, [this, enabled, known]() { setState(true, enabled, known); },
                Qt::QueuedConnection);

            for (const auto& radio : bluetoothRadios) {
                tokens.push_back(radio.StateChanged(
                    [this, radio](const Radio&, const winrt::Windows::Foundation::IInspectable&) {
                        if (m_stop.load())
                            return;

                        const auto state = radio.State();
                        const bool isEnabled = state == RadioState::On;
                        const bool isKnown = state != RadioState::Unknown;
                        QMetaObject::invokeMethod(
                            this,
                            [this, isEnabled, isKnown]() {
                                setState(true, isEnabled, isKnown);
                            },
                            Qt::QueuedConnection);
                    }));
            }
        }

        while (!m_stop.load()) {
            std::deque<Command> pending;
            {
                std::unique_lock lock(m_mutex);
                m_cv.wait_for(lock, std::chrono::milliseconds(kIdlePollMs), [this]() {
                    return m_stop.load() || !m_commands.empty();
                });
                pending.swap(m_commands);
            }

            if (m_stop.load())
                break;

            for (const Command& command : pending) {
                if (m_stop.load())
                    break;
                handleCommand(command);
            }
        }

        for (size_t i = 0; i < bluetoothRadios.size(); ++i)
            bluetoothRadios[i].StateChanged(tokens[i]);
    } catch (const winrt::hresult_error& error) {
        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_STATUS")) {
            qWarning().noquote() << "Bluetooth WinRT error:"
                                 << QString::fromWCharArray(error.message().c_str());
        }
        QMetaObject::invokeMethod(
            this, [this]() { setState(false, false, false); }, Qt::QueuedConnection);
    }

    if (apartmentInitialized)
        winrt::uninit_apartment();
}

void BluetoothService::handleCommand(const Command& command)
{
    switch (command.type) {
    case CommandType::SetRadio: {
        // Radio::GetRadiosAsync() again: the worker owns no cached Radio list
        // beyond the one used for state notifications.
        try {
            const auto radios = Radio::GetRadiosAsync().get();
            const RadioState target = command.enabled ? RadioState::On : RadioState::Off;
            for (const auto& radio : radios) {
                if (radio.Kind() != RadioKind::Bluetooth)
                    continue;
                const RadioAccessStatus status = radio.SetStateAsync(target).get();
                if (status != RadioAccessStatus::Allowed) {
                    QMetaObject::invokeMethod(
                        this,
                        [this]() {
                            emit toggleFailed(QStringLiteral(
                                "Windows a refusé la modification du Bluetooth"));
                        },
                        Qt::QueuedConnection);
                }
            }
        } catch (const winrt::hresult_error& error) {
            const QString message = QString::fromWCharArray(error.message().c_str());
            QMetaObject::invokeMethod(
                this, [this, message]() { emit toggleFailed(message); }, Qt::QueuedConnection);
        }
        break;
    }
    case CommandType::Refresh: {
        RadioFinder finder;
        publishDevices(collectDevices(finder.radio, false), false);
        break;
    }
    case CommandType::Scan: {
        publishDevices(QVariantList(), true);
        RadioFinder finder;
        publishDevices(collectDevices(finder.radio, true), false);
        break;
    }
    case CommandType::Pair: {
        if (command.address.isEmpty())
            break;

        RadioFinder finder;
        BLUETOOTH_DEVICE_INFO info {};
        info.dwSize = sizeof(info);
        info.Address = parseAddress(command.address);

        // Windows shows its own Secure Simple Pairing UI when a passkey is not
        // supplied; devices that still require a fixed PIN fail here with a
        // readable message instead of hanging. General bonding without a
        // mandatory MITM step keeps headsets and mice (no keypad, no display)
        // pairable; keyboards can still authenticate through the Windows UI.
        // The parameter type is MSCMAPI's AUTHENTICATION_REQUIREMENTS from
        // bthdef.h, not BLUETOOTH_AUTHENTICATION_REQUIREMENTS from
        // bluetoothapis.h: both enums exist and are not interchangeable.
        const DWORD result = BluetoothAuthenticateDeviceEx(nullptr, finder.radio, &info, nullptr,
            MITMProtectionNotRequiredGeneralBonding);

        if (result != ERROR_SUCCESS) {
            const QString message = QStringLiteral("Appairage impossible : %1")
                                        .arg(win32Message(result));
            QMetaObject::invokeMethod(
                this, [this, message]() { emit toggleFailed(message); }, Qt::QueuedConnection);
        }

        publishDevices(collectDevices(finder.radio, false), false);
        break;
    }
    case CommandType::Unpair: {
        if (command.address.isEmpty())
            break;

        RadioFinder finder;
        BLUETOOTH_ADDRESS address = parseAddress(command.address);
        const DWORD result = BluetoothRemoveDevice(&address);
        if (result != ERROR_SUCCESS) {
            const QString message = QStringLiteral("Dissociation impossible : %1")
                                        .arg(win32Message(result));
            QMetaObject::invokeMethod(
                this, [this, message]() { emit toggleFailed(message); }, Qt::QueuedConnection);
        }

        publishDevices(collectDevices(finder.radio, false), false);
        break;
    }
    }
}

void BluetoothService::setState(bool present, bool enabled, bool known)
{
    if (m_present == present && m_enabled == enabled && m_known == known)
        return;

    m_present = present;
    m_enabled = enabled;
    m_known = known;
    emit changed();

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_STATUS")) {
        qInfo().noquote() << "Bluetooth: present=" << present << "enabled=" << enabled
                          << "known=" << known;
    }
}

void BluetoothService::publishDevices(const QVariantList& devices, bool scanning)
{
    QMetaObject::invokeMethod(
        this,
        [this, devices, scanning]() {
            const bool changed = m_devices != devices || m_scanning != scanning;

            m_devices = devices;
            m_scanning = scanning;
            if (changed)
                emit devicesChanged();

            // Logged on every publication, not only on change: with no paired
            // device the list is empty both before and after a successful
            // enumeration, and silence would not tell the two apart.
            if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_STATUS")) {
                qInfo().noquote() << "Bluetooth devices:" << m_devices.size()
                                  << "scanning=" << scanning;
            }
        },
        Qt::QueuedConnection);
}
