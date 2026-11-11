#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVariantList>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

// Real Windows virtual desktops. Enumeration/switch/create/remove use the
// private IVirtualDesktopManagerInternal COM interface exposed by Explorer
// (validated on Windows 11 24H2/26H2); window moves use the documented
// IVirtualDesktopManager. All COM calls happen on one STA worker thread.
class VirtualDesktopService final : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QString currentId READ currentId NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        IndexRole,
        ActiveRole,
        OccupiedRole,
        WindowsRole,
    };
    Q_ENUM(Roles)

    explicit VirtualDesktopService(QObject* parent = nullptr);
    ~VirtualDesktopService() override;

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QVariantList desktops() const { return m_desktops; }
    [[nodiscard]] int count() const { return static_cast<int>(m_desktops.size()); }
    [[nodiscard]] QString currentId() const { return m_currentId; }
    [[nodiscard]] bool available() const { return m_available; }
    [[nodiscard]] QString error() const { return m_error; }

    Q_INVOKABLE void switchTo(const QString& id);
    Q_INVOKABLE void createDesktop();
    Q_INVOKABLE void closeDesktop(const QString& id);
    Q_INVOKABLE void moveWindow(const QString& windowId, const QString& desktopId);
    Q_INVOKABLE void refresh();

signals:
    void changed();

private:
    enum class CommandType { Refresh, Switch, Create, Close, Move };

    struct Command {
        CommandType type = CommandType::Refresh;
        QString id;
        QString windowId;
    };

    void enqueue(Command command);
    void run();
    void stop();
    void publish(QVariantList desktops, const QString& currentId, bool available,
        const QString& error);
    void applyDesktops(const QVariantList& desktops);

    std::thread m_thread;
    std::atomic<bool> m_stop { false };
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<Command> m_commands;

    QVariantList m_desktops;
    QString m_currentId;
    bool m_available = false;
    QString m_error;
};
