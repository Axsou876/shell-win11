#pragma once

#include <QList>
#include <QObject>
#include <QStringList>
#include <QVariantList>

// Shell notification inbox. External Windows toast capture is not available to
// an unpackaged application without additional permissions; this service owns
// shell notifications and exposes test notifications for the reference flow
// (long text, actions, middle-click/swipe dismissal).
class NotificationService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList notifications READ notifications NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    // Shell-level "do not disturb": notifications are still recorded in the
    // inbox, they just stop raising the on-screen toast. Persisted, like the
    // Linux shell's PersistentState.doNotDisturb.
    Q_PROPERTY(bool doNotDisturb READ doNotDisturb WRITE setDoNotDisturb NOTIFY changed)

public:
    explicit NotificationService(QObject* parent = nullptr);

    [[nodiscard]] QVariantList notifications() const;
    [[nodiscard]] int count() const { return m_items.size(); }
    [[nodiscard]] bool doNotDisturb() const;
    void setDoNotDisturb(bool enabled);
    Q_INVOKABLE void toggleDoNotDisturb();

    Q_INVOKABLE void addNotification(const QString& app, const QString& title,
        const QString& body, const QStringList& actions = {});
    Q_INVOKABLE void addTestNotification();
    Q_INVOKABLE void dismiss(int id);
    Q_INVOKABLE void invokeAction(int id, const QString& action);
    Q_INVOKABLE void clear();

signals:
    void changed();
    void actionInvoked(const QString& action);

private:
    struct Item {
        int id = 0;
        QString app;
        QString title;
        QString body;
        // Milliseconds since the epoch. The label is derived where it is shown
        // (components/TimeAgo.qml), so it stays relative instead of freezing
        // at the moment the notification arrived.
        qint64 createdAt = 0;
        QStringList actions;
    };

    QList<Item> m_items;
    int m_nextId = 1;
    bool m_doNotDisturb = false;
};
