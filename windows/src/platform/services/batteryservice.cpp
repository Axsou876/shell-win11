#include "batteryservice.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include <windows.h>

BatteryService::BatteryService(QObject* parent)
    : QObject(parent)
{
    refresh();
    if (QCoreApplication::instance())
        QCoreApplication::instance()->installNativeEventFilter(this);
}

BatteryService::~BatteryService()
{
    if (QCoreApplication::instance())
        QCoreApplication::instance()->removeNativeEventFilter(this);
}

bool BatteryService::present() const
{
    return m_present;
}

bool BatteryService::known() const
{
    return m_known;
}

int BatteryService::level() const
{
    return m_level;
}

bool BatteryService::charging() const
{
    return m_charging;
}

bool BatteryService::onAc() const
{
    return m_onAc;
}

void BatteryService::refresh()
{
    SYSTEM_POWER_STATUS status = {};
    if (!GetSystemPowerStatus(&status)) {
        setState(false, false, -1, false, false);
        return;
    }

    const bool unknown = status.BatteryFlag == 255;
    const bool noBattery = (status.BatteryFlag & 128) != 0;
    const bool present = !unknown && !noBattery;
    const int level = status.BatteryLifePercent == 255 ? -1 : status.BatteryLifePercent;
    const bool known = !unknown && level >= 0;
    const bool charging = (status.BatteryFlag & 8) != 0;
    const bool onAc = status.ACLineStatus == 1;

    setState(present, known, level, charging, onAc);
}

bool BatteryService::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
{
    Q_UNUSED(result);

    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
        return false;

    const auto* msg = static_cast<MSG*>(message);
    if (msg && msg->message == WM_POWERBROADCAST && msg->wParam == PBT_APMPOWERSTATUSCHANGE)
        QTimer::singleShot(0, this, &BatteryService::refresh);

    return false;
}

void BatteryService::setState(bool present, bool known, int level, bool charging, bool onAc)
{
    if (m_present == present && m_known == known && m_level == level && m_charging == charging
        && m_onAc == onAc)
        return;

    m_present = present;
    m_known = known;
    m_level = level;
    m_charging = charging;
    m_onAc = onAc;
    emit changed();

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_STATUS")) {
        qInfo().noquote() << "Battery: present=" << present << "known=" << known << "level=" << level
                          << "charging=" << charging << "onAc=" << onAc;
    }
}
