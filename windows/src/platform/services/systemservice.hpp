#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <memory>

// Real Windows system counters for the Performance dashboard. Every value is
// read from Win32 (GetSystemTimes, GlobalMemoryStatusEx, GetIfTable2,
// GetDiskFreeSpaceEx, DXGI, PDH). Unsupported counters expose -1 rather than
// a fabricated value.
class SystemService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(qreal cpuUsage READ cpuUsage NOTIFY changed)
    Q_PROPERTY(qreal memoryUsedGb READ memoryUsedGb NOTIFY changed)
    Q_PROPERTY(qreal memoryTotalGb READ memoryTotalGb NOTIFY changed)
    Q_PROPERTY(qreal memoryPercent READ memoryPercent NOTIFY changed)
    Q_PROPERTY(qreal gpuUsage READ gpuUsage NOTIFY changed)
    Q_PROPERTY(qreal gpuMemoryUsedMb READ gpuMemoryUsedMb NOTIFY changed)
    Q_PROPERTY(qreal gpuMemoryTotalMb READ gpuMemoryTotalMb NOTIFY changed)
    Q_PROPERTY(bool gpuAvailable READ gpuAvailable NOTIFY changed)
    Q_PROPERTY(bool gpuUsageAvailable READ gpuUsageAvailable NOTIFY changed)
    Q_PROPERTY(QString cpuName READ cpuName CONSTANT)
    Q_PROPERTY(QString gpuName READ gpuName NOTIFY changed)
    Q_PROPERTY(qreal networkDownKbps READ networkDownKbps NOTIFY changed)
    Q_PROPERTY(qreal networkUpKbps READ networkUpKbps NOTIFY changed)
    Q_PROPERTY(qreal diskUsedGb READ diskUsedGb NOTIFY changed)
    Q_PROPERTY(qreal diskTotalGb READ diskTotalGb NOTIFY changed)
    Q_PROPERTY(qreal diskPercent READ diskPercent NOTIFY changed)
    Q_PROPERTY(qint64 uptimeSeconds READ uptimeSeconds NOTIFY changed)
    Q_PROPERTY(QVariantList cpuHistory READ cpuHistory NOTIFY changed)
    Q_PROPERTY(QVariantList memoryHistory READ memoryHistory NOTIFY changed)
    Q_PROPERTY(QVariantList gpuHistory READ gpuHistory NOTIFY changed)
    Q_PROPERTY(QVariantList networkHistory READ networkHistory NOTIFY changed)

public:
    explicit SystemService(QObject* parent = nullptr);
    ~SystemService() override;

    [[nodiscard]] qreal cpuUsage() const { return m_cpuUsage; }
    [[nodiscard]] qreal memoryUsedGb() const { return m_memoryUsedGb; }
    [[nodiscard]] qreal memoryTotalGb() const { return m_memoryTotalGb; }
    [[nodiscard]] qreal memoryPercent() const { return m_memoryPercent; }
    [[nodiscard]] qreal gpuUsage() const { return m_gpuUsage; }
    [[nodiscard]] qreal gpuMemoryUsedMb() const { return m_gpuMemoryUsedMb; }
    [[nodiscard]] qreal gpuMemoryTotalMb() const { return m_gpuMemoryTotalMb; }
    [[nodiscard]] bool gpuAvailable() const { return m_gpuAvailable; }
    [[nodiscard]] bool gpuUsageAvailable() const { return m_gpuUsageAvailable; }
    [[nodiscard]] QString cpuName() const { return m_cpuName; }
    [[nodiscard]] QString gpuName() const { return m_gpuName; }
    [[nodiscard]] qreal networkDownKbps() const { return m_networkDownKbps; }
    [[nodiscard]] qreal networkUpKbps() const { return m_networkUpKbps; }
    [[nodiscard]] qreal diskUsedGb() const { return m_diskUsedGb; }
    [[nodiscard]] qreal diskTotalGb() const { return m_diskTotalGb; }
    [[nodiscard]] qreal diskPercent() const { return m_diskPercent; }
    [[nodiscard]] qint64 uptimeSeconds() const { return m_uptimeSeconds; }
    [[nodiscard]] QVariantList cpuHistory() const { return m_cpuHistory; }
    [[nodiscard]] QVariantList memoryHistory() const { return m_memoryHistory; }
    [[nodiscard]] QVariantList gpuHistory() const { return m_gpuHistory; }
    [[nodiscard]] QVariantList networkHistory() const { return m_networkHistory; }

signals:
    void changed();

private:
    void refresh();
    void appendHistory(QVariantList& history, qreal value);
    void initializeGpu();
    void refreshGpu();

    struct Impl;
    std::unique_ptr<Impl> m_impl;

    class QTimer* m_timer = nullptr;
    qreal m_cpuUsage = 0;
    qreal m_memoryUsedGb = 0;
    qreal m_memoryTotalGb = 0;
    qreal m_memoryPercent = 0;
    qreal m_gpuUsage = -1;
    qreal m_gpuMemoryUsedMb = 0;
    qreal m_gpuMemoryTotalMb = 0;
    bool m_gpuAvailable = false;
    bool m_gpuUsageAvailable = false;
    QString m_cpuName;
    QString m_gpuName;
    qreal m_networkDownKbps = 0;
    qreal m_networkUpKbps = 0;
    qreal m_diskUsedGb = 0;
    qreal m_diskTotalGb = 0;
    qreal m_diskPercent = 0;
    qint64 m_uptimeSeconds = 0;
    QVariantList m_cpuHistory;
    QVariantList m_memoryHistory;
    QVariantList m_gpuHistory;
    QVariantList m_networkHistory;
};
