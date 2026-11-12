#include "notificationservice.hpp"

#include <QDateTime>
#include <QLocale>
#include <QSettings>

NotificationService::NotificationService(QObject* parent)
    : QObject(parent)
{
    QSettings settings;
    m_doNotDisturb = settings.value(QStringLiteral("notifications/doNotDisturb"), false).toBool();
}

bool NotificationService::doNotDisturb() const
{
    return m_doNotDisturb;
}

void NotificationService::setDoNotDisturb(bool enabled)
{
    if (m_doNotDisturb == enabled)
        return;

    m_doNotDisturb = enabled;

    QSettings settings;
    settings.setValue(QStringLiteral("notifications/doNotDisturb"), enabled);

    emit changed();
}

void NotificationService::toggleDoNotDisturb()
{
    setDoNotDisturb(!m_doNotDisturb);
}

QVariantList NotificationService::notifications() const
{
    QVariantList result;
    for (const Item& item : m_items) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), item.id);
        map.insert(QStringLiteral("app"), item.app);
        map.insert(QStringLiteral("title"), item.title);
        map.insert(QStringLiteral("body"), item.body);
        map.insert(QStringLiteral("time"), item.time);
        map.insert(QStringLiteral("actions"), item.actions);
        result.append(map);
    }
    return result;
}

void NotificationService::addNotification(const QString& app, const QString& title,
    const QString& body, const QStringList& actions)
{
    Item item;
    item.id = m_nextId++;
    item.app = app;
    item.title = title;
    item.body = body;
    item.time = QStringLiteral("maintenant");
    item.actions = actions;
    m_items.prepend(item);
    if (m_items.size() > 50)
        m_items.removeLast();
    emit changed();
}

void NotificationService::addTestNotification()
{
    addNotification(QStringLiteral("Shell"), QStringLiteral("Test notification"),
        QStringLiteral("Here's a really long message to test truncation and wrapping. "
                       "You can middle click or flick this notification to dismiss it!"),
        { QStringLiteral("Another action") });
}

void NotificationService::dismiss(int id)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).id == id) {
            m_items.removeAt(i);
            emit changed();
            return;
        }
    }
}

void NotificationService::invokeAction(int id, const QString& action)
{
    emit actionInvoked(action);
    dismiss(id);
}

void NotificationService::clear()
{
    if (m_items.isEmpty())
        return;
    m_items.clear();
    emit changed();
}
