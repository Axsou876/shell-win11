#include "platform.hpp"

#include <QBuffer>
#include <QClipboard>
#include <QCursor>
#include <QDebug>
#include <QFileInfo>
#include <algorithm>
#include <QGuiApplication>
#include <QImage>
#include <QScopedValueRollback>
#include <QSet>
#include <QSettings>
#include <QStringList>
#include <QWindow>
#include <QtGui/qscreen_platform.h>

#include <string>

#include <windows.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shellscalingapi.h>

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

namespace {
Platform* g_activeWindowPlatform = nullptr;

constexpr int kEscapeHotkeyId = 0xCAFE;

void CALLBACK winEventCallback(
    HWINEVENTHOOK, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD)
{
    Platform* platform = g_activeWindowPlatform;
    if (!platform)
        return;

    QMetaObject::invokeMethod(
        platform,
        [platform, event, hwnd, idObject, idChild]() {
            if (g_activeWindowPlatform == platform)
                platform->handleWinEvent(static_cast<quint32>(event),
                    reinterpret_cast<quintptr>(hwnd), static_cast<qint32>(idObject),
                    static_cast<qint32>(idChild));
        },
        Qt::QueuedConnection);
}

bool isShellWindowClass(const QString& name)
{
    return name == QStringLiteral("Progman") || name == QStringLiteral("WorkerW")
        || name == QStringLiteral("Shell_TrayWnd") || name == QStringLiteral("Shell_SecondaryTrayWnd")
        || name == QStringLiteral("Windows.UI.Core.CoreWindow");
}

QList<HWND> taskbarWindows()
{
    QList<HWND> result;
    EnumWindows(
        [](HWND hwnd, LPARAM lParam) -> BOOL {
            wchar_t name[64] = {};
            if (GetClassNameW(hwnd, name, 64) <= 0)
                return TRUE;
            if (wcscmp(name, L"Shell_TrayWnd") != 0
                && wcscmp(name, L"Shell_SecondaryTrayWnd") != 0)
                return TRUE;
            reinterpret_cast<QList<HWND>*>(lParam)->append(hwnd);
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&result));
    return result;
}

// The desktop icons are a SysListView32 owned by Explorer, either directly
// under Progman or under one of the WorkerW windows depending on the build
// and on whether a slideshow wallpaper is active. Hiding that list view is
// what the "Show desktop icons" toggle does, and unlike deleting shortcuts it
// is undone by ShowWindow alone.
HWND desktopIconWindow()
{
    HWND defView = FindWindowExW(FindWindowW(L"Progman", nullptr), nullptr,
        L"SHELLDLL_DefView", nullptr);

    if (!defView) {
        EnumWindows(
            [](HWND top, LPARAM lParam) -> BOOL {
                HWND* out = reinterpret_cast<HWND*>(lParam);
                if (*out)
                    return FALSE;
                *out = FindWindowExW(top, nullptr, L"SHELLDLL_DefView", nullptr);
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&defView));
    }

    return defView ? FindWindowExW(defView, nullptr, L"SysListView32", nullptr) : nullptr;
}

QString readRegistryString(const QString& path, const QString& key)
{
    const QSettings settings(path, QSettings::NativeFormat);
    return settings.value(key).toString();
}

int readRegistryInt(const QString& path, const QString& key, int fallback)
{
    const QSettings settings(path, QSettings::NativeFormat);
    return settings.value(key, fallback).toInt();
}

UINT dpiForWindow(HWND hwnd)
{
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);

    if (const HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
        const auto getDpi = reinterpret_cast<GetDpiForWindowFn>(
            reinterpret_cast<void*>(GetProcAddress(user32, "GetDpiForWindow")));
        if (getDpi)
            return getDpi(hwnd);
    }

    return 96;
}

UINT dpiForMonitor(HMONITOR monitor, HWND hwnd)
{
    UINT dpiX = 0;
    UINT dpiY = 0;
    if (monitor
        && SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))
        && dpiX > 0)
        return dpiX;

    return dpiForWindow(hwnd);
}

HMONITOR monitorForScreen(QScreen* screen)
{
    if (screen) {
        if (auto* native = screen->nativeInterface<QNativeInterface::QWindowsScreen>())
            return native->handle();
    }

    return nullptr;
}
} // namespace

Platform::Platform(QObject* parent)
    : QObject(parent)
{
    m_windowsBuild = readRegistryInt(QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
        QStringLiteral("CurrentBuildNumber"), 0);

    refresh();

    // The shell replaces the Windows taskbar; auto-hide expands each monitor's
    // work area so the shell can fill it. The window itself is kept hidden
    // while the shell runs and the previous state is restored when it quits.
    m_taskbarTimer.setInterval(250);
    connect(&m_taskbarTimer, &QTimer::timeout, this, [this]() {
        enforceTaskbarHidden();
        enforceDesktopIconsHidden();
    });
    hideTaskbar();
    hideDesktopIcons();
    // Start regardless of the first attempt: Explorer may not have created
    // the taskbar or the icon list view yet at login, and the same timer
    // retries until both are found.
    m_taskbarTimer.start();
    QTimer::singleShot(1200, this, &Platform::refreshAppBar);

    m_refreshTimer.setInterval(1500);
    connect(&m_refreshTimer, &QTimer::timeout, this, &Platform::refresh);
    m_refreshTimer.start();

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        restoreTaskbar();
        restoreDesktopIcons();
        unregisterAppBar();
    });
    connect(qApp, &QCoreApplication::aboutToQuit, this, &Platform::stopActiveWindowTracking);
    if (auto* gui = qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        connect(gui, &QGuiApplication::screenAdded, this, &Platform::handleScreensChanged);
        connect(gui, &QGuiApplication::screenRemoved, this, &Platform::handleScreenRemoved);
        connect(gui, &QGuiApplication::primaryScreenChanged, this, &Platform::refreshAppBar);
        connect(gui, &QGuiApplication::primaryScreenChanged, this,
            [this]() { emit primaryScreenNameChanged(); });
    }

    if (qApp)
        qApp->installNativeEventFilter(this);

    startActiveWindowTracking();
}

Platform::~Platform()
{
    restoreTaskbar();
    restoreDesktopIcons();
    unregisterAppBar();
    stopActiveWindowTracking();
    if (qApp)
        qApp->removeNativeEventFilter(this);
}

QString Platform::name() const
{
    return QStringLiteral("Caelestia Shell for Windows");
}

QString Platform::version() const
{
    return QStringLiteral("0.2.5");
}

int Platform::windowsBuild() const
{
    return m_windowsBuild;
}

bool Platform::darkMode() const
{
    return m_darkMode;
}

QColor Platform::accentColor() const
{
    return m_accentColor;
}

QString Platform::wallpaperPath() const
{
    return m_wallpaperPath;
}

bool Platform::wallpaperAvailable() const
{
    return m_wallpaperAvailable;
}

bool Platform::animationsEnabled() const
{
    return m_animationsEnabled;
}

qreal Platform::systemScale() const
{
    return m_systemScale;
}

QString Platform::rhiBackend() const
{
    return m_rhiBackend;
}

bool Platform::appBarRegistered() const
{
    return !m_appBars.isEmpty();
}

int Platform::appBarWidth() const
{
    return m_appBarWidth;
}

int Platform::appBarHeight() const
{
    return m_appBarHeight;
}

QString Platform::activeWindowTitle() const
{
    return m_activeWindowTitle;
}

QString Platform::activeWindowIcon() const
{
    return m_activeWindowIcon;
}

QString Platform::activeWindowId() const
{
    return m_activeWindowHwnd ? QString::number(m_activeWindowHwnd) : QString();
}

QVariantList Platform::screens() const
{
    QList<QScreen*> ordered = QGuiApplication::screens();
    std::sort(ordered.begin(), ordered.end(), [](const QScreen* a, const QScreen* b) {
        const QPoint pa = a->geometry().topLeft();
        const QPoint pb = b->geometry().topLeft();
        if (pa.x() != pb.x())
            return pa.x() < pb.x();
        if (pa.y() != pb.y())
            return pa.y() < pb.y();
        return a->name() < b->name();
    });

    QVariantList result;
    QSet<QString> seen;
    result.reserve(ordered.size());
    for (auto* screen : ordered) {
        const QRect geometry = screen->geometry();
        const QString key = screen->name() + QLatin1Char('|') + QString::number(geometry.x())
            + QLatin1Char(',') + QString::number(geometry.y()) + QLatin1Char(',')
            + QString::number(geometry.width()) + QLatin1Char(',')
            + QString::number(geometry.height());
        if (seen.contains(key))
            continue;
        seen.insert(key);
        result.append(QVariant::fromValue(screen));
        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_APPBAR")) {
            qInfo().noquote() << "Screen:" << screen->name() << geometry << "dpr"
                              << screen->devicePixelRatio();
        }
    }
    return result;
}

QString Platform::primaryScreenName() const
{
    QScreen* primary = QGuiApplication::primaryScreen();
    return primary ? primary->name() : QString();
}

void Platform::setRhiBackend(const QString& backend)
{
    if (m_rhiBackend == backend)
        return;

    m_rhiBackend = backend;
    emit rhiBackendChanged();
}

void Platform::hideTaskbar()
{
    if (m_taskbarHidden)
        return;

    const QList<HWND> bars = taskbarWindows();
    if (bars.isEmpty())
        return;

    // If the previous shell was killed before it could restore the taskbar,
    // recover the saved visibility/state before hiding it again.
    QSettings settings;
    if (settings.value(QStringLiteral("shell/taskbarHeld"), false).toBool()) {
        const quint32 previous
            = settings.value(QStringLiteral("shell/taskbarState"), 0).toUInt();
        APPBARDATA restore = {};
        restore.cbSize = sizeof(APPBARDATA);
        for (HWND bar : bars) {
            ShowWindow(bar, SW_SHOW);
            restore.hWnd = bar;
            restore.lParam = static_cast<LPARAM>(previous);
            SHAppBarMessage(ABM_SETSTATE, &restore);
        }
        settings.setValue(QStringLiteral("shell/taskbarHeld"), false);
    }

    APPBARDATA abd = {};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = bars.first();
    m_taskbarState = static_cast<quint32>(SHAppBarMessage(ABM_GETSTATE, &abd));
    m_taskbarWasVisible = IsWindowVisible(bars.first());

    settings.setValue(QStringLiteral("shell/taskbarState"), m_taskbarState);
    settings.setValue(QStringLiteral("shell/taskbarHeld"), true);
    settings.sync();

    for (HWND bar : bars) {
        abd.hWnd = bar;
        abd.lParam = static_cast<LPARAM>(ABS_AUTOHIDE);
        SHAppBarMessage(ABM_SETSTATE, &abd);
        // Auto-hide alone still reveals the taskbar when the pointer reaches
        // the bottom edge. Hide the window as well; the work area is already
        // expanded by the auto-hide state.
        ShowWindow(bar, SW_HIDE);
    }

    m_taskbarHidden = true;
    m_taskbarTimer.start();
}

void Platform::enforceTaskbarHidden()
{
    if (!m_taskbarHidden) {
        hideTaskbar();
        return;
    }

    // Explorer can re-show the taskbar for the Start menu or after a shell
    // refresh; put it back off-screen immediately. The auto-hide state above
    // is what keeps the work area full, not the visibility of the window.
    for (HWND bar : taskbarWindows())
        ShowWindow(bar, SW_HIDE);
}

void Platform::restoreTaskbar()
{
    m_taskbarTimer.stop();
    if (!m_taskbarHidden)
        return;

    const QList<HWND> bars = taskbarWindows();
    APPBARDATA abd = {};
    abd.cbSize = sizeof(APPBARDATA);
    for (HWND bar : bars) {
        if (m_taskbarWasVisible)
            ShowWindow(bar, SW_SHOW);
        abd.hWnd = bar;
        abd.lParam = static_cast<LPARAM>(m_taskbarState);
        SHAppBarMessage(ABM_SETSTATE, &abd);
    }

    QSettings settings;
    settings.setValue(QStringLiteral("shell/taskbarHeld"), false);
    settings.sync();
    m_taskbarHidden = false;
}

void Platform::hideDesktopIcons()
{
    if (m_desktopIconsHidden)
        return;

    const HWND list = desktopIconWindow();
    if (!list)
        return;

    QSettings settings;
    // A previous run that was killed may have left the icons hidden with no
    // record of their original state; reveal them once so the new session
    // starts from a known state before hiding them again.
    if (settings.value(QStringLiteral("shell/desktopIconsHeld"), false).toBool())
        ShowWindow(list, SW_SHOW);

    m_desktopIconsWereVisible = IsWindowVisible(list);

    settings.setValue(QStringLiteral("shell/desktopIconsHeld"), true);
    settings.sync();

    ShowWindow(list, SW_HIDE);
    m_desktopIconsHidden = true;
    m_taskbarTimer.start();
}

void Platform::enforceDesktopIconsHidden()
{
    if (!m_desktopIconsHidden) {
        hideDesktopIcons();
        return;
    }

    // Explorer rebuilds the list view after a refresh; hide the new one too.
    if (const HWND list = desktopIconWindow())
        ShowWindow(list, SW_HIDE);
}

void Platform::restoreDesktopIcons()
{
    if (!m_desktopIconsHidden)
        return;

    if (const HWND list = desktopIconWindow()) {
        if (m_desktopIconsWereVisible)
            ShowWindow(list, SW_SHOW);
    }

    QSettings settings;
    settings.setValue(QStringLiteral("shell/desktopIconsHeld"), false);
    settings.sync();
    m_desktopIconsHidden = false;
}

bool Platform::registerLeftAppBar(QWindow* window, int logicalWidth)
{
    if (!window)
        return false;

    if (m_appBars.contains(window))
        return true;

    if (!m_appBarMessage)
        m_appBarMessage = RegisterWindowMessageW(L"CaelestiaShellAppBarMessage");

    if (!m_appBarMessage) {
        qWarning("Caelestia AppBar: callback message registration failed");
        return false;
    }

    QScreen* screen = window->screen() ? window->screen() : QGuiApplication::primaryScreen();
    if (!screen)
        return false;

    const auto hwnd = static_cast<quintptr>(window->winId());
    if (!hwnd) {
        qWarning("Caelestia AppBar: invalid window handle");
        return false;
    }

    APPBARDATA abd = {};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = reinterpret_cast<HWND>(hwnd);
    abd.uCallbackMessage = m_appBarMessage;

    if (!SHAppBarMessage(ABM_NEW, &abd)) {
        qWarning("Caelestia AppBar: ABM_NEW failed");
        return false;
    }

    const bool firstAppBar = m_appBars.isEmpty();

    AppBarEntry entry;
    entry.window = window;
    entry.hwnd = hwnd;
    entry.screen = screen;
    entry.logicalWidth = qMax(1, logicalWidth);
    m_appBars.insert(window, entry);

    auto it = m_appBars.find(window);
    it->windowScreenConnection
        = connect(window, &QWindow::screenChanged, this, [this, window](QScreen* newScreen) {
              handleWindowScreenChanged(window, newScreen);
          });
    connect(window, &QObject::destroyed, this, [this, window]() { unregisterAppBar(window); });

    connectEntryScreen(*it, screen);
    refreshAppBarEntry(*it);

    // Raw mouse input keeps the closed click-through overlay able to detect
    // the top-edge Dashboard hover without a polling timer. It is delivered
    // even when the shell is not focused.
    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = reinterpret_cast<HWND>(it->hwnd);
    if (RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        m_rawInputHwnd = it->hwnd;
        if (firstAppBar)
            qInfo("Caelestia raw mouse input registered");
    } else if (firstAppBar) {
        qWarning("Caelestia raw mouse input registration failed");
    }

    if (firstAppBar) {
        emit appBarRegisteredChanged();
        qInfo("Caelestia AppBar registered");
    }

    const HWND barHwnd = reinterpret_cast<HWND>(it->hwnd);
    HMONITOR monitor = monitorForScreen(screen);
    if (!monitor)
        monitor = MonitorFromWindow(barHwnd, MONITOR_DEFAULTTONEAREST);
    qInfo().noquote() << "Caelestia AppBar screen:" << screen->name() << screen->geometry()
                      << "logical" << it->logicalWidth << "x" << m_appBarHeight
                      << "dpi" << dpiForMonitor(monitor, barHwnd)
                      << "dpr" << screen->devicePixelRatio();
    return true;
}

void Platform::unregisterAppBar(QWindow* window)
{
    auto it = m_appBars.find(window);
    if (it == m_appBars.end())
        return;

    APPBARDATA abd = {};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = reinterpret_cast<HWND>(it->hwnd);
    SHAppBarMessage(ABM_REMOVE, &abd);

    disconnect(it->windowScreenConnection);
    disconnectEntryScreen(*it);
    m_appBars.erase(it);

    if (m_appBars.isEmpty()) {
        m_appBarHeight = 0;
        m_appBarWidth = 0;
        emit appBarHeightChanged();
        emit appBarWidthChanged();
        emit appBarRegisteredChanged();
        qInfo("Caelestia AppBar unregistered");
    }
}

void Platform::unregisterAppBar()
{
    const auto windows = m_appBars.keys();
    for (auto* window : windows)
        unregisterAppBar(window);
}

void Platform::restoreLastExternalWindow()
{
    const HWND hwnd = reinterpret_cast<HWND>(m_activeWindowHwnd);
    if (hwnd && IsWindow(hwnd))
        SetForegroundWindow(hwnd);
}

void Platform::raiseAppBars()
{
    for (auto it = m_appBars.begin(); it != m_appBars.end(); ++it) {
        HWND hwnd = reinterpret_cast<HWND>(it->hwnd);
        if (hwnd && IsWindow(hwnd))
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
}

void Platform::setWindowInputState(QWindow* window, bool clickThrough, bool focusable)
{
    if (!window)
        return;

    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd)
        return;

    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    LONG_PTR updated = style;
    if (clickThrough)
        updated |= WS_EX_TRANSPARENT;
    else
        updated &= ~WS_EX_TRANSPARENT;

    if (focusable)
        updated &= ~WS_EX_NOACTIVATE;
    else
        updated |= WS_EX_NOACTIVATE;

    if (updated == style)
        return;

    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, updated);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void Platform::disableWindowClose(QWindow* window)
{
    if (!window)
        return;

    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd)
        return;

    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if ((style & WS_SYSMENU) == 0)
        return;

    SetWindowLongPtrW(hwnd, GWL_STYLE, style & ~WS_SYSMENU);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void Platform::setEscapeHotkey(QWindow* window, bool enabled){
    const HWND hwnd = window ? reinterpret_cast<HWND>(window->winId()) : nullptr;

    if (!enabled) {
        if (m_escapeHotkeyHwnd && hwnd
            && reinterpret_cast<HWND>(m_escapeHotkeyHwnd) == hwnd) {
            UnregisterHotKey(hwnd, kEscapeHotkeyId);
            m_escapeHotkeyHwnd = 0;
        }
        return;
    }

    if (!hwnd || m_escapeHotkeyHwnd == reinterpret_cast<quintptr>(hwnd))
        return;

    if (m_escapeHotkeyHwnd) {
        UnregisterHotKey(reinterpret_cast<HWND>(m_escapeHotkeyHwnd), kEscapeHotkeyId);
        m_escapeHotkeyHwnd = 0;
    }

    if (RegisterHotKey(hwnd, kEscapeHotkeyId, 0, VK_ESCAPE)) {
        m_escapeHotkeyHwnd = reinterpret_cast<quintptr>(hwnd);
    } else {
        qWarning("Caelestia: RegisterHotKey(Escape) failed");
    }
}

void Platform::fitWindowToScreen(QWindow* window, QScreen* screen)
{
    if (!window || !screen)
        return;

    // setScreen is what makes Qt use that monitor's DPI for the window; the
    // geometry is in DIPs, so QScreen::geometry is exactly the right rect.
    window->setScreen(screen);
    window->setGeometry(screen->geometry());
}

bool Platform::openExternal(const QString& target)
{
    const QString trimmed = target.trimmed();
    if (trimmed.isEmpty())
        return false;

    // ShellExecuteW is the Windows "run this" entry point: it searches PATH,
    // honours App Paths, and dispatches registered URI schemes such as
    // ms-settings:. Its documented success values are > 32.
    const auto result = ShellExecuteW(nullptr, L"open",
        reinterpret_cast<LPCWSTR>(trimmed.utf16()), nullptr, nullptr, SW_SHOWNORMAL);
    const bool ok = reinterpret_cast<qintptr>(result) > 32;
    if (!ok)
        qWarning("Caelestia: ShellExecuteW failed for '%s' (%lld)", qPrintable(trimmed),
            static_cast<long long>(reinterpret_cast<qintptr>(result)));
    return ok;
}

void Platform::copyText(const QString& text)
{
    QGuiApplication::clipboard()->setText(text);
}

QVariant Platform::shellSetting(const QString& key, const QVariant& fallback) const
{
    QSettings settings;
    return settings.value(QStringLiteral("shell/") + key, fallback);
}

void Platform::setShellSetting(const QString& key, const QVariant& value)
{
    QSettings settings;
    settings.setValue(QStringLiteral("shell/") + key, value);
    settings.sync();
}

void Platform::refreshAppBar()
{
    if (m_appBars.isEmpty() || m_appBarUpdating)
        return;

    QScopedValueRollback<bool> guard(m_appBarUpdating, true);
    for (auto it = m_appBars.begin(); it != m_appBars.end(); ++it)
        refreshAppBarEntry(it.value());
}

void Platform::refreshAppBarEntry(AppBarEntry& entry)
{
    HWND hwnd = reinterpret_cast<HWND>(entry.hwnd);
    if (!hwnd || !IsWindow(hwnd))
        return;

    QScopedValueRollback<bool> guard(m_appBarUpdating, true);

    HMONITOR monitor = monitorForScreen(entry.screen);
    if (!monitor)
        monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo = {};
    monitorInfo.cbSize = sizeof(MONITORINFO);
    if (!GetMonitorInfoW(monitor, &monitorInfo))
        return;

    const UINT dpi = dpiForMonitor(monitor, hwnd);
    const qreal scale = static_cast<qreal>(dpi) / 96.0;
    const int physicalWidth = qMax(1, qRound(entry.logicalWidth * scale));

    RECT desired = monitorInfo.rcMonitor;
    desired.top = monitorInfo.rcWork.top;
    desired.bottom = monitorInfo.rcWork.bottom;
    desired.right = desired.left + physicalWidth;

    APPBARDATA abd = {};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hwnd;
    abd.uEdge = ABE_LEFT;
    abd.rc = desired;

    SHAppBarMessage(ABM_QUERYPOS, &abd);

    const QRect requestedRect(
        abd.rc.left, abd.rc.top, abd.rc.right - abd.rc.left, abd.rc.bottom - abd.rc.top);
    if (entry.lastRect == requestedRect)
        return;

    SHAppBarMessage(ABM_SETPOS, &abd);

    const int physicalFinalWidth = abd.rc.right - abd.rc.left;
    const int physicalFinalHeight = abd.rc.bottom - abd.rc.top;
    if (physicalFinalWidth <= 0 || physicalFinalHeight <= 0)
        return;

    entry.lastRect
        = QRect(abd.rc.left, abd.rc.top, physicalFinalWidth, physicalFinalHeight);

    const int logicalWidth = qRound(physicalFinalWidth / scale);
    const int logicalHeight = qRound(physicalFinalHeight / scale);

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_APPBAR")) {
        qInfo().noquote() << "AppBar rect" << (entry.screen ? entry.screen->name() : QStringLiteral("?"))
                          << "dpi" << dpi << "scale" << scale << "desired" << desired.left
                          << desired.top << desired.right << desired.bottom << "final" << abd.rc.left
                          << abd.rc.top << abd.rc.right << abd.rc.bottom;
    }

    QScreen* screen = entry.window ? entry.window->screen() : nullptr;
    if (screen && (screen == QGuiApplication::primaryScreen() || m_appBars.size() == 1)) {
        if (m_appBarWidth != logicalWidth) {
            m_appBarWidth = logicalWidth;
            emit appBarWidthChanged();
        }
        if (m_appBarHeight != logicalHeight) {
            m_appBarHeight = logicalHeight;
            emit appBarHeightChanged();
        }
    }

    if (!SetWindowPos(hwnd, HWND_TOPMOST, abd.rc.left, abd.rc.top, physicalFinalWidth,
            physicalFinalHeight, SWP_NOACTIVATE | SWP_SHOWWINDOW)) {
        qWarning("Caelestia AppBar: SetWindowPos failed");
    }
}

void Platform::connectEntryScreen(AppBarEntry& entry, QScreen* screen)
{
    disconnectEntryScreen(entry);
    entry.screen = screen;
    if (!screen)
        return;

    const auto window = entry.window;
    entry.geometryConnection = connect(screen, &QScreen::geometryChanged, this, [this, window]() {
        auto it = m_appBars.find(window);
        if (it != m_appBars.end())
            refreshAppBarEntry(it.value());
    });
    entry.dpiConnection
        = connect(screen, &QScreen::logicalDotsPerInchChanged, this, [this, window]() {
              auto it = m_appBars.find(window);
              if (it != m_appBars.end())
                  refreshAppBarEntry(it.value());
          });
}

void Platform::disconnectEntryScreen(AppBarEntry& entry)
{
    disconnect(entry.geometryConnection);
    disconnect(entry.dpiConnection);
    entry.screen = nullptr;
}

void Platform::handleWindowScreenChanged(QWindow* window, QScreen* screen)
{
    auto it = m_appBars.find(window);
    if (it == m_appBars.end())
        return;

    connectEntryScreen(it.value(), screen);
    refreshAppBarEntry(it.value());
}

void Platform::handleScreensChanged()
{
    // Rebuild the QML windows only after Qt has finished its own monitor
    // bookkeeping; destroying QQuickWindows inside the screen signal crashes
    // the Windows platform plugin.
    QTimer::singleShot(0, this, [this]() {
        emit screensChanged();
        refreshAppBar();
    });
}

void Platform::handleScreenRemoved(QScreen* screen)
{
    if (!screen)
        return;

    const auto windows = m_appBars.keys();
    for (auto* window : windows) {
        auto it = m_appBars.find(window);
        if (it == m_appBars.end())
            continue;
        if (it->screen == screen || (window && window->screen() == screen)) {
            unregisterAppBar(window);
            if (window)
                window->hide();
        }
    }

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_APPBAR"))
        qInfo().noquote() << "Screen removed:" << screen->name();

    QTimer::singleShot(0, this, [this]() {
        emit screensChanged();
        refreshAppBar();
    });
}

bool Platform::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
{
    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
        return false;

    const auto* msg = static_cast<MSG*>(message);
    if (!msg)
        return false;

    if (msg->message == WM_INPUT && m_rawInputHwnd
        && msg->hwnd == reinterpret_cast<HWND>(m_rawInputHwnd)) {
        // The closed overlay is click-through, so raw mouse input drives the
        // Dashboard strip and the open-panel halo.
        const QPoint pos = QCursor::pos();
        emit globalMouseMoved(pos);
    }

    if (msg->message == WM_HOTKEY && msg->wParam == kEscapeHotkeyId) {
        emit escapePressed();
        if (result)
            *result = 0;
        return true;
    }

    if (!m_appBars.isEmpty() && msg->message == m_appBarMessage) {
        if (msg->wParam == ABN_POSCHANGED && !m_appBarUpdating)
            QTimer::singleShot(0, this, &Platform::refreshAppBar);
        return false;
    }

    if (!m_appBars.isEmpty()
        && (msg->message == WM_DISPLAYCHANGE || msg->message == WM_DPICHANGED))
        QTimer::singleShot(0, this, &Platform::refreshAppBar);

    return false;
}

void Platform::startActiveWindowTracking()
{
    if (m_activeWindowTracking)
        return;

    m_activeWindowTracking = true;
    g_activeWindowPlatform = this;

    const auto installHook = [](DWORD minEvent, DWORD maxEvent) {
        return reinterpret_cast<quintptr>(SetWinEventHook(
            minEvent, maxEvent, nullptr, winEventCallback, 0, 0, WINEVENT_OUTOFCONTEXT));
    };

    m_foregroundHook = installHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND);
    m_nameChangeHook = installHook(EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE);
    m_minimizeHook = installHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND);
    m_destroyHook = installHook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_DESTROY);

    if (!m_foregroundHook)
        qWarning("Caelestia active window: SetWinEventHook(EVENT_SYSTEM_FOREGROUND) failed");

    updateActiveWindow(reinterpret_cast<quintptr>(GetForegroundWindow()), true);
}

void Platform::stopActiveWindowTracking()
{
    if (!m_activeWindowTracking)
        return;

    if (m_foregroundHook)
        UnhookWinEvent(reinterpret_cast<HWINEVENTHOOK>(m_foregroundHook));
    if (m_nameChangeHook)
        UnhookWinEvent(reinterpret_cast<HWINEVENTHOOK>(m_nameChangeHook));
    if (m_minimizeHook)
        UnhookWinEvent(reinterpret_cast<HWINEVENTHOOK>(m_minimizeHook));
    if (m_destroyHook)
        UnhookWinEvent(reinterpret_cast<HWINEVENTHOOK>(m_destroyHook));

    m_foregroundHook = 0;
    m_nameChangeHook = 0;
    m_minimizeHook = 0;
    m_destroyHook = 0;
    m_activeWindowTracking = false;

    if (g_activeWindowPlatform == this)
        g_activeWindowPlatform = nullptr;

    clearActiveWindow();
}

void Platform::handleWinEvent(quint32 event, quintptr hwnd, qint32 idObject, qint32 idChild)
{
    if (!m_activeWindowTracking)
        return;

    switch (event) {
    case EVENT_SYSTEM_FOREGROUND:
        updateActiveWindow(hwnd, true);
        break;
    case EVENT_OBJECT_NAMECHANGE:
        if (idObject == OBJID_WINDOW && idChild == CHILDID_SELF && hwnd == m_activeWindowHwnd)
            updateActiveWindow(hwnd, false);
        break;
    case EVENT_SYSTEM_MINIMIZESTART:
        if (hwnd == m_activeWindowHwnd)
            updateActiveWindow(reinterpret_cast<quintptr>(GetForegroundWindow()), true);
        break;
    case EVENT_SYSTEM_MINIMIZEEND:
        if (hwnd == m_activeWindowHwnd)
            updateActiveWindow(hwnd, true);
        break;
    case EVENT_OBJECT_DESTROY:
        if (idObject == OBJID_WINDOW && idChild == CHILDID_SELF && hwnd == m_activeWindowHwnd) {
            m_activeWindowHwnd = 0;
            updateActiveWindow(reinterpret_cast<quintptr>(GetForegroundWindow()), true);
        }
        break;
    default:
        break;
    }
}

void Platform::updateActiveWindow(quintptr hwndValue, bool refreshIcon)
{
    if (!m_activeWindowTracking)
        return;

    HWND hwnd = reinterpret_cast<HWND>(hwndValue);
    if (!hwnd || !IsWindow(hwnd)) {
        clearActiveWindow();
        return;
    }

    // Clicking an AppBar must not replace the last external window.
    for (const auto& entry : m_appBars) {
        if (hwnd == reinterpret_cast<HWND>(entry.hwnd))
            return;
    }

    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == GetCurrentProcessId())
        return;

    if (!IsWindowVisible(hwnd)) {
        clearActiveWindow();
        return;
    }

    if (isShellWindowClass(windowClassName(hwndValue))) {
        clearActiveWindow();
        return;
    }

    QString title;
    const int titleLength = GetWindowTextLengthW(hwnd);
    if (titleLength > 0) {
        std::wstring buffer(static_cast<size_t>(titleLength) + 1, L'\0');
        const int copied = GetWindowTextW(hwnd, buffer.data(), titleLength + 1);
        if (copied > 0)
            title = QString::fromWCharArray(buffer.data(), copied);
    }

    const bool iconNeedsRefresh = refreshIcon || hwndValue != m_activeWindowHwnd;
    const QString icon = iconNeedsRefresh ? readWindowIcon(hwndValue) : m_activeWindowIcon;

    const quintptr previousHwnd = m_activeWindowHwnd;
    m_activeWindowHwnd = hwndValue;
    // Two windows can share title and icon; the id still has to refresh.
    if (!setActiveWindowInfo(title, icon) && previousHwnd != hwndValue)
        emit activeWindowChanged();
}

void Platform::clearActiveWindow()
{
    const quintptr previousHwnd = m_activeWindowHwnd;
    m_activeWindowHwnd = 0;
    if (!setActiveWindowInfo(QString(), QString()) && previousHwnd != 0)
        emit activeWindowChanged();
}

bool Platform::setActiveWindowInfo(const QString& title, const QString& icon)
{
    if (m_activeWindowTitle == title && m_activeWindowIcon == icon)
        return false;

    m_activeWindowTitle = title;
    m_activeWindowIcon = icon;
    emit activeWindowChanged();

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_ACTIVE_WINDOW")) {
        qInfo().noquote() << "Active window:" << (title.isEmpty() ? QStringLiteral("<no title>") : title)
                          << "icon:" << !icon.isEmpty();
    }
    return true;
}

QString Platform::readWindowIcon(quintptr hwndValue) const
{
    HWND hwnd = reinterpret_cast<HWND>(hwndValue);
    if (!hwnd)
        return {};

    const auto queryIcon = [hwnd](WPARAM which) -> HICON {
        DWORD_PTR result = 0;
        if (SendMessageTimeoutW(hwnd, WM_GETICON, which, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 100, &result))
            return reinterpret_cast<HICON>(result);
        return nullptr;
    };

    HICON icon = queryIcon(ICON_BIG);
    if (!icon)
        icon = queryIcon(ICON_SMALL2);
    if (!icon)
        icon = queryIcon(ICON_SMALL);
    if (!icon)
        icon = reinterpret_cast<HICON>(GetClassLongPtrW(hwnd, GCLP_HICON));
    if (!icon)
        icon = reinterpret_cast<HICON>(GetClassLongPtrW(hwnd, GCLP_HICONSM));

    bool ownedIcon = false;

    if (!icon) {
        DWORD processId = 0;
        GetWindowThreadProcessId(hwnd, &processId);
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (process) {
            wchar_t path[32768] = {};
            DWORD pathLength = 32768;
            if (QueryFullProcessImageNameW(process, 0, path, &pathLength)) {
                SHFILEINFOW info = {};
                if (SHGetFileInfoW(path, 0, &info, sizeof(info), SHGFI_ICON | SHGFI_LARGEICON)) {
                    icon = info.hIcon;
                    ownedIcon = true;
                }
            }
            CloseHandle(process);
        }
    }

    if (!icon)
        return {};

    QImage image = QImage::fromHICON(icon);
    if (ownedIcon)
        DestroyIcon(icon);

    if (image.isNull())
        return {};

    if (image.width() > 64 || image.height() > 64)
        image = image.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG"))
        return {};

    return QStringLiteral("data:image/png;base64,") + QString::fromLatin1(png.toBase64());
}

QString Platform::windowClassName(quintptr hwndValue)
{
    wchar_t buffer[256] = {};
    const int length = GetClassNameW(reinterpret_cast<HWND>(hwndValue), buffer, 256);
    if (length <= 0)
        return {};
    return QString::fromWCharArray(buffer, length);
}

void Platform::refresh()
{
    const bool dark = readRegistryInt(
                          QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
                          QStringLiteral("AppsUseLightTheme"), 1)
        == 0;
    if (m_darkMode != dark) {
        m_darkMode = dark;
        emit darkModeChanged();
    }

    DWORD colorization = 0;
    BOOL opaque = FALSE;
    if (SUCCEEDED(DwmGetColorizationColor(&colorization, &opaque))) {
        QColor accent = QColor::fromRgb(static_cast<QRgb>(colorization));
        if (!accent.isValid())
            accent = QColor(QStringLiteral("#ffb0ca"));
        accent.setAlpha(255);
        if (m_accentColor != accent) {
            m_accentColor = accent;
            emit accentColorChanged();
        }
    }

    wchar_t wallpaper[MAX_PATH] = {};
    if (SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, wallpaper, 0)) {
        const QString path = QString::fromWCharArray(wallpaper);
        if (m_wallpaperPath != path) {
            m_wallpaperPath = path;
            emit wallpaperPathChanged();
        }
        const bool available = QFileInfo::exists(path);
        if (m_wallpaperAvailable != available) {
            m_wallpaperAvailable = available;
            emit wallpaperAvailableChanged();
        }
    }

    BOOL animations = TRUE;
    if (SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animations, 0)) {
        const bool enabled = animations != FALSE;
        if (m_animationsEnabled != enabled) {
            m_animationsEnabled = enabled;
            emit animationsEnabledChanged();
        }
    }

    HDC screen = GetDC(nullptr);
    if (screen) {
        const qreal scale = static_cast<qreal>(GetDeviceCaps(screen, LOGPIXELSX)) / 96.0;
        ReleaseDC(nullptr, screen);
        if (!qFuzzyCompare(m_systemScale, scale)) {
            m_systemScale = scale;
            emit systemScaleChanged();
        }
    }
}


