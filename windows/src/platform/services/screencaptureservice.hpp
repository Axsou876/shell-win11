#pragma once

#include <QHash>
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QRect>
#include <QVariantMap>

class QScreen;

// Frozen per-screen desktop captures for the area picker. Every capture is
// taken before the picker window is shown, so the overlay never photographs
// itself; regions are cropped out of that frozen frame afterwards.
//
// BitBlt from the screen DC is the documented Win32 desktop capture path.
// CAPTUREBLT is what makes layered windows — including this shell's own
// AppBar — part of the shot. Windows Graphics Capture would be the modern
// route, but it needs D3D11 frame-pool interop for no gain on a static grab.
class ScreenCaptureService final : public QObject {
    Q_OBJECT

    // Bumped on every capture and on clear(). QML uses it as the tail of the
    // provider URL so a second shot is never served from the pixmap cache.
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)

public:
    explicit ScreenCaptureService(QObject* parent = nullptr);

    [[nodiscard]] int revision() const { return m_revision; }
    [[nodiscard]] int count() const { return m_captures.size(); }
    [[nodiscard]] QString error() const { return m_error; }

    // Captures the monitor of `screen` and returns a token for the provider
    // and for the region calls, or 0 on failure (reason in `error`).
    Q_INVOKABLE int captureScreen(QScreen* screen);
    Q_INVOKABLE void clear();

    // x, y, width and height are in the picker window's QML coordinate space.
    // The picker stretches the frozen frame to its window, so viewWidth and
    // viewHeight (the window size in QML units) define the mapping; this stays
    // correct on mixed-DPI desktops where the window DPR is not the monitor's.
    Q_INVOKABLE bool copyRegion(int token, int x, int y, int width, int height,
        int viewWidth, int viewHeight);
    Q_INVOKABLE QString saveRegion(int token, int x, int y, int width, int height,
        int viewWidth, int viewHeight);

    // Root window under a point of the captured screen, in the same QML
    // coordinates. Used to snap the selection to a window while hovering, like
    // the Linux picker does. Empty map when the point is over the desktop or
    // one of our own surfaces.
    Q_INVOKABLE QVariantMap windowAt(int token, int x, int y, int viewWidth,
        int viewHeight);

    // Called by ScreenCaptureProvider, possibly from the image loading thread.
    [[nodiscard]] QImage imageForToken(int token) const;

signals:
    void changed();

private:
    struct Capture {
        QImage image;
        // Physical rectangle of the monitor on the virtual desktop.
        QRect physical;
    };

    [[nodiscard]] QImage crop(const Capture& capture, int x, int y, int width,
        int height, int viewWidth, int viewHeight) const;
    [[nodiscard]] QImage captureMonitor(QScreen* screen, Capture* capture) const;
    [[nodiscard]] bool findCapture(int token, Capture* capture) const;
    void setError(const QString& message);

    QHash<int, Capture> m_captures;
    int m_nextToken = 0;
    int m_revision = 0;
    QString m_error;
    mutable QMutex m_mutex;
};
