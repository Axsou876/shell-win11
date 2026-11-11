#include "weatherservice.hpp"

#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QVariantMap>

namespace {
struct WeatherCode {
    QString icon;
    QString description;
};

WeatherCode weatherCode(int code)
{
    switch (code) {
    case 0:
        return { QStringLiteral("clear_day"), QStringLiteral("Ciel dégagé") };
    case 1:
        return { QStringLiteral("partly_cloudy_day"), QStringLiteral("Plutôt dégagé") };
    case 2:
        return { QStringLiteral("partly_cloudy_day"), QStringLiteral("Partiellement nuageux") };
    case 3:
        return { QStringLiteral("cloud"), QStringLiteral("Couvert") };
    case 45:
    case 48:
        return { QStringLiteral("foggy"), QStringLiteral("Brouillard") };
    case 51:
    case 53:
    case 55:
        return { QStringLiteral("rainy"), QStringLiteral("Bruine") };
    case 56:
    case 57:
        return { QStringLiteral("rainy"), QStringLiteral("Bruine verglaçante") };
    case 61:
    case 63:
    case 65:
        return { QStringLiteral("rainy"), QStringLiteral("Pluie") };
    case 66:
    case 67:
        return { QStringLiteral("rainy"), QStringLiteral("Pluie verglaçante") };
    case 71:
    case 73:
    case 75:
    case 77:
        return { QStringLiteral("weather_snowy"), QStringLiteral("Neige") };
    case 80:
    case 81:
    case 82:
        return { QStringLiteral("rainy"), QStringLiteral("Averses") };
    case 85:
    case 86:
        return { QStringLiteral("weather_snowy"), QStringLiteral("Averses de neige") };
    case 95:
        return { QStringLiteral("thunderstorm"), QStringLiteral("Orage") };
    case 96:
    case 99:
        return { QStringLiteral("thunderstorm"), QStringLiteral("Orage avec grêle") };
    default:
        return { QStringLiteral("cloud"), QStringLiteral("Conditions inconnues") };
    }
}

QString formatNumber(double value, int decimals = 0)
{
    return QString::number(value, 'f', decimals);
}
} // namespace

WeatherService::WeatherService(QObject* parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_refreshTimer(new QTimer(this))
{
    QSettings settings;
    m_city = settings.value(QStringLiteral("weather/city")).toString().trimmed();

    m_refreshTimer->setInterval(15 * 60 * 1000);
    connect(m_refreshTimer, &QTimer::timeout, this, &WeatherService::refresh);
    m_refreshTimer->start();

    if (!m_city.isEmpty())
        QTimer::singleShot(0, this, &WeatherService::refresh);
}

WeatherService::~WeatherService()
{
    if (m_reply)
        m_reply->abort();
}

void WeatherService::setLoading(bool loading)
{
    if (m_loading == loading)
        return;
    m_loading = loading;
    emit changed();
}

void WeatherService::refresh()
{
    if (m_city.isEmpty())
        return;
    requestGeocode();
}

void WeatherService::setCity(const QString& city)
{
    const QString trimmed = city.trimmed();
    if (m_city == trimmed)
        return;

    m_city = trimmed;
    QSettings settings;
    if (m_city.isEmpty())
        settings.remove(QStringLiteral("weather/city"));
    else
        settings.setValue(QStringLiteral("weather/city"), m_city);
    settings.sync();

    m_temperature.clear();
    m_description.clear();
    m_icon.clear();
    m_humidity.clear();
    m_feelsLike.clear();
    m_wind.clear();
    m_sunrise.clear();
    m_sunset.clear();
    m_forecast.clear();
    m_error.clear();
    m_available = false;
    m_updatedAt.clear();
    emit changed();

    if (!m_city.isEmpty())
        refresh();
}

void WeatherService::clearCity()
{
    setCity({});
}

void WeatherService::requestGeocode()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    setLoading(true);
    m_error.clear();

    QUrl url(QStringLiteral("https://geocoding-api.open-meteo.com/v1/search"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("name"), m_city);
    query.addQueryItem(QStringLiteral("count"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("language"), QStringLiteral("fr"));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
        QStringLiteral("CaelestiaShellWindows/0.3"));
    request.setTransferTimeout(12000);
    QNetworkReply* reply = m_network->get(request);
    m_reply = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply != m_reply) {
            reply->deleteLater();
            return;
        }
        m_reply = nullptr;
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            fail(QStringLiteral("Recherche de ville impossible : %1").arg(reply->errorString()));
            return;
        }

        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray results = document.object().value(QStringLiteral("results")).toArray();
        if (results.isEmpty()) {
            fail(QStringLiteral("Ville introuvable : %1").arg(m_city));
            return;
        }

        const QJsonObject result = results.first().toObject();
        const double latitude = result.value(QStringLiteral("latitude")).toDouble();
        const double longitude = result.value(QStringLiteral("longitude")).toDouble();
        const QString name = result.value(QStringLiteral("name")).toString(m_city);
        const QString country = result.value(QStringLiteral("country")).toString();
        requestForecast(latitude, longitude, country.isEmpty() ? name : name + ", " + country);
    });
}

void WeatherService::requestForecast(double latitude, double longitude,
    const QString& resolvedName)
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    QUrl url(QStringLiteral("https://api.open-meteo.com/v1/forecast"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("latitude"), formatNumber(latitude, 4));
    query.addQueryItem(QStringLiteral("longitude"), formatNumber(longitude, 4));
    query.addQueryItem(QStringLiteral("current"),
        QStringLiteral("temperature_2m,relative_humidity_2m,apparent_temperature,"
                       "weather_code,wind_speed_10m"));
    query.addQueryItem(QStringLiteral("daily"),
        QStringLiteral("weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset"));
    query.addQueryItem(QStringLiteral("timezone"), QStringLiteral("auto"));
    query.addQueryItem(QStringLiteral("forecast_days"), QStringLiteral("7"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
        QStringLiteral("CaelestiaShellWindows/0.3"));
    request.setTransferTimeout(12000);
    QNetworkReply* reply = m_network->get(request);
    m_reply = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply, resolvedName]() {
        if (reply != m_reply) {
            reply->deleteLater();
            return;
        }
        m_reply = nullptr;
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            fail(QStringLiteral("Météo indisponible : %1").arg(reply->errorString()));
            return;
        }

        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
        const QJsonObject root = document.object();
        const QJsonObject current = root.value(QStringLiteral("current")).toObject();
        if (current.isEmpty()) {
            fail(QStringLiteral("Réponse météo vide"));
            return;
        }

        const int code = current.value(QStringLiteral("weather_code")).toInt();
        const WeatherCode weather = weatherCode(code);
        m_temperature = formatNumber(current.value(QStringLiteral("temperature_2m")).toDouble(), 1)
            + QStringLiteral(" °C");
        m_description = weather.description;
        m_icon = weather.icon;
        m_humidity = formatNumber(
                        current.value(QStringLiteral("relative_humidity_2m")).toDouble())
            + QStringLiteral(" %");
        m_feelsLike = formatNumber(
                          current.value(QStringLiteral("apparent_temperature")).toDouble(), 1)
            + QStringLiteral(" °C");
        m_wind = formatNumber(current.value(QStringLiteral("wind_speed_10m")).toDouble(), 1)
            + QStringLiteral(" km/h");
        m_city = resolvedName;

        const QJsonObject daily = root.value(QStringLiteral("daily")).toObject();
        const QJsonArray dates = daily.value(QStringLiteral("time")).toArray();
        const QJsonArray codes = daily.value(QStringLiteral("weather_code")).toArray();
        const QJsonArray maxima = daily.value(QStringLiteral("temperature_2m_max")).toArray();
        const QJsonArray minima = daily.value(QStringLiteral("temperature_2m_min")).toArray();
        const QJsonArray sunrises = daily.value(QStringLiteral("sunrise")).toArray();
        const QJsonArray sunsets = daily.value(QStringLiteral("sunset")).toArray();

        QVariantList forecast;
        const QLocale french(QLocale::French, QLocale::France);
        for (int i = 0; i < dates.size(); ++i) {
            const QString dateText = dates.at(i).toString();
            const QDate date = QDate::fromString(dateText, Qt::ISODate);
            const WeatherCode day = weatherCode(codes.at(i).toInt());
            QVariantMap item;
            item.insert(QStringLiteral("date"), dateText);
            item.insert(QStringLiteral("day"),
                i == 0 ? QStringLiteral("Aujourd'hui") : french.toString(date, QStringLiteral("ddd")));
            item.insert(QStringLiteral("icon"), day.icon);
            item.insert(QStringLiteral("description"), day.description);
            item.insert(QStringLiteral("min"),
                formatNumber(minima.at(i).toDouble()) + QStringLiteral("°"));
            item.insert(QStringLiteral("max"),
                formatNumber(maxima.at(i).toDouble()) + QStringLiteral("°"));
            forecast.append(item);
        }
        m_forecast = forecast;

        if (!sunrises.isEmpty()) {
            const QString sunrise = sunrises.first().toString();
            m_sunrise = sunrise.mid(11, 5);
        }
        if (!sunsets.isEmpty()) {
            const QString sunset = sunsets.first().toString();
            m_sunset = sunset.mid(11, 5);
        }

        m_available = true;
        m_updatedAt = QLocale().toString(QDateTime::currentDateTime(), QStringLiteral("HH:mm"));
        m_error.clear();
        setLoading(false);
        emit changed();
    });
}

void WeatherService::fail(const QString& message)
{
    m_error = message;
    m_available = false;
    setLoading(false);
    emit changed();
}
