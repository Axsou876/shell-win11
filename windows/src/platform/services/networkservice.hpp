#pragma once

#include <QObject>
#include <QString>

class NetworkService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString connectionType READ connectionType NOTIFY changed)
    Q_PROPERTY(QString networkName READ networkName NOTIFY changed)
    Q_PROPERTY(int signalStrength READ signalStrength NOTIFY changed)
    Q_PROPERTY(bool connected READ connected NOTIFY changed)
    Q_PROPERTY(bool known READ known NOTIFY changed)
    Q_PROPERTY(bool wifiConnected READ wifiConnected NOTIFY changed)
    Q_PROPERTY(QString wifiSsid READ wifiSsid NOTIFY changed)
    Q_PROPERTY(int wifiSignal READ wifiSignal NOTIFY changed)
    Q_PROPERTY(bool ethernetConnected READ ethernetConnected NOTIFY changed)
    Q_PROPERTY(QString ethernetName READ ethernetName NOTIFY changed)

public:
    explicit NetworkService(QObject* parent = nullptr);
    ~NetworkService() override;

    [[nodiscard]] QString connectionType() const;
    [[nodiscard]] QString networkName() const;
    [[nodiscard]] int signalStrength() const;
    [[nodiscard]] bool connected() const;
    [[nodiscard]] bool known() const;
    [[nodiscard]] bool wifiConnected() const;
    [[nodiscard]] QString wifiSsid() const;
    [[nodiscard]] int wifiSignal() const;
    [[nodiscard]] bool ethernetConnected() const;
    [[nodiscard]] QString ethernetName() const;

    void scheduleRefresh();

signals:
    void changed();

private:
    void refresh();
    void setState(const QString& connectionType, const QString& networkName, int signalStrength,
        bool connected, bool known, bool wifiConnected, const QString& wifiSsid, int wifiSignal,
        bool ethernetConnected, const QString& ethernetName);
    void stopNotifications();

    void* m_wlanHandle = nullptr;
    void* m_ipChangeHandle = nullptr;
    bool m_refreshPending = false;
    QString m_connectionType = QStringLiteral("unavailable");
    QString m_networkName;
    int m_signalStrength = 0;
    bool m_connected = false;
    bool m_known = false;
    bool m_wifiConnected = false;
    QString m_wifiSsid;
    int m_wifiSignal = 0;
    bool m_ethernetConnected = false;
    QString m_ethernetName;
};
