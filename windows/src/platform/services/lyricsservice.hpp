#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

class QJsonObject;
class QNetworkAccessManager;
class QNetworkReply;

// Lyrics for the track the media service is playing, from lrclib.net.
//
// The API needs no key and no account: the exact-match endpoint takes the
// artist, the title, the album and the duration, and returns synced LRC or a
// plain text. Nothing is cached on disk — the service keeps a small in-memory
// cache so scrubbing back and forth does not hit the network again — and the
// request is only made when the track actually changes.
class LyricsService final : public QObject {
    Q_OBJECT

    // idle | loading | ready | none | error
    Q_PROPERTY(QString status READ status NOTIFY changed)
    // True when `lines` carries timestamps, so the view can follow along.
    Q_PROPERTY(bool synced READ synced NOTIFY changed)
    // [{ "time": ms (int), "text": string }, …] sorted by time.
    Q_PROPERTY(QVariantList lines READ lines NOTIFY changed)
    // Untimed lyrics, one entry per line. Empty when `synced` is true.
    Q_PROPERTY(QStringList plainLines READ plainLines NOTIFY changed)
    Q_PROPERTY(bool instrumental READ instrumental NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)

public:
    explicit LyricsService(QObject* parent = nullptr);
    ~LyricsService() override;

    [[nodiscard]] QString status() const { return m_status; }
    [[nodiscard]] bool synced() const { return m_synced; }
    [[nodiscard]] QVariantList lines() const { return m_lines; }
    [[nodiscard]] QStringList plainLines() const { return m_plainLines; }
    [[nodiscard]] bool instrumental() const { return m_instrumental; }
    [[nodiscard]] QString error() const { return m_error; }

    // Identifies the track. Calling it again with the same arguments is a
    // no-op, which is what lets the QML side call it on every signal.
    Q_INVOKABLE void request(const QString& title, const QString& artist,
        const QString& album, qint64 durationMs);
    Q_INVOKABLE void clear();

signals:
    void changed();

private:
    struct Entry {
        QString status;
        bool synced = false;
        QVariantList lines;
        QStringList plainLines;
        bool instrumental = false;
        QString error;
    };

    void abortReply();
    void get(const QString& path, const QList<QPair<QString, QString>>& params);
    void handleGetReply(const QByteArray& body, bool notFound);
    void handleSearchReply(const QByteArray& body);
    void applyTrackObject(const QJsonObject& object);
    void applyEntry(const Entry& entry, const QString& key);
    void publish(const QString& status, const QString& error = {});
    void resetFields();

    static Entry parseEntry(const QJsonObject& object);

    QNetworkAccessManager* m_network = nullptr;
    QNetworkReply* m_reply = nullptr;
    // Set while the search fallback runs, so a 404 there does not loop.
    bool m_searching = false;
    QString m_key;
    QString m_album;
    qint64 m_durationMs = 0;

    QHash<QString, Entry> m_cache;

    QString m_status = QStringLiteral("idle");
    bool m_synced = false;
    QVariantList m_lines;
    QStringList m_plainLines;
    bool m_instrumental = false;
    QString m_error;
};
