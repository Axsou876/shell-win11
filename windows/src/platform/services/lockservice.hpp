#pragma once

#include <QObject>
#include <QString>

class QTimer;

// Windows port of the Linux session lock screen.
//
// This is a session lock, not a security boundary. Qt has no secure-input
// channel, so Win+L (and the Ctrl+Alt+Del secure desktop) remain the
// authoritative locks; the overlay only blocks the shell's own surfaces and
// covers the desktop visually. The password field validates the current
// account with LogonUserW, the documented Win32 credential check. PIN and
// Windows Hello credentials cannot be validated that way, which is why the
// screen always offers a handover to the real Windows lock.
//
// The desktop is frozen *before* the overlay exists, so the lock screen never
// photographs itself. requestLock() only announces the intent; the QML layer
// captures every monitor and then calls show().
class LockService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool locked READ locked NOTIFY lockedChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool blocked READ blocked NOTIFY blockedChanged)
    Q_PROPERTY(bool capsLock READ capsLock NOTIFY capsLockChanged)
    Q_PROPERTY(int attempts READ attempts NOTIFY changed)
    Q_PROPERTY(int maxAttempts READ maxAttempts CONSTANT)
    Q_PROPERTY(QString error READ error NOTIFY changed)

public:
    explicit LockService(QObject* parent = nullptr);

    [[nodiscard]] bool locked() const { return m_locked; }
    [[nodiscard]] bool busy() const { return m_busy; }
    [[nodiscard]] bool blocked() const { return m_attempts >= maxAttempts(); }
    [[nodiscard]] bool capsLock() const;
    [[nodiscard]] int attempts() const { return m_attempts; }
    [[nodiscard]] int maxAttempts() const { return 5; }
    [[nodiscard]] QString error() const { return m_error; }

    // Emitted so every consumer (power menu, launcher, global hotkey) can ask
    // for the lock screen without capturing anything themselves.
    Q_INVOKABLE void requestLock();
    // Called once the frozen frames are in the capture service.
    Q_INVOKABLE void show();
    // Runs LogonUserW on a worker thread; the result arrives through
    // lockedChanged/unlocked and error.
    Q_INVOKABLE void authenticate(const QString& password);
    // Hands over to the Windows secure desktop. The QML layer hides the
    // overlay before calling this, so unlocking Windows does not land back on
    // a still-visible lock screen.
    Q_INVOKABLE bool lockWindows();
    Q_INVOKABLE void unlock();

signals:
    void lockRequested();
    void unlocked();
    void lockedChanged();
    void busyChanged();
    void blockedChanged();
    void capsLockChanged();
    void changed();

private:
    void setBusy(bool busy);
    void setError(const QString& message);
    void finishAuthentication(bool ok, const QString& message);

    bool m_locked = false;
    bool m_busy = false;
    int m_attempts = 0;
    QString m_error;
    QTimer* m_capsTimer = nullptr;
    bool m_capsLock = false;
    // Incremented on every submitted attempt and on unlock. A queued result
    // carrying an older id is dropped, so a slow LogonUserW can never unlock
    // the overlay after the user has left the screen another way.
    quint64 m_requestId = 0;
};
