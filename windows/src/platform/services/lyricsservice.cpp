#include "lyricsservice.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

#include <algorithm>

namespace {

// LRC lines look like "[01:23.45] text", sometimes with several timestamps in
// front of the same words (repeated choruses). Metadata tags such as
// "[ar:…]" and "[offset:…]" share the bracket syntax and must be skipped.
const QRegularExpression& stampPattern()
{
    static const QRegularExpression pattern(
        QStringLiteral("\\[(\\d{1,3}):(\\d{1,2}(?:[.,]\\d{1,3})?)\\]"));
    return pattern;
}

int lrcOffsetMs(const QString& text)
{
    static const QRegularExpression offsetPattern(
        QStringLiteral("\\[offset:\\s*([+-]?\\d+)\\]"),
        QRegularExpression::CaseInsensitiveOption);
    const auto match = offsetPattern.match(text);
    if (!match.hasMatch())
        return 0;
    return match.captured(1).toInt();
}

QVariantList parseLrc(const QString& text, int offsetMs)
{
    QVariantList out;
    const QStringList rows = text.split(QLatin1Char('\n'));
    for (const QString& raw : rows) {
        const QString row = raw.trimmed();
        if (row.isEmpty())
            continue;

        auto it = stampPattern().globalMatch(row);
        int consumed = -1;
        QList<qint64> times;
        while (it.hasNext()) {
            const auto match = it.next();
            const qint64 minutes = match.captured(1).toLongLong();
            const qint64 seconds = static_cast<qint64>(
                match.captured(2).replace(QLatin1Char(','), QLatin1Char('.')).toDouble()
                * 1000.0);
            times.append(minutes * 60000 + seconds);
            consumed = match.capturedEnd();
        }

        if (times.isEmpty())
            continue; // metadata or a stray line

        QString content = row.mid(consumed).trimmed();
        if (content.isEmpty())
            content = QStringLiteral("♪");

        for (const qint64 time : times) {
            QVariantMap line;
            line.insert(QStringLiteral("time"), static_cast<int>(qMax<qint64>(0, time + offsetMs)));
            line.insert(QStringLiteral("text"), content);
            out.append(line);
        }
    }

    std::sort(out.begin(), out.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value(QStringLiteral("time")).toInt()
            < b.toMap().value(QStringLiteral("time")).toInt();
    });
    return out;
}

QStringList splitPlain(const QString& text)
{
    QStringList out;
    const QStringList rows = text.split(QLatin1Char('\n'));
    for (const QString& row : rows) {
        const QString line = row.trimmed();
        if (!line.isEmpty())
            out.append(line);
    }
    return out;
}

} // namespace

LyricsService::LyricsService(QObject* parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

LyricsService::~LyricsService()
{
    abortReply();
}

void LyricsService::abortReply()
{
    if (!m_reply)
        return;
    QNetworkReply* reply = m_reply;
    m_reply = nullptr;
    reply->disconnect(this);
    reply->abort();
    reply->deleteLater();
}

void LyricsService::resetFields()
{
    m_synced = false;
    m_lines.clear();
    m_plainLines.clear();
    m_instrumental = false;
    m_error.clear();
}

void LyricsService::clear()
{
    abortReply();
    m_searching = false;
    m_key.clear();
    resetFields();
    m_status = QStringLiteral("idle");
    emit changed();
}

void LyricsService::request(const QString& title, const QString& artist,
    const QString& album, qint64 durationMs)
{
    const QString cleanTitle = title.trimmed();
    const QString cleanArtist = artist.trimmed();
    if (cleanTitle.isEmpty()) {
        if (!m_key.isEmpty())
            clear();
        return;
    }

    // The duration is part of the key because the same title can exist as a
    // studio take and a live version; lrclib matches on it too.
    const QString key = cleanArtist + QChar(0x1f) + cleanTitle + QChar(0x1f)
        + QString::number(durationMs / 1000);
    if (key == m_key)
        return;
    m_key = key;
    m_album = album.trimmed();
    m_durationMs = durationMs;

    if (const auto cached = m_cache.constFind(key); cached != m_cache.constEnd()) {
        abortReply();
        m_searching = false;
        applyEntry(*cached, key);
        return;
    }

    abortReply();
    m_searching = false;
    resetFields();
    m_status = QStringLiteral("loading");
    emit changed();

    QList<QPair<QString, QString>> params;
    params.append({ QStringLiteral("track_name"), cleanTitle });
    params.append({ QStringLiteral("artist_name"), cleanArtist });
    if (!m_album.isEmpty())
        params.append({ QStringLiteral("album_name"), m_album });
    if (durationMs > 0)
        params.append({ QStringLiteral("duration"),
            QString::number(qMax<qint64>(1, durationMs / 1000)) });
    get(QStringLiteral("/api/get"), params);
}

void LyricsService::get(const QString& path, const QList<QPair<QString, QString>>& params)
{
    QUrl url(QStringLiteral("https://lrclib.net") + path);
    QUrlQuery query;
    for (const auto& [name, value] : params)
        query.addQueryItem(name, value);
    url.setQuery(query);

    QNetworkRequest request(url);
    // lrclib asks clients to identify themselves so they can rate-limit fairly.
    request.setHeader(QNetworkRequest::UserAgentHeader,
        QStringLiteral("CaelestiaShellWindows/0.3 (https://github.com/caelestia)"));
    request.setRawHeader("Lrclib-Client", "CaelestiaShellWindows/0.3");
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

        const QByteArray body = reply->readAll();
        const int httpStatus =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool notFound = httpStatus == 404;
        // A transport failure (no HTTP status at all) is not "no lyrics": the
        // tab must say the lookup failed, not that the track has no words.
        if (reply->error() != QNetworkReply::NoError && httpStatus == 0 && !notFound) {
            m_searching = false;
            publish(QStringLiteral("error"), reply->errorString());
            return;
        }

        if (m_searching)
            handleSearchReply(body);
        else
            handleGetReply(body, notFound);
    });
}

void LyricsService::handleGetReply(const QByteArray& body, bool notFound)
{
    if (!notFound && !body.isEmpty()) {
        const QJsonDocument document = QJsonDocument::fromJson(body);
        if (document.isObject()) {
            applyTrackObject(document.object());
            return;
        }
    }

    // No exact match: the search endpoint also does fuzzy title matching, so a
    // remaster or a "feat." suffix still finds the words.
    m_searching = true;
    QList<QPair<QString, QString>> params;
    const QStringList parts = m_key.split(QChar(0x1f));
    if (parts.size() >= 2) {
        if (!parts.at(0).isEmpty())
            params.append({ QStringLiteral("artist_name"), parts.at(0) });
        params.append({ QStringLiteral("track_name"), parts.at(1) });
    }
    get(QStringLiteral("/api/search"), params);
}

void LyricsService::handleSearchReply(const QByteArray& body)
{
    m_searching = false;
    const QJsonDocument document = QJsonDocument::fromJson(body);
    if (document.isArray()) {
        const QJsonArray results = document.array();
        // Prefer a synced entry, then a plain one.
        for (const QJsonValue& value : results) {
            const QJsonObject object = value.toObject();
            if (!object.value(QStringLiteral("syncedLyrics")).toString().isEmpty()) {
                applyTrackObject(object);
                return;
            }
        }
        if (!results.isEmpty()) {
            applyTrackObject(results.first().toObject());
            return;
        }
    }
    publish(QStringLiteral("none"));
}

LyricsService::Entry LyricsService::parseEntry(const QJsonObject& object)
{
    Entry entry;
    entry.instrumental = object.value(QStringLiteral("instrumental")).toBool();

    const QString synced = object.value(QStringLiteral("syncedLyrics")).toString();
    if (!synced.isEmpty()) {
        entry.lines = parseLrc(synced, lrcOffsetMs(synced));
        entry.synced = !entry.lines.isEmpty();
    }

    if (!entry.synced) {
        const QString plain = object.value(QStringLiteral("plainLyrics")).toString();
        entry.plainLines = splitPlain(plain);
    }

    if (entry.synced || !entry.plainLines.isEmpty()) {
        entry.status = QStringLiteral("ready");
    } else {
        entry.status = entry.instrumental ? QStringLiteral("ready")
                                          : QStringLiteral("none");
    }
    return entry;
}

void LyricsService::applyTrackObject(const QJsonObject& object)
{
    applyEntry(parseEntry(object), m_key);
}

void LyricsService::applyEntry(const Entry& entry, const QString& key)
{
    m_status = entry.status;
    m_synced = entry.synced;
    m_lines = entry.lines;
    m_plainLines = entry.plainLines;
    m_instrumental = entry.instrumental;
    m_error = entry.error;

    if (m_cache.size() > 16)
        m_cache.clear();
    m_cache.insert(key, entry);

    emit changed();
}

void LyricsService::publish(const QString& status, const QString& error)
{
    m_status = status;
    m_error = error;
    if (status != QStringLiteral("ready")) {
        m_synced = false;
        m_lines.clear();
        m_plainLines.clear();
        m_instrumental = false;
    }
    emit changed();
}
