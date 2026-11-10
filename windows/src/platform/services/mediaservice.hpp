#pragma once

#include <QObject>
#include <QString>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

// Windows Global System Media Transport Controls (GSMTC). The worker owns the
// WinRT session manager and polls once per second; the GUI only receives
// plain snapshots. Playback commands are posted to the same worker.
class MediaService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool playing READ playing NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString artist READ artist NOTIFY changed)
    Q_PROPERTY(QString album READ album NOTIFY changed)
    Q_PROPERTY(QString sourceApp READ sourceApp NOTIFY changed)
    Q_PROPERTY(QString artworkPath READ artworkPath NOTIFY changed)
    Q_PROPERTY(qint64 positionMs READ positionMs NOTIFY changed)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY changed)
    Q_PROPERTY(bool canPlayPause READ canPlayPause NOTIFY changed)
    Q_PROPERTY(bool canNext READ canNext NOTIFY changed)
    Q_PROPERTY(bool canPrevious READ canPrevious NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)

public:
    explicit MediaService(QObject* parent = nullptr);
    ~MediaService() override;

    [[nodiscard]] bool available() const { return m_available; }
    [[nodiscard]] bool playing() const { return m_playing; }
    [[nodiscard]] QString title() const { return m_title; }
    [[nodiscard]] QString artist() const { return m_artist; }
    [[nodiscard]] QString album() const { return m_album; }
    [[nodiscard]] QString sourceApp() const { return m_sourceApp; }
    [[nodiscard]] QString artworkPath() const { return m_artworkPath; }
    [[nodiscard]] qint64 positionMs() const { return m_positionMs; }
    [[nodiscard]] qint64 durationMs() const { return m_durationMs; }
    [[nodiscard]] bool canPlayPause() const { return m_canPlayPause; }
    [[nodiscard]] bool canNext() const { return m_canNext; }
    [[nodiscard]] bool canPrevious() const { return m_canPrevious; }
    [[nodiscard]] QString error() const { return m_error; }

    Q_INVOKABLE void playPause();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void refresh();

    void stop();

signals:
    void changed();

private:
    struct Snapshot;
    void applySnapshot(const Snapshot& snapshot);
    void run();

    std::thread m_worker;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_stop { false };
    std::atomic<bool> m_started { false };
    std::atomic<int> m_command { 0 };
    std::atomic<bool> m_refresh { false };

    bool m_available = false;
    bool m_playing = false;
    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_sourceApp;
    QString m_artworkPath;
    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    bool m_canPlayPause = false;
    bool m_canNext = false;
    bool m_canPrevious = false;
    QString m_error;
    int m_artworkSerial = 0;
};
