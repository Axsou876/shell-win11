#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

// Bluetooth radio control (WinRT Radios) plus device listing, pairing and
// removal (Win32 bthprops).
//
// Everything runs on one worker thread: WinRT needs its own apartment, and a
// classic Bluetooth inquiry blocks for seconds, so it can never run on the GUI
// thread. Commands are queued and drained by that worker; results are posted
// back with a queued invocation.
class BluetoothService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool present READ present NOTIFY changed)
    Q_PROPERTY(bool enabled READ enabled NOTIFY changed)
    Q_PROPERTY(bool known READ known NOTIFY changed)
    // Each entry is { address, name, connected, paired }. `address` is the
    // "AA:BB:CC:DD:EE:FF" form used by every invokable below.
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY devicesChanged)

public:
    explicit BluetoothService(QObject* parent = nullptr);
    ~BluetoothService() override;

    [[nodiscard]] bool present() const;
    [[nodiscard]] bool enabled() const;
    [[nodiscard]] bool known() const;
    [[nodiscard]] QVariantList devices() const;
    [[nodiscard]] bool scanning() const;

    Q_INVOKABLE void setEnabled(bool enabled);
    Q_INVOKABLE void refreshDevices();
    // Runs a classic Bluetooth inquiry (~5 s) and also reports unpaired
    // devices that are currently discoverable. BLE-only devices are not
    // returned by the Win32 inquiry; that limitation is documented.
    Q_INVOKABLE void scanDevices();
    Q_INVOKABLE void pairDevice(const QString& address);
    Q_INVOKABLE void unpairDevice(const QString& address);

    void stop();

signals:
    void changed();
    void devicesChanged();
    void toggleFailed(const QString& message);

private:
    enum class CommandType { SetRadio, Refresh, Scan, Pair, Unpair };

    struct Command {
        CommandType type = CommandType::Refresh;
        bool enabled = false;
        QString address;
    };

    void run();
    void handleCommand(const Command& command);
    void setState(bool present, bool enabled, bool known);
    void publishDevices(const QVariantList& devices, bool scanning);
    void post(Command command);

    std::thread m_worker;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_stop { false };
    std::atomic<bool> m_started { false };
    std::deque<Command> m_commands;
    bool m_present = false;
    bool m_enabled = false;
    bool m_known = false;
    QVariantList m_devices;
    bool m_scanning = false;
};
