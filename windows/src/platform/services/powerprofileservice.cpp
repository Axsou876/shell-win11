#include "powerprofileservice.hpp"

#include <QDebug>
#include <QSettings>
#include <QTimer>
#include <QUuid>
#include <QVariantMap>

#include <windows.h>
#include <powersetting.h>
#include <powrprof.h>

namespace {
// The three overlay ids documented for PowerSetActiveOverlayScheme. "Balanced"
// is the all-zero GUID, which is also what the registry holds when the user
// never picked another overlay: both cases mean the same thing.
constexpr GUID kOverlayPowerSaver { 0x961cc777, 0x2547, 0x4f9d,
    { 0x81, 0x74, 0x7d, 0x86, 0x18, 0x1b, 0x8a, 0x7a } };
constexpr GUID kOverlayBalanced { 0x00000000, 0x0000, 0x0000,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
constexpr GUID kOverlayPerformance { 0xded574b5, 0x45a0, 0x4f42,
    { 0x87, 0x37, 0x46, 0x34, 0x5c, 0x09, 0xc2, 0x38 } };

// Where Windows keeps the overlay the user selected. Reading it is the only
// portable way to know the current overlay: PowerGetActiveOverlayScheme only
// exists on Windows 11 22621 and later, and PowerGetEffectiveOverlayScheme
// reports what the active *scheme* yields rather than the user's choice.
constexpr auto kPowerSchemesKey
    = "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Power\\User\\PowerSchemes";
constexpr auto kOverlayAcValue = "ActiveOverlayAcPowerScheme";
constexpr auto kOverlayDcValue = "ActiveOverlayDcPowerScheme";

constexpr int kPollIntervalMs = 5000;

QString guidToString(const GUID& guid)
{
    return QUuid(guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
        guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6],
        guid.Data4[7])
        .toString(QUuid::WithoutBraces);
}

bool stringToGuid(const QString& text, GUID& guid)
{
    const QUuid value(text);

    // QUuid::isNull() cannot validate the input: the balanced overlay id *is*
    // the all-zero GUID. Round-tripping the text is the only check that accepts
    // it and still rejects garbage.
    if (value.toString(QUuid::WithoutBraces) != text)
        return false;

    guid = {};
    guid.Data1 = value.data1;
    guid.Data2 = value.data2;
    guid.Data3 = value.data3;
    for (int index = 0; index < 8; ++index)
        guid.Data4[index] = value.data4[index];
    return true;
}

QString normalizeId(const QString& value)
{
    const QString text = value.trimmed().toLower();
    if (text.isEmpty())
        return {};

    GUID scratch {};
    return stringToGuid(text, scratch) ? text : QString();
}

// PowerGetActiveOverlayScheme is not exported by every powrprof.dll, and it is
// not declared in the SDK headers. Resolving it at runtime keeps the binary
// loadable on machines that only have the legacy power schemes.
using GetActiveOverlayFn = DWORD(WINAPI*)(GUID*);
using SetActiveOverlayFn = DWORD(WINAPI*)(const GUID*);

struct OverlayApi {
    GetActiveOverlayFn getActive = nullptr;
    SetActiveOverlayFn setActive = nullptr;
};

const OverlayApi& overlayApi()
{
    static const OverlayApi api = [] {
        OverlayApi resolved;
        HMODULE module = GetModuleHandleW(L"powrprof.dll");
        if (!module)
            module = LoadLibraryExW(L"powrprof.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module)
            return resolved;

        resolved.getActive = reinterpret_cast<GetActiveOverlayFn>(
            GetProcAddress(module, "PowerGetActiveOverlayScheme"));
        resolved.setActive = reinterpret_cast<SetActiveOverlayFn>(
            GetProcAddress(module, "PowerSetActiveOverlayScheme"));
        return resolved;
    }();
    return api;
}

QString overlayIdFromRegistry()
{
    const QSettings power(QString::fromLatin1(kPowerSchemesKey), QSettings::NativeFormat);
    const QString ac = normalizeId(power.value(QString::fromLatin1(kOverlayAcValue)).toString());
    const QString dc = normalizeId(power.value(QString::fromLatin1(kOverlayDcValue)).toString());

    SYSTEM_POWER_STATUS status {};
    const bool onBattery = GetSystemPowerStatus(&status) != FALSE && status.ACLineStatus == 0;
    if (onBattery && !dc.isEmpty())
        return dc;
    return ac.isEmpty() ? dc : ac;
}

// Friendly name of a power scheme, as shown in the Control Panel. The name is
// stored per locale, so it is read through the power API rather than from the
// registry.
QString schemeName(const GUID& guid)
{
    DWORD size = 0;
    if (PowerReadFriendlyName(nullptr, &guid, nullptr, nullptr, nullptr, &size) != ERROR_SUCCESS)
        return {};
    if (size == 0)
        return {};

    QByteArray buffer(static_cast<int>(size), '\0');
    DWORD written = size;
    if (PowerReadFriendlyName(nullptr, &guid, nullptr, nullptr,
            reinterpret_cast<PUCHAR>(buffer.data()), &written)
        != ERROR_SUCCESS)
        return {};

    return QString::fromWCharArray(reinterpret_cast<const wchar_t*>(buffer.constData())).trimmed();
}

QVariantMap profileEntry(const QString& id, const QString& name, const QString& icon)
{
    QVariantMap entry;
    entry.insert(QStringLiteral("id"), id);
    entry.insert(QStringLiteral("name"), name);
    entry.insert(QStringLiteral("icon"), icon);
    return entry;
}

QVariantList overlayProfiles()
{
    QVariantList profiles;
    profiles.append(profileEntry(guidToString(kOverlayPowerSaver),
        QStringLiteral("Économie d'énergie"), QStringLiteral("energy_savings_leaf")));
    profiles.append(profileEntry(guidToString(kOverlayBalanced), QStringLiteral("Équilibré"),
        QStringLiteral("balance")));
    profiles.append(profileEntry(guidToString(kOverlayPerformance), QStringLiteral("Performances"),
        QStringLiteral("speed")));
    return profiles;
}
} // namespace

PowerProfileService::PowerProfileService(QObject* parent)
    : QObject(parent)
{
    m_poll = new QTimer(this);
    m_poll->setInterval(kPollIntervalMs);
    connect(m_poll, &QTimer::timeout, this, &PowerProfileService::refresh);
    m_poll->start();

    refresh();
}

PowerProfileService::~PowerProfileService() = default;

bool PowerProfileService::available() const
{
    return m_available;
}

bool PowerProfileService::overlayMode() const
{
    return m_overlayMode;
}

QVariantList PowerProfileService::profiles() const
{
    return m_profiles;
}

QString PowerProfileService::activeProfileId() const
{
    return m_activeProfileId;
}

void PowerProfileService::setProfile(const QString& id)
{
    const bool applied = m_overlayMode ? applyOverlay(id) : applyScheme(id);
    if (!applied) {
        emit actionFailed(QStringLiteral("Windows a refusé de changer le mode d'alimentation."));
        return;
    }

    refresh();
}

void PowerProfileService::refresh()
{
    if (overlayApi().setActive)
        readOverlay();
    else
        readSchemes();
}

bool PowerProfileService::applyOverlay(const QString& id)
{
    GUID guid {};
    if (!stringToGuid(id, guid))
        return false;

    const SetActiveOverlayFn setActive = overlayApi().setActive;
    if (!setActive)
        return false;
    return setActive(&guid) == ERROR_SUCCESS;
}

bool PowerProfileService::applyScheme(const QString& id)
{
    GUID guid {};
    if (!stringToGuid(id, guid))
        return false;
    return PowerSetActiveScheme(nullptr, &guid) == ERROR_SUCCESS;
}

void PowerProfileService::readOverlay()
{
    QString activeId;

    // Windows 11 22621+ reports the user's overlay directly.
    if (const GetActiveOverlayFn getActive = overlayApi().getActive) {
        GUID guid {};
        if (getActive(&guid) == ERROR_SUCCESS)
            activeId = guidToString(guid);
    }

    if (activeId.isEmpty())
        activeId = overlayIdFromRegistry();

    publish(true, true, overlayProfiles(), activeId);
}

void PowerProfileService::readSchemes()
{
    QVariantList profiles;

    for (DWORD index = 0;; ++index) {
        GUID guid {};
        DWORD size = sizeof(guid);
        const DWORD result = PowerEnumerate(nullptr, nullptr, nullptr, ACCESS_SCHEME, index,
            reinterpret_cast<UCHAR*>(&guid), &size);

        if (result != ERROR_SUCCESS)
            break;
        if (size != sizeof(guid))
            continue;

        const QString id = guidToString(guid);
        QString name = schemeName(guid);
        if (name.isEmpty())
            name = id;

        profiles.append(profileEntry(id, name, QStringLiteral("bolt")));
    }

    QString activeId;
    GUID* active = nullptr;
    if (PowerGetActiveScheme(nullptr, &active) == ERROR_SUCCESS && active) {
        activeId = guidToString(*active);
        LocalFree(active);
    }

    publish(!profiles.isEmpty(), false, profiles, activeId);
}

void PowerProfileService::publish(bool available, bool overlayMode,
    const QVariantList& profiles, const QString& activeProfileId)
{
    const bool dirty = m_available != available || m_overlayMode != overlayMode
        || m_profiles != profiles || m_activeProfileId != activeProfileId;
    if (!dirty)
        return;

    m_available = available;
    m_overlayMode = overlayMode;
    m_profiles = profiles;
    m_activeProfileId = activeProfileId;

    emit changed();

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_STATUS")) {
        qInfo().noquote() << "Power profiles:" << (overlayMode ? "overlay" : "schemes")
                          << "active=" << activeProfileId;
    }
}
