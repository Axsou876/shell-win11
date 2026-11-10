#include "mediaservice.hpp"

#include <QDir>
#include <QImage>
#include <QMetaObject>
#include <QStandardPaths>

#include <chrono>
#include <vector>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Storage::Streams;

struct MediaService::Snapshot {
    bool available = false;
    bool playing = false;
    QString title;
    QString artist;
    QString album;
    QString sourceApp;
    QByteArray artwork;
    bool artworkChanged = false;
    bool canPlayPause = false;
    bool canNext = false;
    bool canPrevious = false;
    qint64 positionMs = 0;
    qint64 durationMs = 0;
};

namespace {
QString fromHString(const winrt::hstring& value)
{
    return QString::fromWCharArray(value.c_str());
}

QString artworkCachePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + QStringLiteral("/media");
    QDir().mkpath(dir);
    return dir;
}
} // namespace

MediaService::MediaService(QObject* parent)
    : QObject(parent)
{
    m_started = true;
    m_worker = std::thread([this]() { run(); });
}

MediaService::~MediaService()
{
    stop();
}

void MediaService::stop()
{
    if (!m_started.exchange(false))
        return;
    m_stop = true;
    m_cv.notify_all();
    if (m_worker.joinable())
        m_worker.join();
}

void MediaService::playPause()
{
    m_command = 1;
    m_cv.notify_all();
}

void MediaService::next()
{
    m_command = 2;
    m_cv.notify_all();
}

void MediaService::previous()
{
    m_command = 3;
    m_cv.notify_all();
}

void MediaService::refresh()
{
    m_refresh = true;
    m_cv.notify_all();
}

void MediaService::run()
{
    bool apartmentInitialized = false;
    GlobalSystemMediaTransportControlsSessionManager manager = nullptr;
    QString lastKey;
    QByteArray lastArtwork;
    Snapshot lastPublished;

    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        apartmentInitialized = true;
    } catch (const winrt::hresult_error&) {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                m_error = QStringLiteral("Windows Media Control indisponible");
                emit changed();
            },
            Qt::QueuedConnection);
        return;
    }

    while (!m_stop.load()) {
        try {
            if (!manager) {
                manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
            }

            const auto session = manager ? manager.GetCurrentSession() : nullptr;

            const int command = m_command.exchange(0);
            if (session && command != 0) {
                try {
                    if (command == 1) {
                        const bool playing = session.GetPlaybackInfo().PlaybackStatus()
                            == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
                        if (playing)
                            session.TryPauseAsync().get();
                        else
                            session.TryPlayAsync().get();
                    } else if (command == 2) {
                        session.TrySkipNextAsync().get();
                    } else if (command == 3) {
                        session.TrySkipPreviousAsync().get();
                    }
                } catch (const winrt::hresult_error&) {
                    // The app may have closed its session between the snapshot
                    // and the command; the next poll reports the empty state.
                }
            }

            Snapshot snapshot;
            if (session) {
                const auto properties = session.TryGetMediaPropertiesAsync().get();
                const auto playback = session.GetPlaybackInfo();
                const auto controls = playback.Controls();
                snapshot.available = !properties.Title().empty();
                snapshot.playing = playback.PlaybackStatus()
                    == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
                snapshot.title = fromHString(properties.Title());
                snapshot.artist = fromHString(properties.Artist());
                snapshot.album = fromHString(properties.AlbumTitle());
                snapshot.sourceApp = fromHString(session.SourceAppUserModelId());
                snapshot.canPlayPause = controls && controls.IsPlayPauseToggleEnabled();
                snapshot.canNext = controls && controls.IsNextEnabled();
                snapshot.canPrevious = controls && controls.IsPreviousEnabled();

                const auto timeline = session.GetTimelineProperties();
                const auto position = std::chrono::duration_cast<std::chrono::milliseconds>(
                    timeline.Position() - timeline.StartTime());
                const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    timeline.EndTime() - timeline.StartTime());
                snapshot.positionMs = qMax<qint64>(0, position.count());
                snapshot.durationMs = qMax<qint64>(0, duration.count());

                const QString key = snapshot.title + QChar(0x1f) + snapshot.artist
                    + QChar(0x1f) + snapshot.album + QChar(0x1f) + snapshot.sourceApp;
                if (!key.isEmpty() && key != lastKey) {
                    lastKey = key;
                    lastArtwork.clear();
                    snapshot.artworkChanged = true;
                    const auto thumbnail = properties.Thumbnail();
                    if (thumbnail) {
                        try {
                            const auto stream = thumbnail.OpenReadAsync().get();
                            const auto size = stream.Size();
                            if (size > 0) {
                                DataReader reader(stream.GetInputStreamAt(0));
                                reader.LoadAsync(static_cast<uint32_t>(size)).get();
                                std::vector<uint8_t> bytes(static_cast<size_t>(size));
                                reader.ReadBytes(winrt::array_view<uint8_t>(
                                    bytes.data(), bytes.data() + bytes.size()));
                                reader.Close();
                                lastArtwork = QByteArray(
                                    reinterpret_cast<const char*>(bytes.data()),
                                    static_cast<int>(bytes.size()));
                            }
                        } catch (const winrt::hresult_error&) {
                            lastArtwork.clear();
                        }
                    }
                }
                snapshot.artwork = lastArtwork;
            } else {
                lastKey.clear();
                lastArtwork.clear();
            }

            const bool changed = snapshot.available != lastPublished.available
                || snapshot.playing != lastPublished.playing
                || snapshot.title != lastPublished.title
                || snapshot.artist != lastPublished.artist
                || snapshot.album != lastPublished.album
                || snapshot.sourceApp != lastPublished.sourceApp
                || snapshot.canPlayPause != lastPublished.canPlayPause
                || snapshot.canNext != lastPublished.canNext
                || snapshot.canPrevious != lastPublished.canPrevious
                || snapshot.positionMs != lastPublished.positionMs
                || snapshot.durationMs != lastPublished.durationMs
                || snapshot.artworkChanged || m_refresh.exchange(false);

            if (changed) {
                lastPublished = snapshot;
                MediaService::Snapshot copy = snapshot;
                QMetaObject::invokeMethod(
                    this, [this, copy]() { applySnapshot(copy); }, Qt::QueuedConnection);
            }
        } catch (const winrt::hresult_error& error) {
            const QString message = QString::fromWCharArray(error.message().c_str());
            QMetaObject::invokeMethod(
                this,
                [this, message]() {
                    if (m_error == message)
                        return;
                    m_error = message;
                    m_available = false;
                    emit changed();
                },
                Qt::QueuedConnection);
        }

        std::unique_lock lock(m_mutex);
        m_cv.wait_for(lock, std::chrono::seconds(1), [this]() {
            return m_stop.load() || m_command.load() != 0 || m_refresh.load();
        });
    }

    if (apartmentInitialized)
        winrt::uninit_apartment();
}

void MediaService::applySnapshot(const Snapshot& snapshot)
{
    m_available = snapshot.available;
    m_playing = snapshot.playing;
    m_title = snapshot.title;
    m_artist = snapshot.artist;
    m_album = snapshot.album;
    m_sourceApp = snapshot.sourceApp;
    m_canPlayPause = snapshot.canPlayPause;
    m_canNext = snapshot.canNext;
    m_canPrevious = snapshot.canPrevious;
    m_positionMs = snapshot.positionMs;
    m_durationMs = snapshot.durationMs;
    m_error.clear();

    if (snapshot.artworkChanged) {
        const QImage image = QImage::fromData(snapshot.artwork);
        if (!image.isNull()) {
            const QString path = artworkCachePath()
                + QStringLiteral("/artwork-%1.png").arg(++m_artworkSerial);
            if (image.save(path, "PNG")) {
                QDir().remove(m_artworkPath);
                m_artworkPath = path;
            }
        } else {
            QDir().remove(m_artworkPath);
            m_artworkPath.clear();
        }
    } else if (!m_available) {
        QDir().remove(m_artworkPath);
        m_artworkPath.clear();
    }

    emit changed();
}
