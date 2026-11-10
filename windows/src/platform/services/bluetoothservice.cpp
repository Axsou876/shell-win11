#include "bluetoothservice.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>

#include <winrt/Windows.Devices.Radios.h>
#include <winrt/Windows.Foundation.Collections.h>

#include <chrono>
#include <vector>

using namespace winrt::Windows::Devices::Radios;

BluetoothService::BluetoothService(QObject* parent)
    : QObject(parent)
{
    m_started = true;
    m_worker = std::thread([this]() { run(); });

    if (QCoreApplication::instance())
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this,
            &BluetoothService::stop);
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

void BluetoothService::setEnabled(bool enabled)
{
    m_requestedState = enabled ? 1 : 0;
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

        {
            std::unique_lock lock(m_mutex);
            while (!m_stop.load()) {
                m_cv.wait_for(lock, std::chrono::milliseconds(200));
                const int requested = m_requestedState.exchange(-1);
                if (requested < 0 || bluetoothRadios.empty())
                    continue;

                try {
                    const RadioState target = requested == 1 ? RadioState::On : RadioState::Off;
                    for (const auto& radio : bluetoothRadios) {
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
                        this, [this, message]() { emit toggleFailed(message); },
                        Qt::QueuedConnection);
                }
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
