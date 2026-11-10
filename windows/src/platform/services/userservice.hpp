#pragma once

#include <QObject>
#include <QString>

// Windows account information for the dashboard profile card and the power
// menu. The avatar is read from the account-picture folder; no placeholder
// photo is generated when Windows has none.
class UserService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString displayName READ displayName NOTIFY changed)
    Q_PROPERTY(QString userName READ userName NOTIFY changed)
    Q_PROPERTY(QString avatarPath READ avatarPath NOTIFY changed)
    Q_PROPERTY(QString windowsVersion READ windowsVersion NOTIFY changed)
    Q_PROPERTY(QString hostName READ hostName NOTIFY changed)

public:
    explicit UserService(QObject* parent = nullptr);

    [[nodiscard]] QString displayName() const { return m_displayName; }
    [[nodiscard]] QString userName() const { return m_userName; }
    [[nodiscard]] QString avatarPath() const { return m_avatarPath; }
    [[nodiscard]] QString windowsVersion() const { return m_windowsVersion; }
    [[nodiscard]] QString hostName() const { return m_hostName; }

signals:
    void changed();

private:
    void loadAvatar();

    QString m_displayName;
    QString m_userName;
    QString m_avatarPath;
    QString m_windowsVersion;
    QString m_hostName;
};
