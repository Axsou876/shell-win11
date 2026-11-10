#include "platform.hpp"

#include <QBuffer>
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
    // work area so the shell can fill it. The previous state is restored when
    // the shell quits.
    hideTaskbar();
    QTimer::singleShot(1200, this, &Platform::refreshAppBar);

    m_refreshTimer.setInterval(1500);
    connect(&m_refreshTimer, &QTimer::timeout, this, &Platform::refresh);
    m_refreshTimer.start();

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        restoreTaskbar();
        unregisterAppBar();
    });
    connect(qApp, &QCoreApplication::aboutToQuit, this, &Platform::stopActiveWindowTracking);
    if (auto* gui = qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        connect(gui, &QGuiApplication::screenAdded, this, &Platform::handleScreensChanged);
        connect(gui, &QGuiApplication::screenRemoved, this, &Platform::handleScreenRemoved);
        connect(gui, &QGuiApplication::primaryScreenChanged, this, &Platform::refreshAppBar);
    }

    if (qApp)
        qApp->installNativeEventFilter(this);

    startActiveWindowTracking();
}

Platform::~Platform()
{
    restoreTaskbar();
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

    APPBARDATA abd = {};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = bars.first();
    m_taskbarState = static_cast<quint32>(SHAppBarMessage(ABM_GETSTATE, &abd));
    m_taskbarWasVisible = IsWindowVisible(bars.first());

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
}

void Platform::restoreTaskbar()
{
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

    m_taskbarHidden = false;
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

void Platform::setEscapeHotkey(QWindow* window, bool enabled)
{
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

    m_activeWindowHwnd = hwndValue;
    setActiveWindowInfo(title, icon);
}

void Platform::clearActiveWindow()
{
    m_activeWindowHwnd = 0;
    setActiveWindowInfo(QString(), QString());
}

void Platform::setActiveWindowInfo(const QString& title, const QString& icon)
{
    if (m_activeWindowTitle == title && m_activeWindowIcon == icon)
        return;

    m_activeWindowTitle = title;
    m_activeWindowIcon = icon;
    emit activeWindowChanged();

    if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_ACTIVE_WINDOW")) {
        qInfo().noquote() << "Active window:" << (title.isEmpty() ? QStringLiteral("<no title>") : title)
                          << "icon:" << !icon.isEmpty();
    }
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


