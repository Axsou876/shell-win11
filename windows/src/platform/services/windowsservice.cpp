#include "windowsservice.hpp"

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMutexLocker>
#include <QTimer>
#include <QtConcurrent>

#include <algorithm>
#include <utility>

#include <windows.h>
#include <dwmapi.h>
#include <shobjidl.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace {
// One poll drives both the details and the preview: 700 ms is fast enough for
// the panel to follow a window swap without re-capturing at video rates.
constexpr int kFocusPollInterval = 700;
constexpr int kPreviewWidth = 512;

bool debugEnabled()
{
    return !qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_STATUS");
}

bool skipWindow(HWND hwnd)
{
    if (!IsWindowVisible(hwnd))
        return true;
    if (GetWindowTextLengthW(hwnd) == 0)
        return true;
    if (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
        return true;

    wchar_t className[128] = {};
    GetClassNameW(hwnd, className, 128);
    const QString name = QString::fromWCharArray(className);
    if (name == QStringLiteral("Progman") || name == QStringLiteral("WorkerW")
        || name == QStringLiteral("Shell_TrayWnd")
        || name == QStringLiteral("Shell_SecondaryTrayWnd")
        || name == QStringLiteral("Windows.UI.Core.CoreWindow")) {
        return true;
    }

    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))
        && cloaked) {
        return true;
    }
    return false;
}

QString processPath(HWND hwnd)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid)
        return QString();

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process)
        return QString();

    wchar_t path[32768] = {};
    DWORD size = 32768;
    QString result;
    if (QueryFullProcessImageNameW(process, 0, path, &size))
        result = QString::fromWCharArray(path);
    CloseHandle(process);
    return result;
}

QString processName(HWND hwnd)
{
    const QString path = processPath(hwnd);
    if (path.isEmpty())
        return QStringLiteral("Application");

    const QString name = QFileInfo(path).completeBaseName();
    return name.isEmpty() ? QStringLiteral("Application") : name;
}

// The extended frame bounds exclude the invisible resize border, which is what
// the user perceives as the window rectangle. DWM reports an empty rectangle
// for minimized windows, so fall back to the restored placement for those.
RECT windowFrame(HWND hwnd)
{
    RECT frame {};
    if (FAILED(DwmGetWindowAttribute(
            hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(frame)))) {
        frame = RECT {};
        GetWindowRect(hwnd, &frame);
    }

    if (IsIconic(hwnd)) {
        WINDOWPLACEMENT placement {};
        placement.length = sizeof(placement);
        if (GetWindowPlacement(hwnd, &placement))
            frame = placement.rcNormalPosition;
    }
    return frame;
}

QVariantMap describeWindow(HWND hwnd)
{
    const RECT frame = windowFrame(hwnd);
    const int width = std::max(0L, frame.right - frame.left);
    const int height = std::max(0L, frame.bottom - frame.top);

    wchar_t title[512] = {};
    GetWindowTextW(hwnd, title, sizeof(title) / sizeof(title[0]));
    wchar_t className[256] = {};
    GetClassNameW(hwnd, className, sizeof(className) / sizeof(className[0]));

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    MONITORINFOEXW monitor {};
    monitor.cbSize = sizeof(monitor);
    const HMONITOR monitorHandle = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    // MONITORINFOEXW only extends MONITORINFO with the device name.
    GetMonitorInfoW(monitorHandle, reinterpret_cast<LPMONITORINFO>(&monitor));
    // szDevice is "\\.\DISPLAY1"; the tail is the name Windows settings shows.
    const QString monitorName = QString::fromWCharArray(monitor.szDevice).section('\\', -1);

    const bool fullscreen = width > 0 && height > 0
        && frame.left <= monitor.rcMonitor.left && frame.top <= monitor.rcMonitor.top
        && frame.right >= monitor.rcMonitor.right
        && frame.bottom >= monitor.rcMonitor.bottom;

    QVariantMap details;
    details.insert(QStringLiteral("valid"), true);
    details.insert(QStringLiteral("id"),
        QString::number(reinterpret_cast<quintptr>(hwnd)));
    details.insert(QStringLiteral("title"),
        QString::fromWCharArray(title).simplified());
    details.insert(QStringLiteral("className"), QString::fromWCharArray(className));
    details.insert(QStringLiteral("app"), processName(hwnd));
    details.insert(QStringLiteral("path"), processPath(hwnd));
    details.insert(QStringLiteral("address"),
        QStringLiteral("0x%1").arg(reinterpret_cast<quintptr>(hwnd), 0, 16));
    details.insert(QStringLiteral("pid"), static_cast<int>(pid));
    details.insert(QStringLiteral("x"), static_cast<int>(frame.left));
    details.insert(QStringLiteral("y"), static_cast<int>(frame.top));
    details.insert(QStringLiteral("width"), width);
    details.insert(QStringLiteral("height"), height);
    details.insert(QStringLiteral("monitor"), monitorName);
    details.insert(QStringLiteral("dpi"), static_cast<int>(GetDpiForWindow(hwnd)));
    details.insert(QStringLiteral("maximized"), IsZoomed(hwnd) != FALSE);
    details.insert(QStringLiteral("minimized"), IsIconic(hwnd) != FALSE);
    details.insert(QStringLiteral("topmost"),
        (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0);
    details.insert(QStringLiteral("fullscreen"), fullscreen);
    return details;
}

// PrintWindow asks the window to render its own frame into a memory DC. The
// PW_RENDERFULLCONTENT flag routes it through DWM, which is what makes UWP
// apps and partly covered windows capture correctly.
QImage grabWindow(HWND hwnd)
{
    if (IsIconic(hwnd))
        return QImage();

    const RECT frame = windowFrame(hwnd);
    const int width = frame.right - frame.left;
    const int height = frame.bottom - frame.top;
    if (width <= 0 || height <= 0)
        return QImage();

    const HDC screen = GetDC(nullptr);
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
    const BOOL painted = PrintWindow(hwnd, memory, PW_RENDERFULLCONTENT);
    // GetDIBits refuses a bitmap that is still selected into a DC.
    SelectObject(memory, previous);

    QImage image;
    if (painted) {
        BITMAPINFO info {};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = width;
        info.bmiHeader.biHeight = -height; // top-down rows
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;

        image = QImage(width, height, QImage::Format_ARGB32);
        if (GetDIBits(memory, bitmap, 0, static_cast<UINT>(height), image.bits(), &info,
                DIB_RGB_COLORS)
            == 0) {
            image = QImage();
        }
    }

    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);

    if (image.isNull())
        return image;
    return image.scaledToWidth(std::min(kPreviewWidth, width), Qt::SmoothTransformation);
}
} // namespace

struct WindowService::Impl {
    ComPtr<IVirtualDesktopManager> desktopManager;
};

WindowService::WindowService(QObject* parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_focusedTimer(new QTimer(this))
    , m_impl(new Impl())
{
    CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_ALL,
        IID_PPV_ARGS(&m_impl->desktopManager));

    m_timer->setInterval(2000);
    connect(m_timer, &QTimer::timeout, this, &WindowService::refresh);
    m_timer->start();
    refresh();

    m_focusedTimer->setInterval(kFocusPollInterval);
    connect(m_focusedTimer, &QTimer::timeout, this, [this]() {
        refreshFocused();
        capturePreview();
    });
}

WindowService::~WindowService()
{
    delete m_impl;
}

void WindowService::refresh()
{
    QVariantList result;

    EnumWindows(
        [](HWND hwnd, LPARAM data) -> BOOL {
            auto* list = reinterpret_cast<QVariantList*>(data);
            if (skipWindow(hwnd))
                return TRUE;

            wchar_t title[512] = {};
            GetWindowTextW(hwnd, title, 512);
            const QString windowTitle = QString::fromWCharArray(title).simplified();
            if (windowTitle.isEmpty())
                return TRUE;

            QVariantMap item;
            item.insert(QStringLiteral("id"),
                QString::number(reinterpret_cast<quintptr>(hwnd)));
            item.insert(QStringLiteral("title"), windowTitle);
            item.insert(QStringLiteral("app"), processName(hwnd));
            item.insert(QStringLiteral("active"), GetForegroundWindow() == hwnd);
            list->append(item);
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&result));

    if (m_impl->desktopManager) {
        QVariantList filtered;
        for (const QVariant& entry : std::as_const(result)) {
            const QVariantMap item = entry.toMap();
            const HWND hwnd = reinterpret_cast<HWND>(
                item.value(QStringLiteral("id")).toString().toULongLong());
            BOOL onCurrent = TRUE;
            if (SUCCEEDED(m_impl->desktopManager->IsWindowOnCurrentVirtualDesktop(
                    hwnd, &onCurrent))
                && !onCurrent) {
                continue;
            }
            filtered.append(item);
        }
        result = filtered;
    }

    if (m_windows == result)
        return;
    m_windows = result;
    emit changed();
}

void WindowService::focusWindow(const QString& id)
{
    const HWND hwnd = reinterpret_cast<HWND>(id.toULongLong());
    if (!hwnd || !IsWindow(hwnd))
        return;
    if (IsIconic(hwnd))
        ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    refresh();
}

quintptr WindowService::focusedHwnd() const
{
    if (QDateTime::currentMSecsSinceEpoch() < m_pinnedUntil)
        return m_focusedHwnd;

    const HWND foreground = GetForegroundWindow();
    if (foreground) {
        DWORD pid = 0;
        GetWindowThreadProcessId(foreground, &pid);
        // Our own overlays hold the focus while a panel is open. Following
        // them would erase the very window the panel is describing.
        if (pid != GetCurrentProcessId() && IsWindowVisible(foreground)
            && !skipWindow(foreground)) {
            return reinterpret_cast<quintptr>(foreground);
        }
    }
    return m_focusedHwnd;
}

void WindowService::refreshFocused()
{
    const quintptr value = focusedHwnd();
    if (!value || !IsWindow(reinterpret_cast<HWND>(value))) {
        m_focusedHwnd = 0;
        if (m_focused.isEmpty())
            return;
        m_focused.clear();
        emit focusedChanged();
        return;
    }

    m_focusedHwnd = value;
    const QVariantMap details = describeWindow(reinterpret_cast<HWND>(value));
    if (details == m_focused)
        return;

    m_focused = details;
    emit focusedChanged();

    if (debugEnabled()) {
        qInfo().noquote() << "Focused window:" << details.value(QStringLiteral("title"))
                          << details.value(QStringLiteral("address"))
                          << "size=" << details.value(QStringLiteral("width"))
                          << "x" << details.value(QStringLiteral("height"));
    }
}

void WindowService::trackFocused(bool enabled, const QString& windowId)
{
    if (enabled) {
        // Opening the panel moves the focus to the shell, so a bare foreground
        // poll starts blind. Platform tracks foreground changes app-wide and
        // keeps the last external HWND; QML hands it over here.
        const quintptr seed = windowId.toULongLong();
        if (seed && IsWindow(reinterpret_cast<HWND>(seed)))
            m_focusedHwnd = seed;

        refreshFocused();
        capturePreview();
        m_focusedTimer->start();
        return;
    }

    m_focusedTimer->stop();
    m_pinnedUntil = 0;
    m_focusedHwnd = 0;

    if (!m_focused.isEmpty()) {
        m_focused.clear();
        emit focusedChanged();
    }

    {
        QMutexLocker lock(&m_previewMutex);
        m_preview = QImage();
    }
    ++m_previewRevision;
    emit previewChanged();
}

void WindowService::capturePreview()
{
    const HWND hwnd = reinterpret_cast<HWND>(m_focusedHwnd);
    if (!hwnd || !IsWindow(hwnd) || m_capturePending > 0)
        return;

    ++m_capturePending;

    // The capture can take tens of milliseconds on a large maximized window;
    // doing it on the GUI thread would stutter the panel animation.
    auto* watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher]() {
        const QImage image = watcher->result();
        watcher->deleteLater();
        --m_capturePending;
        if (image.isNull())
            return;

        {
            QMutexLocker lock(&m_previewMutex);
            m_preview = image;
        }
        ++m_previewRevision;
        emit previewChanged();
    });
    watcher->setFuture(QtConcurrent::run([hwnd]() { return grabWindow(hwnd); }));
}

QImage WindowService::previewImage() const
{
    QMutexLocker lock(&m_previewMutex);
    return m_preview;
}

void WindowService::runAction(const char* name, const std::function<bool(quintptr)>& action)
{
    const quintptr value = m_focusedHwnd;
    HWND hwnd = reinterpret_cast<HWND>(value);
    if (debugEnabled()) {
        qInfo().noquote() << "Window action:" << name << "hwnd="
                          << QString::number(value, 16);
    }
    if (!hwnd || !IsWindow(hwnd)) {
        setActionError(QStringLiteral("Aucune fenêtre suivie : l'action n'a pas de cible."));
        return;
    }

    if (!action(value)) {
        if (debugEnabled())
            qInfo().noquote() << "Window action failed:" << name
                              << QString::number(GetLastError());
        setActionError(QStringLiteral("Windows a refusé l'action sur cette fenêtre."));
        return;
    }

    setActionError(QString());
    // Window state settles asynchronously (a maximize animation has to run
    // first), so refresh twice instead of right away.
    QTimer::singleShot(200, this, &WindowService::refreshFocused);
    QTimer::singleShot(400, this, &WindowService::capturePreview);
}

void WindowService::minimizeFocused()
{
    runAction("minimize", [](quintptr value) {
        ShowWindow(reinterpret_cast<HWND>(value), SW_MINIMIZE);
        return true;
    });

    // Windows activates the next window of the z-order; following it would
    // make the panel jump away from the window it just minimized.
    m_pinnedUntil = QDateTime::currentMSecsSinceEpoch() + 1500;
}

void WindowService::closeFocused()
{
    // WM_CLOSE is the graceful path: the app still gets to show its
    // "unsaved changes" prompt instead of being terminated.
    runAction("close", [](quintptr value) {
        return PostMessageW(reinterpret_cast<HWND>(value), WM_CLOSE, 0, 0) != FALSE;
    });
}

void WindowService::setActionError(const QString& message)
{
    if (m_actionError == message)
        return;

    m_actionError = message;
    emit actionErrorChanged();

    if (message.isEmpty())
        return;

    const int token = ++m_errorToken;
    QTimer::singleShot(5000, this, [this, token]() {
        if (token != m_errorToken || m_actionError.isEmpty())
            return;
        m_actionError.clear();
        emit actionErrorChanged();
    });
}
