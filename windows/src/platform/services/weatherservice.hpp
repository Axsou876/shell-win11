#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

// Open-Meteo weather provider. It needs no API key and stores only the city
// name in QSettings. When no city is configured the service stays unavailable
// and the QML tab shows the setup form.
class WeatherService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool configured READ configured NOTIFY changed)
    Q_PROPERTY(bool loading READ loading NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString city READ city NOTIFY changed)
    Q_PROPERTY(QString temperature READ temperature NOTIFY changed)
    Q_PROPERTY(QString description READ description NOTIFY changed)
    Q_PROPERTY(QString icon READ icon NOTIFY changed)
    Q_PROPERTY(QString humidity READ humidity NOTIFY changed)
    Q_PROPERTY(QString wind READ wind NOTIFY changed)
    Q_PROPERTY(QString sunrise READ sunrise NOTIFY changed)
    Q_PROPERTY(QString sunset READ sunset NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QString updatedAt READ updatedAt NOTIFY changed)
    Q_PROPERTY(QVariantList forecast READ forecast NOTIFY changed)

public:
    explicit WeatherService(QObject* parent = nullptr);
    ~WeatherService() override;

    [[nodiscard]] bool configured() const { return !m_city.isEmpty(); }
    [[nodiscard]] bool loading() const { return m_loading; }
    [[nodiscard]] bool available() const { return m_available; }
    [[nodiscard]] QString city() const { return m_city; }
    [[nodiscard]] QString temperature() const { return m_temperature; }
    [[nodiscard]] QString description() const { return m_description; }
    [[nodiscard]] QString icon() const { return m_icon; }
    [[nodiscard]] QString humidity() const { return m_humidity; }
    [[nodiscard]] QString wind() const { return m_wind; }
    [[nodiscard]] QString sunrise() const { return m_sunrise; }
    [[nodiscard]] QString sunset() const { return m_sunset; }
    [[nodiscard]] QString error() const { return m_error; }
    [[nodiscard]] QString updatedAt() const { return m_updatedAt; }
    [[nodiscard]] QVariantList forecast() const { return m_forecast; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setCity(const QString& city);
    Q_INVOKABLE void clearCity();

signals:
    void changed();

private:
    void requestGeocode();
    void requestForecast(double latitude, double longitude, const QString& resolvedName);
    void setLoading(bool loading);
    void fail(const QString& message);
    void finish();

    QNetworkAccessManager* m_network = nullptr;
    QNetworkReply* m_reply = nullptr;
    QTimer* m_refreshTimer = nullptr;
    QString m_city;
    bool m_loading = false;
    bool m_available = false;
    QString m_temperature;
    QString m_description;
    QString m_icon;
    QString m_humidity;
    QString m_wind;
    QString m_sunrise;
    QString m_sunset;
    QString m_error;
    QString m_updatedAt;
    QVariantList m_forecast;
};
