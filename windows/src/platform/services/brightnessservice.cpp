#include "brightnessservice.hpp"

#include <QDebug>
#include <QMetaObject>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <cmath>
#include <vector>

#include <windows.h>
// Splits into two SDK headers: enumeration lives in the physical-monitor API,
// while the raw VCP get/set calls live in the low-level configuration API.
#include <lowlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>

namespace {
// VCP code 0x10 is the standard DDC/CI "brightness" control.
constexpr BYTE kBrightnessVcpCode = 0x10;
constexpr int kRefreshIntervalMs = 20000;
constexpr int kDefaultStep = 5;

struct PhysicalMonitorSet {
    std::vector<PHYSICAL_MONITOR> monitors;

    ~PhysicalMonitorSet()
    {
        if (!monitors.empty())
            DestroyPhysicalMonitors(static_cast<DWORD>(monitors.size()), monitors.data());
    }
};

// The primary monitor always starts at (0,0) in the virtual desktop, so this
// resolves the monitor the user sees as "main" without depending on a window.
bool openPrimaryMonitors(PhysicalMonitorSet& set)
{
    const HMONITOR monitor = MonitorFromPoint(POINT { 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    if (!monitor)
        return false;

    DWORD count = 0;
    if (!GetNumberOfPhysicalMonitorsFromHMONITOR(monitor, &count) || count == 0)
        return false;

    set.monitors.resize(count);
    if (!GetPhysicalMonitorsFromHMONITOR(monitor, count, set.monitors.data())) {
        set.monitors.clear();
        return false;
    }
    return true;
}

bool readDdc(int* percent, bool* available)
{
    PhysicalMonitorSet set;
    if (!openPrimaryMonitors(set))
        return false;

    bool supported = false;
    int value = 0;
    for (const PHYSICAL_MONITOR& monitor : set.monitors) {
        DWORD current = 0;
        DWORD maximum = 0;
        if (!GetVCPFeatureAndVCPFeatureReply(
                monitor.hPhysicalMonitor, kBrightnessVcpCode, nullptr, &current, &maximum))
            continue;
        if (maximum == 0)
            continue;
        supported = true;
        value = static_cast<int>(std::min<DWORD>(100, current * 100 / maximum));
        break;
    }

    if (!supported)
        return false;

    *percent = value;
    if (available)
        *available = true;
    return true;
}

bool writeDdc(int percent)
{
    PhysicalMonitorSet set;
    if (!openPrimaryMonitors(set))
        return false;

    bool written = false;
    for (const PHYSICAL_MONITOR& monitor : set.monitors) {
        DWORD current = 0;
        DWORD maximum = 0;
        if (!GetVCPFeatureAndVCPFeatureReply(
                monitor.hPhysicalMonitor, kBrightnessVcpCode, nullptr, &current, &maximum)
            || maximum == 0)
            continue;
        const DWORD target = static_cast<DWORD>(std::round(percent * maximum / 100.0));
        if (SetVCPFeature(monitor.hPhysicalMonitor, kBrightnessVcpCode, target))
            written = true;
    }
    return written;
}
} // namespace

BrightnessService::BrightnessService(QObject* parent)
    : QObject(parent)
{
    refresh();

    auto* timer = new QTimer(this);
    timer->setInterval(kRefreshIntervalMs);
    connect(timer, &QTimer::timeout, this, &BrightnessService::refresh);
    timer->start();
}

BrightnessService::~BrightnessService() = default;

bool BrightnessService::available() const
{
    return m_available;
}

bool BrightnessService::known() const
{
    return m_known;
}

int BrightnessService::percent() const
{
    return m_percent;
}

void BrightnessService::refresh()
{
    {
        std::lock_guard lock(m_pendingMutex);
        if (m_busy)
            return;
        m_busy = true;
    }

    QtConcurrent::run([this]() {
        int percent = 0;
        const bool ok = readDdc(&percent, nullptr);
        {
            std::lock_guard lock(m_pendingMutex);
            m_busy = false;
        }
        publishFromWorker(ok, ok, percent);
    });
}

void BrightnessService::setPercent(int percent)
{
    const int clamped = std::clamp(percent, 0, 100);

    // Update the UI and OSD immediately; the monitor catches up on the worker.
    publish(true, true, clamped);

    bool startWorker = false;
    {
        std::lock_guard lock(m_pendingMutex);
        m_pendingPercent = clamped;
        if (!m_busy) {
            m_busy = true;
            startWorker = true;
        }
    }

    if (startWorker)
        QtConcurrent::run([this]() { runWriter(); });
}

void BrightnessService::runWriter()
{
    bool written = false;
    // setPercent() always queues a value before starting this worker, so the
    // loop body runs at least once before this initial value could be used.
    int target = 0;

    while (true) {
        {
            std::lock_guard lock(m_pendingMutex);
            if (m_pendingPercent < 0) {
                // Clearing the flag under the lock makes the exit atomic with
                // respect to setPercent(): a new value either starts a fresh
                // worker or stays queued for this one. Nothing is lost.
                m_busy = false;
                break;
            }
            target = m_pendingPercent;
            m_pendingPercent = -1;
        }

        if (writeDdc(target))
            written = true;

        // Read back so vendor rounding (e.g. 0-100 -> 0-80) is reflected
        // instead of leaving the OSD on a value the panel never accepted.
        int read = 0;
        if (readDdc(&read, nullptr))
            target = read;
    }

    publishFromWorker(written, written, target);
}

void BrightnessService::increase(int step)
{
    const int amount = step > 0 ? step : kDefaultStep;
    setPercent(percent() + amount);
}

void BrightnessService::decrease(int step)
{
    const int amount = step > 0 ? step : kDefaultStep;
    setPercent(percent() - amount);
}

void BrightnessService::publish(bool available, bool known, int percent)
{
    const int clamped = std::clamp(percent, 0, 100);
    if (m_available == available && m_known == known && m_percent == clamped)
        return;

    m_available = available;
    m_known = known;
    m_percent = clamped;
    emit changed();

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_BRIGHTNESS")) {
        qInfo().noquote() << "Brightness: available=" << available << "known=" << known
                          << "percent=" << clamped;
    }
}

void BrightnessService::publishFromWorker(bool available, bool known, int percent)
{
    QMetaObject::invokeMethod(
        this, [this, available, known, percent]() { publish(available, known, percent); },
        Qt::QueuedConnection);
}
