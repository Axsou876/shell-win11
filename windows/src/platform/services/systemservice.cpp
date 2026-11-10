#include "systemservice.hpp"

#include <QDebug>
#include <QElapsedTimer>
#include <QSettings>
#include <QTimer>

#include <algorithm>
#include <numeric>

#include <winsock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace {
constexpr int kHistorySize = 60;

quint64 fileTimeValue(const FILETIME& time)
{
    ULARGE_INTEGER value = {};
    value.LowPart = time.dwLowDateTime;
    value.HighPart = time.dwHighDateTime;
    return value.QuadPart;
}

QString cpuName()
{
    QSettings registry(QStringLiteral(
                           "HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
        QSettings::NativeFormat);
    return registry.value(QStringLiteral("ProcessorNameString")).toString().simplified();
}
} // namespace

struct SystemService::Impl {
    quint64 lastIdle = 0;
    quint64 lastKernel = 0;
    quint64 lastUser = 0;
    bool cpuBaseline = false;

    quint64 lastInOctets = 0;
    quint64 lastOutOctets = 0;
    QElapsedTimer networkTimer;
    bool networkBaseline = false;

    ComPtr<IDXGIFactory1> factory;
    ComPtr<IDXGIAdapter3> adapter;
    bool gpuInitialized = false;
    bool gpuInitFailed = false;

    PDH_HQUERY gpuQuery = nullptr;
    PDH_HCOUNTER gpuCounter = nullptr;
    bool pdhInitialized = false;
    bool pdhFailed = false;

    int tick = 0;
};

SystemService::SystemService(QObject* parent)
    : QObject(parent)
    , m_impl(std::make_unique<Impl>())
    , m_timer(new QTimer(this))
{
    m_cpuName = cpuName();
    m_timer->setInterval(1000);
    m_timer->setTimerType(Qt::CoarseTimer);
    connect(m_timer, &QTimer::timeout, this, &SystemService::refresh);
    initializeGpu();
    refresh();
    m_timer->start();
}

SystemService::~SystemService()
{
    if (m_impl->gpuQuery)
        PdhCloseQuery(m_impl->gpuQuery);
}

void SystemService::initializeGpu()
{
    if (m_impl->gpuInitialized)
        return;
    m_impl->gpuInitialized = true;

    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&m_impl->factory)))) {
        m_impl->gpuInitFailed = true;
        return;
    }

    ComPtr<IDXGIAdapter1> adapter1;
    if (FAILED(m_impl->factory->EnumAdapters1(0, &adapter1))
        || FAILED(adapter1.As(&m_impl->adapter))) {
        m_impl->gpuInitFailed = true;
        return;
    }

    DXGI_ADAPTER_DESC1 description = {};
    if (SUCCEEDED(adapter1->GetDesc1(&description)))
        m_gpuName = QString::fromWCharArray(description.Description).simplified();
    m_gpuAvailable = !m_gpuName.isEmpty();

    if (PdhOpenQueryW(nullptr, 0, &m_impl->gpuQuery) == ERROR_SUCCESS
        && PdhAddEnglishCounterW(m_impl->gpuQuery,
               L"\\GPU Engine(*)\\Utilization Percentage", 0, &m_impl->gpuCounter)
            == ERROR_SUCCESS) {
        PdhCollectQueryData(m_impl->gpuQuery);
        m_impl->pdhInitialized = true;
        m_gpuUsageAvailable = true;
    } else {
        m_impl->pdhFailed = true;
    }
}

void SystemService::refresh()
{
    ++m_impl->tick;

    FILETIME idleTime = {};
    FILETIME kernelTime = {};
    FILETIME userTime = {};
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        const quint64 idle = fileTimeValue(idleTime);
        const quint64 kernel = fileTimeValue(kernelTime);
        const quint64 user = fileTimeValue(userTime);
        if (m_impl->cpuBaseline) {
            const quint64 idleDelta = idle - m_impl->lastIdle;
            const quint64 totalDelta = (kernel - m_impl->lastKernel) + (user - m_impl->lastUser);
            if (totalDelta > 0)
                m_cpuUsage = qBound(0.0, (1.0 - static_cast<double>(idleDelta)
                                                    / static_cast<double>(totalDelta))
                        * 100.0,
                    100.0);
        }
        m_impl->lastIdle = idle;
        m_impl->lastKernel = kernel;
        m_impl->lastUser = user;
        m_impl->cpuBaseline = true;
    }

    MEMORYSTATUSEX memory = {};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory)) {
        m_memoryTotalGb = memory.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
        m_memoryUsedGb = (memory.ullTotalPhys - memory.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0);
        m_memoryPercent = memory.dwMemoryLoad;
    }

    if (!m_impl->networkTimer.isValid())
        m_impl->networkTimer.start();
    PMIB_IF_TABLE2 table = nullptr;
    if (GetIfTable2(&table) == NO_ERROR && table) {
        quint64 inOctets = 0;
        quint64 outOctets = 0;
        for (ULONG i = 0; i < table->NumEntries; ++i) {
            const MIB_IF_ROW2& row = table->Table[i];
            if (row.OperStatus != IfOperStatusUp
                || row.Type == IF_TYPE_SOFTWARE_LOOPBACK)
                continue;
            inOctets += row.InOctets;
            outOctets += row.OutOctets;
        }
        if (m_impl->networkBaseline) {
            const qint64 elapsed = m_impl->networkTimer.restart();
            if (elapsed > 0) {
                m_networkDownKbps = static_cast<double>(inOctets - m_impl->lastInOctets)
                    * 8.0 / elapsed;
                m_networkUpKbps = static_cast<double>(outOctets - m_impl->lastOutOctets)
                    * 8.0 / elapsed;
            }
        } else {
            m_impl->networkTimer.restart();
            m_impl->networkBaseline = true;
        }
        m_impl->lastInOctets = inOctets;
        m_impl->lastOutOctets = outOctets;
        FreeMibTable(table);
    }

    ULARGE_INTEGER total = {};
    ULARGE_INTEGER free = {};
    if (GetDiskFreeSpaceExW(L"C:\\", nullptr, &total, &free)) {
        m_diskTotalGb = total.QuadPart / (1024.0 * 1024.0 * 1024.0);
        m_diskUsedGb = (total.QuadPart - free.QuadPart) / (1024.0 * 1024.0 * 1024.0);
        m_diskPercent = m_diskTotalGb > 0 ? m_diskUsedGb / m_diskTotalGb * 100.0 : 0;
    }

    m_uptimeSeconds = static_cast<qint64>(GetTickCount64() / 1000);
    appendHistory(m_cpuHistory, m_cpuUsage);
    appendHistory(m_memoryHistory, m_memoryPercent);
    appendHistory(m_networkHistory, m_networkDownKbps + m_networkUpKbps);

    if (m_impl->tick % 2 == 0)
        refreshGpu();

    emit changed();
}

void SystemService::refreshGpu()
{
    if (m_impl->gpuInitFailed || !m_impl->adapter)
        return;

    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
    if (SUCCEEDED(m_impl->adapter->QueryVideoMemoryInfo(
            0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info))) {
        m_gpuMemoryUsedMb = info.CurrentUsage / (1024.0 * 1024.0);
        m_gpuMemoryTotalMb = info.Budget / (1024.0 * 1024.0);
    }

    if (!m_impl->pdhInitialized || !m_impl->gpuCounter)
        return;

    if (PdhCollectQueryData(m_impl->gpuQuery) != ERROR_SUCCESS)
        return;

    DWORD size = 0;
    DWORD count = 0;
    PDH_STATUS status = PdhGetFormattedCounterArrayW(
        m_impl->gpuCounter, PDH_FMT_DOUBLE, &size, &count, nullptr);
    if (status != PDH_MORE_DATA || size == 0) {
        m_gpuUsageAvailable = false;
        m_gpuUsage = -1;
        return;
    }

    QByteArray buffer(static_cast<int>(size), '\0');
    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
    status = PdhGetFormattedCounterArrayW(
        m_impl->gpuCounter, PDH_FMT_DOUBLE, &size, &count, items);
    if (status != ERROR_SUCCESS) {
        m_gpuUsageAvailable = false;
        m_gpuUsage = -1;
        return;
    }

    double totalUsage = 0;
    for (DWORD i = 0; i < count; ++i) {
        if (items[i].FmtValue.CStatus == ERROR_SUCCESS)
            totalUsage += items[i].FmtValue.doubleValue;
    }
    m_gpuUsage = qBound(0.0, totalUsage, 100.0);
    m_gpuUsageAvailable = true;
    appendHistory(m_gpuHistory, m_gpuUsage);
}

void SystemService::appendHistory(QVariantList& history, qreal value)
{
    history.append(value);
    if (history.size() > kHistorySize)
        history.removeFirst();
}
