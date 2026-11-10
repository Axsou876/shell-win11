#pragma once

#include <QObject>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class BluetoothService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool present READ present NOTIFY changed)
    Q_PROPERTY(bool enabled READ enabled NOTIFY changed)
    Q_PROPERTY(bool known READ known NOTIFY changed)

public:
    explicit BluetoothService(QObject* parent = nullptr);
    ~BluetoothService() override;

    [[nodiscard]] bool present() const;
    [[nodiscard]] bool enabled() const;
    [[nodiscard]] bool known() const;

    void stop();

signals:
    void changed();

private:
    void run();
    void setState(bool present, bool enabled, bool known);

    std::thread m_worker;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_stop { false };
    std::atomic<bool> m_started { false };
    bool m_present = false;
    bool m_enabled = false;
    bool m_known = false;
};
