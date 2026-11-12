#pragma once

#include <QObject>
#include <QString>

// Keeps the session awake, the "Keep awake" card of the Linux utilities
// module.
//
// Windows has a documented per-process request object for exactly this:
// PowerCreateRequest + PowerSetRequest. Unlike SetThreadExecutionState it is
// not tied to the calling thread, it shows up in `powercfg /requests` under a
// readable reason, and the OS drops it automatically if the process dies.
class IdleInhibitService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool active READ active NOTIFY changed)
    // Local time the inhibition started, "HH:mm", or empty when inactive.
    Q_PROPERTY(QString since READ since NOTIFY changed)

public:
    explicit IdleInhibitService(QObject* parent = nullptr);
    ~IdleInhibitService() override;

    [[nodiscard]] bool active() const;
    [[nodiscard]] QString since() const;

    Q_INVOKABLE void setActive(bool active);
    Q_INVOKABLE void toggle();

signals:
    void changed();
    void actionFailed(const QString& message);

private:
    void apply(bool active);

    // POWER_REQUEST handle. Kept as void* so this header does not pull in
    // windows.h.
    void* m_request = nullptr;
    bool m_active = false;
    QString m_since;
};
