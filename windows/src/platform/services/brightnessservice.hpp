#pragma once

#include <QObject>

#include <mutex>

// Controls the primary monitor's backlight through DDC/CI (VCP 0x10).
//
// DDC/CI is a slow 100 ms-ish I2C transaction. Reads and writes therefore run
// on a worker thread; the QML property is updated optimistically so the OSD
// and any slider never wait for the monitor. A single worker at a time keeps
// concurrent Get/Set calls from interleaving on the same physical monitor.
class BrightnessService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool known READ known NOTIFY changed)
    Q_PROPERTY(int percent READ percent NOTIFY changed)

public:
    explicit BrightnessService(QObject* parent = nullptr);
    ~BrightnessService() override;

    [[nodiscard]] bool available() const;
    [[nodiscard]] bool known() const;
    [[nodiscard]] int percent() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setPercent(int percent);
    Q_INVOKABLE void increase(int step = 5);
    Q_INVOKABLE void decrease(int step = 5);

signals:
    void changed();

private:
    // Both helpers run on a worker thread and never touch Qt properties.
    void runWriter();
    void publish(bool available, bool known, int percent);
    void publishFromWorker(bool available, bool known, int percent);

    // m_busy and m_pendingPercent are only ever touched with m_pendingMutex
    // held, so an enqueue can never race the worker's exit decision.
    std::mutex m_pendingMutex;
    bool m_busy = false;
    int m_pendingPercent = -1;

    bool m_available = false;
    bool m_known = false;
    int m_percent = 0;
};
