#pragma once

#include <QObject>
#include <QString>

// Session actions are separated from the UI so the confirmation dialog is the
// only path to a destructive call. The service never runs a shell command.
class PowerService final : public QObject {
    Q_OBJECT

public:
    explicit PowerService(QObject* parent = nullptr);

    Q_INVOKABLE bool lock();
    Q_INVOKABLE bool sleep();
    Q_INVOKABLE bool restart();
    Q_INVOKABLE bool shutdown();
    Q_INVOKABLE bool signOut();

signals:
    void actionFailed(const QString& message);

private:
    bool enableShutdownPrivilege();
    void fail(const QString& action, unsigned long error);
};
