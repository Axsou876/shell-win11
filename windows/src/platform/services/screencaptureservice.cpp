#include "screencaptureservice.hpp"

#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QGuiApplication>
#include <QMutexLocker>
#include <QScreen>
#include <QStandardPaths>

#include <windows.h>
#include <dwmapi.h>

namespace {
// GDI capture of one monitor rectangle. The screen DC covers the whole virtual
// desktop, so rcMonitor doubles as the source offset.
QImage captureMonitorRect(const RECT& rect, qreal* scale)
{
    const int width = static_cast<int>(rect.right - rect.left);
    const int height = static_cast<int>(rect.bottom - rect.top);
    if (width <= 0 || height <= 0)
        return QImage();

    const HDC screen = GetDC(nullptr);
    if (!screen)
        return QImage();
    const HDC memory = CreateCompatibleDC(screen);
    const HBITMAP bitmap = CreateCompatibleBitmap(screen, width, height);
    if (!memory || !bitmap) {
        if (bitmap)
            DeleteObject(bitmap);
        if (memory)
            DeleteDC(memory);
        ReleaseDC(nullptr, screen);
        return QImage();
    }

    const HGDIOBJ previous = SelectObject(memory, bitmap);
    // CAPTUREBLT includes layered windows; SRCCOPY alone leaves holes where
    // the shell's own bar and other DWM-composited surfaces are.
    const BOOL copied = BitBlt(memory, 0, 0, width, height, screen, rect.left, rect.top,
        SRCCOPY | CAPTUREBLT);
    // GetDIBits refuses a bitmap that is still selected into a DC.
    SelectObject(memory, previous);

    QImage image;
    if (copied) {
        BITMAPINFO info {};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = width;
        info.bmiHeader.biHeight = -height; // top-down rows
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;

        image = QImage(width, height, QImage::Format_RGB32);
        if (GetDIBits(memory, bitmap, 0, static_cast<UINT>(height), image.bits(), &info,
                DIB_RGB_COLORS)
            == 0) {
            image = QImage();
        }
    }

    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);

    if (!image.isNull() && scale)
        image.setDevicePixelRatio(*scale);
    return image;
}

HMONITOR monitorForScreen(QScreen* screen)
{
    if (screen) {
        if (auto* native = screen->nativeInterface<QNativeInterface::QWindowsScreen>())
            return native->handle();
    }
    return nullptr;
}

// Shared state for the EnumWindows hit test below.
struct HitTest {
    POINT point {};
    DWORD selfPid = 0;
    RECT result {};
    bool found = false;
};

// Windows that make up the desktop itself. They sit at the bottom of the
// z-order, but they still contain the point when no real window does.
bool isDesktopWindow(HWND hwnd)
{
    wchar_t name[64] = {};
    if (GetClassNameW(hwnd, name, static_cast<int>(sizeof(name) / sizeof(name[0])))
        == 0)
        return true;
    return lstrcmpW(name, L"Progman") == 0 || lstrcmpW(name, L"WorkerW") == 0
        || lstrcmpW(name, L"Shell_TrayWnd") == 0
        || lstrcmpW(name, L"Shell_SecondaryTrayWnd") == 0;
}

// EnumWindows walks the top-level windows from top to bottom, which is exactly
// the order the picker wants. WindowFromPoint cannot be used here: the picker
// window itself covers the screen, so the point always lands on our own
// surface. Skipping our process leaves the window that is visually underneath
// the frozen frame.
BOOL CALLBACK pickTopLevelWindow(HWND hwnd, LPARAM parameter)
{
    auto* test = reinterpret_cast<HitTest*>(parameter);

    if (!IsWindowVisible(hwnd) || IsIconic(hwnd) || isDesktopWindow(hwnd))
        return TRUE;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == test->selfPid)
        return TRUE;

    // Cloaked windows belong to another virtual desktop or to a suspended
    // UWP app; they must not win the hit test.
    DWORD cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked,
            sizeof(cloaked)))
        && cloaked)
        return TRUE;

    // The extended frame bounds exclude the invisible resize border that
    // GetWindowRect includes, so the selection matches what the user sees.
    RECT rect {};
    if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect,
            sizeof(rect)))) {
        if (!GetWindowRect(hwnd, &rect))
            return TRUE;
    }

    if (!PtInRect(&rect, test->point))
        return TRUE;

    if (rect.right - rect.left < 8 || rect.bottom - rect.top < 8)
        return TRUE;

    test->result = rect;
    test->found = true;
    return FALSE;
}
} // namespace

ScreenCaptureService::ScreenCaptureService(QObject* parent)
    : QObject(parent)
{
}

void ScreenCaptureService::setError(const QString& message)
{
    if (m_error == message)
        return;
    m_error = message;
    emit changed();
}

QImage ScreenCaptureService::captureMonitor(QScreen* screen, Capture* capture) const
{
    if (!screen || !capture)
        return QImage();

    const HMONITOR monitor = monitorForScreen(screen);
    MONITORINFO info {};
    info.cbSize = sizeof(info);
    if (!monitor || !GetMonitorInfoW(monitor, &info))
        return QImage();

    const QRect logical = screen->geometry();
    capture->physical = QRect(info.rcMonitor.left, info.rcMonitor.top,
        info.rcMonitor.right - info.rcMonitor.left,
        info.rcMonitor.bottom - info.rcMonitor.top);
    qreal scale = logical.width() > 0
        ? static_cast<qreal>(capture->physical.width()) / logical.width()
        : screen->devicePixelRatio();

    return captureMonitorRect(info.rcMonitor, &scale);
}

int ScreenCaptureService::captureScreen(QScreen* screen)
{
    Capture capture;
    capture.image = captureMonitor(screen, &capture);
    if (capture.image.isNull()) {
        setError(QStringLiteral(
            "Capture d'écran impossible : le bureau n'expose pas son contenu."));
        return 0;
    }

    const int token = ++m_nextToken;
    {
        QMutexLocker lock(&m_mutex);
        m_captures.insert(token, capture);
    }
    ++m_revision;
    if (!m_error.isEmpty()) {
        m_error.clear();
    }
    emit changed();
    return token;
}

void ScreenCaptureService::clear()
{
    {
        QMutexLocker lock(&m_mutex);
        m_captures.clear();
    }
    ++m_revision;
    if (!m_error.isEmpty())
        m_error.clear();
    emit changed();
}

QImage ScreenCaptureService::imageForToken(int token) const
{
    QMutexLocker lock(&m_mutex);
    return m_captures.value(token).image;
}

bool ScreenCaptureService::findCapture(int token, Capture* capture) const
{
    if (!capture)
        return false;
    QMutexLocker lock(&m_mutex);
    const auto it = m_captures.constFind(token);
    if (it == m_captures.constEnd())
        return false;
    *capture = it.value();
    return true;
}

QVariantMap ScreenCaptureService::windowAt(int token, int x, int y, int viewWidth,
    int viewHeight)
{
    Capture capture;
    if (!findCapture(token, &capture) || capture.image.isNull() || viewWidth <= 0
        || viewHeight <= 0)
        return {};

    const qreal scaleX = static_cast<qreal>(capture.image.width()) / viewWidth;
    const qreal scaleY = static_cast<qreal>(capture.image.height()) / viewHeight;

    // windowAt expects virtual-desktop physical coordinates.
    HitTest test;
    test.point = POINT {
        capture.physical.x() + qRound(x * scaleX),
        capture.physical.y() + qRound(y * scaleY),
    };
    test.selfPid = GetCurrentProcessId();
    EnumWindows(pickTopLevelWindow, reinterpret_cast<LPARAM>(&test));
    if (!test.found)
        return {};

    const RECT& rect = test.result;
    const int width = static_cast<int>(rect.right - rect.left);
    const int height = static_cast<int>(rect.bottom - rect.top);

    QVariantMap result;
    result.insert(QStringLiteral("valid"), true);
    result.insert(QStringLiteral("x"),
        qRound((rect.left - capture.physical.x()) / scaleX));
    result.insert(QStringLiteral("y"),
        qRound((rect.top - capture.physical.y()) / scaleY));
    result.insert(QStringLiteral("width"), qRound(width / scaleX));
    result.insert(QStringLiteral("height"), qRound(height / scaleY));
    return result;
}

QImage ScreenCaptureService::crop(const Capture& capture, int x, int y, int width,
    int height, int viewWidth, int viewHeight) const
{
    if (capture.image.isNull() || width <= 0 || height <= 0 || viewWidth <= 0
        || viewHeight <= 0)
        return QImage();

    const qreal scaleX = static_cast<qreal>(capture.image.width()) / viewWidth;
    const qreal scaleY = static_cast<qreal>(capture.image.height()) / viewHeight;
    const QRect physical(qRound(x * scaleX), qRound(y * scaleY),
        qRound(width * scaleX), qRound(height * scaleY));
    const QRect available = physical.intersected(capture.image.rect());
    if (available.width() <= 0 || available.height() <= 0)
        return QImage();
    return capture.image.copy(available);
}

bool ScreenCaptureService::copyRegion(int token, int x, int y, int width, int height,
    int viewWidth, int viewHeight)
{
    Capture capture;
    if (!findCapture(token, &capture))
        return false;

    const QImage region = crop(capture, x, y, width, height, viewWidth, viewHeight);
    if (region.isNull()) {
        setError(QStringLiteral("La zone sélectionnée est vide."));
        return false;
    }

    QClipboard* clipboard = QGuiApplication::clipboard();
    if (!clipboard) {
        setError(QStringLiteral("Presse-papiers indisponible."));
        return false;
    }
    clipboard->setImage(region);
    if (!m_error.isEmpty())
        m_error.clear();
    return true;
}

QString ScreenCaptureService::saveRegion(int token, int x, int y, int width,
    int height, int viewWidth, int viewHeight)
{
    Capture capture;
    if (!findCapture(token, &capture))
        return QString();

    const QImage region = crop(capture, x, y, width, height, viewWidth, viewHeight);
    if (region.isNull()) {
        setError(QStringLiteral("La zone sélectionnée est vide."));
        return QString();
    }

    const QString folder = QStandardPaths::writableLocation(
        QStandardPaths::PicturesLocation)
        + QStringLiteral("/Screenshots");
    if (!QDir().mkpath(folder)) {
        setError(QStringLiteral("Impossible de créer le dossier de captures."));
        return QString();
    }
    const QString path = folder + QStringLiteral("/Capture ")
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH-mm-ss"))
        + QStringLiteral(".png");
    if (!region.save(path, "PNG")) {
        setError(QStringLiteral("Impossible d'enregistrer la capture."));
        return QString();
    }
    if (!m_error.isEmpty())
        m_error.clear();
    return path;
}
