#include "windowmanagerservice.hpp"

#include "tilingengine.hpp"
#include "virtualdesktopservice.hpp"

#include <QCoreApplication>
#include <QCursor>
#include <QDebug>
#include <QGuiApplication>
#include <QMetaObject>
#include <QProcess>
#include <QScreen>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QVariantMap>

#include <windows.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <timeapi.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <utility>

using Microsoft::WRL::ComPtr;

namespace {
constexpr UINT kCommandMessage = WM_APP + 1;
constexpr UINT kWinEventMessage = WM_APP + 2;
constexpr UINT kStopMessage = WM_APP + 3;
// Posted by the low-level keyboard hook: wParam is an index into
// Impl::hookBindings. The hook itself must stay fast, so the action runs here.
constexpr UINT kHookActionMessage = WM_APP + 4;
// Posted by the low-level mouse hook while Win + left button drags a window.
// wParam is one of the kDrag* phases below.
constexpr UINT kDragMessage = WM_APP + 5;
constexpr WPARAM kDragBegin = 0;
constexpr WPARAM kDragMove = 1;
constexpr WPARAM kDragEnd = 2;
// Posted when the Windows key is tapped on its own: Explorer would open the
// Start menu, the shell opens the Caelestia launcher instead.
constexpr UINT kLauncherMessage = WM_APP + 6;
constexpr UINT_PTR kAnimationTimer = 1;
constexpr UINT_PTR kRescanTimer = 2;
constexpr UINT_PTR kDebounceTimer = 3;
constexpr UINT_PTR kFocusTimer = 4;
constexpr int kHotkeyBase = 0x4100;
constexpr int kAnimationDurationMs = 180;
constexpr int kAnimationTickMs = 7;
// How long the desktop slide lasts. The reference is Caelestia on Hyprland:
// the incoming desktop sweeps in horizontally over roughly 0.4 s.
constexpr int kDesktopSlideDurationMs = 380;

HWND g_messageWindow = nullptr;

qint64 nowMs()
{
    return static_cast<qint64>(GetTickCount64());
}

QString windowClassName(HWND hwnd)
{
    wchar_t buffer[256] = {};
    GetClassNameW(hwnd, buffer, 256);
    return QString::fromWCharArray(buffer);
}

bool isShellWindowClass(const QString& className)
{
    const QString name = className.toLower();
    return name == QStringLiteral("progman") || name == QStringLiteral("workerw")
        || name == QStringLiteral("shell_traywnd")
        || name == QStringLiteral("shell_secondarytraywnd")
        || name == QStringLiteral("windows.ui.core.corewindow")
        || name == QStringLiteral("xamlexplorerhostislandwindow")
        || name == QStringLiteral("taskswitcherwnd")
        || name == QStringLiteral("multitaskingviewframe")
        || name == QStringLiteral("foregroundstaging")
        || name == QStringLiteral("shell_inputswitchtoplevelwindow")
        || name == QStringLiteral("windows.internal.shell.tabproxywindow")
        || name == QStringLiteral("windows.ui.input.inputsite.windowclass");
}

QRect windowRect(HWND hwnd)
{
    RECT rect = {};
    if (!GetWindowRect(hwnd, &rect))
        return {};
    return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
}

// The active-window outline is gone on purpose. The Hyprland-style shell has
// no focus ring: the white 2 px frame that used to follow the active window
// read as an artefact while the pointer moved across windows.
struct MonitorInfo {
    QString key;
    QRect work;
    QRect bounds;
    bool valid = false;
};

MonitorInfo monitorFor(HWND hwnd)
{
    MonitorInfo result;
    HMONITOR monitor = hwnd ? MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST)
                            : MonitorFromPoint(POINT { 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    if (!monitor)
        return result;
    MONITORINFOEXW info = {};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info))
        return result;
    result.key = QString::fromWCharArray(info.szDevice);
    result.work = QRect(info.rcWork.left, info.rcWork.top,
        info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top);
    result.bounds = QRect(info.rcMonitor.left, info.rcMonitor.top,
        info.rcMonitor.right - info.rcMonitor.left,
        info.rcMonitor.bottom - info.rcMonitor.top);
    result.valid = true;
    return result;
}

QRect contentRect(const QRect& work, int windowCount)
{
    const int outer = windowCount <= 1 ? 20 : 10;
    QRect content = work.adjusted(outer, outer, -outer, -outer);
    if (content.width() < 80 || content.height() < 80)
        content = work.adjusted(4, 4, -4, -4);
    return content;
}

struct ParsedKey {
    UINT mods = 0;
    UINT vk = 0;
    bool valid = false;
};

ParsedKey parseKeyBinding(const QString& text)
{
    ParsedKey result;
    const QStringList parts = text.split(QLatin1Char('+'), Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return result;

    for (const QString& rawPart : parts) {
        const QString part = rawPart.trimmed();
        if (part.compare(QStringLiteral("win"), Qt::CaseInsensitive) == 0
            || part.compare(QStringLiteral("super"), Qt::CaseInsensitive) == 0) {
            result.mods |= MOD_WIN;
            continue;
        }
        if (part.compare(QStringLiteral("ctrl"), Qt::CaseInsensitive) == 0
            || part.compare(QStringLiteral("control"), Qt::CaseInsensitive) == 0) {
            result.mods |= MOD_CONTROL;
            continue;
        }
        if (part.compare(QStringLiteral("alt"), Qt::CaseInsensitive) == 0) {
            result.mods |= MOD_ALT;
            continue;
        }
        if (part.compare(QStringLiteral("shift"), Qt::CaseInsensitive) == 0) {
            result.mods |= MOD_SHIFT;
            continue;
        }
        if (result.vk)
            return result;

        const QString key = part.toUpper();
        if (key.size() == 1 && key.at(0).isLetterOrNumber()) {
            result.vk = static_cast<UINT>(key.at(0).unicode());
            continue;
        }

        static const QHash<QString, UINT> named = {
            { QStringLiteral("LEFT"), VK_LEFT },
            { QStringLiteral("RIGHT"), VK_RIGHT },
            { QStringLiteral("UP"), VK_UP },
            { QStringLiteral("DOWN"), VK_DOWN },
            { QStringLiteral("PAGEUP"), VK_PRIOR },
            { QStringLiteral("PAGEDOWN"), VK_NEXT },
            { QStringLiteral("HOME"), VK_HOME },
            { QStringLiteral("END"), VK_END },
            { QStringLiteral("SPACE"), VK_SPACE },
            { QStringLiteral("ENTER"), VK_RETURN },
            { QStringLiteral("RETURN"), VK_RETURN },
            { QStringLiteral("ESCAPE"), VK_ESCAPE },
            { QStringLiteral("ESC"), VK_ESCAPE },
            { QStringLiteral("TAB"), VK_TAB },
            { QStringLiteral("MINUS"), VK_OEM_MINUS },
            { QStringLiteral("EQUAL"), VK_OEM_PLUS },
            { QStringLiteral("BACKSLASH"), VK_OEM_5 },
            { QStringLiteral("COMMA"), VK_OEM_COMMA },
            { QStringLiteral("PERIOD"), VK_OEM_PERIOD },
            { QStringLiteral("BACKSPACE"), VK_BACK },
            { QStringLiteral("DELETE"), VK_DELETE },
            { QStringLiteral("INSERT"), VK_INSERT },
        };
        const auto it = named.constFind(key);
        if (it != named.constEnd()) {
            result.vk = it.value();
            continue;
        }
        if (key.startsWith(QLatin1Char('F'))) {
            bool ok = false;
            const int number = key.mid(1).toInt(&ok);
            if (ok && number >= 1 && number <= 24) {
                result.vk = VK_F1 + static_cast<UINT>(number - 1);
                continue;
            }
        }
        return result;
    }

    result.valid = result.vk != 0 && result.mods != 0;
    return result;
}

enum class WmAction {
    FocusLeft,
    FocusRight,
    FocusUp,
    FocusDown,
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    FocusNext,
    FocusPrev,
    ToggleFloating,
    ToggleFullscreen,
    CloseWindow,
    CycleLayout,
    CycleLayoutBack,
    PromoteMaster,
    ToggleTiling,
    ToggleStrict,
    EmergencyDisable,
    WorkspaceSwitch,
    WorkspaceMove,
    WorkspacePrev,
    WorkspaceNext,
    ResizeWidthDecrease,
    ResizeWidthIncrease,
    ResizeHeightDecrease,
    ResizeHeightIncrease,
    LaunchTerminal,
    LaunchExplorer,
    LaunchBrowser,
    LaunchEditor,
    Launcher,
    Sidebar,
    Session,
    LockScreen,
    Screenshot,
    ScreenshotClip,
};

struct BindingSpec {
    WmAction action;
    int argument;
    QString name;
    QString key;
    bool original;
    bool custom = false;
};

// Keys that only make sense while the tiling layouts run. Tiling is disabled
// on this port: leaving these bound would keep swallowing Win+arrows,
// Win+Minus and Win+Equal, which Windows itself uses (snap, maximise,
// minimise, magnifier). Everything else — close, workspaces, launcher,
// sidebar, session, lock, capture — stays bound.
bool isTilingAction(WmAction action)
{
    switch (action) {
    case WmAction::FocusLeft:
    case WmAction::FocusRight:
    case WmAction::FocusUp:
    case WmAction::FocusDown:
    case WmAction::MoveLeft:
    case WmAction::MoveRight:
    case WmAction::MoveUp:
    case WmAction::MoveDown:
    case WmAction::FocusNext:
    case WmAction::FocusPrev:
    case WmAction::ToggleFloating:
    case WmAction::CycleLayout:
    case WmAction::CycleLayoutBack:
    case WmAction::PromoteMaster:
    case WmAction::ToggleTiling:
    case WmAction::ToggleStrict:
    case WmAction::ResizeWidthDecrease:
    case WmAction::ResizeWidthIncrease:
    case WmAction::ResizeHeightDecrease:
    case WmAction::ResizeHeightIncrease:
        return true;
    default:
        return false;
    }
}

QVector<BindingSpec> defaultBindingSpecs(bool tiling)
{
    QVector<BindingSpec> specs;
    const auto add = [&specs, tiling](WmAction action, int argument, const QString& name,
                          const QString& key, bool original, bool custom = false) {
        if (!tiling && isTilingAction(action))
            return;
        specs.append({ action, argument, name, key, original, custom });
    };

    add(WmAction::FocusLeft, 0, QStringLiteral("focus-left"), QStringLiteral("Win+Left"), true);
    add(WmAction::FocusLeft, 0, QStringLiteral("focus-left"), QStringLiteral("Ctrl+Alt+Left"), false);
    add(WmAction::FocusLeft, 0, QStringLiteral("focus-left"), QStringLiteral("Ctrl+Alt+H"), false);
    add(WmAction::FocusRight, 0, QStringLiteral("focus-right"), QStringLiteral("Win+Right"), true);
    add(WmAction::FocusRight, 0, QStringLiteral("focus-right"), QStringLiteral("Ctrl+Alt+Right"), false);
    add(WmAction::FocusRight, 0, QStringLiteral("focus-right"), QStringLiteral("Ctrl+Alt+L"), false);
    add(WmAction::FocusUp, 0, QStringLiteral("focus-up"), QStringLiteral("Win+Up"), true);
    add(WmAction::FocusUp, 0, QStringLiteral("focus-up"), QStringLiteral("Ctrl+Alt+Up"), false);
    add(WmAction::FocusUp, 0, QStringLiteral("focus-up"), QStringLiteral("Ctrl+Alt+K"), false);
    add(WmAction::FocusDown, 0, QStringLiteral("focus-down"), QStringLiteral("Win+Down"), true);
    add(WmAction::FocusDown, 0, QStringLiteral("focus-down"), QStringLiteral("Ctrl+Alt+Down"), false);
    add(WmAction::FocusDown, 0, QStringLiteral("focus-down"), QStringLiteral("Ctrl+Alt+J"), false);

    add(WmAction::MoveLeft, 0, QStringLiteral("move-left"), QStringLiteral("Win+Shift+Left"), true);
    add(WmAction::MoveLeft, 0, QStringLiteral("move-left"), QStringLiteral("Ctrl+Alt+Shift+Left"), false);
    add(WmAction::MoveLeft, 0, QStringLiteral("move-left"), QStringLiteral("Ctrl+Alt+Shift+H"), false);
    add(WmAction::MoveRight, 0, QStringLiteral("move-right"), QStringLiteral("Win+Shift+Right"), true);
    add(WmAction::MoveRight, 0, QStringLiteral("move-right"), QStringLiteral("Ctrl+Alt+Shift+Right"), false);
    add(WmAction::MoveRight, 0, QStringLiteral("move-right"), QStringLiteral("Ctrl+Alt+Shift+L"), false);
    add(WmAction::MoveUp, 0, QStringLiteral("move-up"), QStringLiteral("Win+Shift+Up"), true);
    add(WmAction::MoveUp, 0, QStringLiteral("move-up"), QStringLiteral("Ctrl+Alt+Shift+Up"), false);
    add(WmAction::MoveUp, 0, QStringLiteral("move-up"), QStringLiteral("Ctrl+Alt+Shift+K"), false);
    add(WmAction::MoveDown, 0, QStringLiteral("move-down"), QStringLiteral("Win+Shift+Down"), true);
    add(WmAction::MoveDown, 0, QStringLiteral("move-down"), QStringLiteral("Ctrl+Alt+Shift+Down"), false);
    add(WmAction::MoveDown, 0, QStringLiteral("move-down"), QStringLiteral("Ctrl+Alt+Shift+J"), false);

    add(WmAction::FocusNext, 0, QStringLiteral("focus-next"), QStringLiteral("Ctrl+Alt+PageDown"), false);
    add(WmAction::FocusPrev, 0, QStringLiteral("focus-prev"), QStringLiteral("Ctrl+Alt+PageUp"), false);
    add(WmAction::ToggleFloating, 0, QStringLiteral("toggle-floating"), QStringLiteral("Win+Alt+Space"), true);
    add(WmAction::ToggleFloating, 0, QStringLiteral("toggle-floating"), QStringLiteral("Ctrl+Alt+V"), false);
    add(WmAction::ToggleFullscreen, 0, QStringLiteral("toggle-fullscreen"), QStringLiteral("Win+F"), true);
    add(WmAction::ToggleFullscreen, 0, QStringLiteral("toggle-fullscreen"), QStringLiteral("Ctrl+Alt+F"), false);
    // Hyprland muscle memory as requested: Super+C closes the active window,
    // Super+Q opens the terminal. This intentionally swaps Caelestia's own
    // SUPER+Q (close) / SUPER+C (editor) defaults; those actions stay
    // reachable through Ctrl+Alt+Q and Ctrl+Alt+C.
    add(WmAction::CloseWindow, 0, QStringLiteral("close-window"), QStringLiteral("Win+C"), true);
    add(WmAction::CloseWindow, 0, QStringLiteral("close-window"), QStringLiteral("Ctrl+Alt+Q"), false);
    add(WmAction::CycleLayout, 0, QStringLiteral("layout-cycle"), QStringLiteral("Ctrl+Alt+P"), false);
    add(WmAction::CycleLayoutBack, 0, QStringLiteral("layout-back"), QStringLiteral("Ctrl+Alt+Shift+P"), false);
    add(WmAction::PromoteMaster, 0, QStringLiteral("promote-master"), QStringLiteral("Ctrl+Alt+Enter"), false);
    add(WmAction::ToggleTiling, 0, QStringLiteral("toggle-tiling"), QStringLiteral("Ctrl+Alt+Shift+T"), false);
    add(WmAction::ToggleStrict, 0, QStringLiteral("toggle-strict"), QStringLiteral("Ctrl+Alt+Shift+S"), false);
    add(WmAction::EmergencyDisable, 0, QStringLiteral("emergency-disable"), QStringLiteral("Ctrl+Alt+Shift+Escape"), false);

    add(WmAction::WorkspacePrev, 0, QStringLiteral("workspace-prev"), QStringLiteral("Ctrl+Alt+Shift+PageUp"), false);
    add(WmAction::WorkspaceNext, 0, QStringLiteral("workspace-next"), QStringLiteral("Ctrl+Alt+Shift+PageDown"), false);
    add(WmAction::ResizeWidthDecrease, 0, QStringLiteral("resize-width-dec"), QStringLiteral("Win+Minus"), true);
    add(WmAction::ResizeWidthDecrease, 0, QStringLiteral("resize-width-dec"), QStringLiteral("Ctrl+Alt+Minus"), false);
    add(WmAction::ResizeWidthIncrease, 0, QStringLiteral("resize-width-inc"), QStringLiteral("Win+Equal"), true);
    add(WmAction::ResizeWidthIncrease, 0, QStringLiteral("resize-width-inc"), QStringLiteral("Ctrl+Alt+Equal"), false);
    add(WmAction::ResizeHeightDecrease, 0, QStringLiteral("resize-height-dec"), QStringLiteral("Win+Shift+Minus"), true);
    add(WmAction::ResizeHeightDecrease, 0, QStringLiteral("resize-height-dec"), QStringLiteral("Ctrl+Alt+Shift+Minus"), false);
    add(WmAction::ResizeHeightIncrease, 0, QStringLiteral("resize-height-inc"), QStringLiteral("Win+Shift+Equal"), true);
    add(WmAction::ResizeHeightIncrease, 0, QStringLiteral("resize-height-inc"), QStringLiteral("Ctrl+Alt+Shift+Equal"), false);

    add(WmAction::LaunchTerminal, 0, QStringLiteral("terminal"), QStringLiteral("Win+T"), true);
    // Hyprland's Super+Return. Windows itself may claim Win+Enter for
    // Narrator; registration is best-effort like every other binding, and the
    // failure is logged rather than fatal.
    add(WmAction::LaunchTerminal, 0, QStringLiteral("terminal"), QStringLiteral("Win+Enter"), true);
    add(WmAction::LaunchTerminal, 0, QStringLiteral("terminal"), QStringLiteral("Win+Q"), true);
    add(WmAction::LaunchTerminal, 0, QStringLiteral("terminal"), QStringLiteral("Ctrl+Alt+T"), false);
    add(WmAction::LaunchExplorer, 0, QStringLiteral("file-manager"), QStringLiteral("Win+E"), true);
    add(WmAction::LaunchExplorer, 0, QStringLiteral("file-manager"), QStringLiteral("Ctrl+Alt+E"), false);
    add(WmAction::LaunchBrowser, 0, QStringLiteral("browser"), QStringLiteral("Win+W"), true);
    add(WmAction::LaunchBrowser, 0, QStringLiteral("browser"), QStringLiteral("Ctrl+Alt+W"), false);
    add(WmAction::LaunchEditor, 0, QStringLiteral("editor"), QStringLiteral("Ctrl+Alt+C"), false);
    add(WmAction::Launcher, 0, QStringLiteral("launcher"), QStringLiteral("Ctrl+Alt+Space"), false);
    // The shell owns the Windows key now (see consumeKey), so Windows' own
    // Win+S search can never open: keep the reflex working by opening the
    // launcher, which searches the same applications.
    add(WmAction::Launcher, 0, QStringLiteral("launcher"), QStringLiteral("Win+S"), true);
    add(WmAction::Sidebar, 0, QStringLiteral("sidebar"), QStringLiteral("Win+N"), true);
    add(WmAction::Sidebar, 0, QStringLiteral("sidebar"), QStringLiteral("Ctrl+Alt+N"), false);
    add(WmAction::Session, 0, QStringLiteral("session"), QStringLiteral("Ctrl+Alt+Shift+M"), false);
    // Shell lock screen. Win+L is Windows' own shortcut but the hook sees it
    // first now that the shell owns the key, so the familiar lock gesture opens
    // the shell's screen, whose password field validates the local account and
    // which always offers the Windows lock as fallback.
    add(WmAction::LockScreen, 0, QStringLiteral("lock"), QStringLiteral("Win+L"), true);
    add(WmAction::LockScreen, 0, QStringLiteral("lock"), QStringLiteral("Win+Alt+L"), true);
    add(WmAction::LockScreen, 0, QStringLiteral("lock"), QStringLiteral("Ctrl+Alt+Shift+U"), false);
    // Screenshot: the area picker freezes the screen, then saves or copies the
    // dragged region. Windows' Win+Shift+S would no longer reach the Snipping
    // Tool, so the same gesture opens the shell's own picker.
    add(WmAction::Screenshot, 0, QStringLiteral("screenshot"),
        QStringLiteral("Win+Shift+S"), true);
    add(WmAction::Screenshot, 0, QStringLiteral("screenshot"),
        QStringLiteral("Ctrl+Alt+S"), false);
    add(WmAction::ScreenshotClip, 0, QStringLiteral("screenshot-clip"),
        QStringLiteral("Ctrl+Alt+X"), false);

    for (int i = 1; i <= 10; ++i) {
        const QString key = QString::number(i % 10);
        const QString suffix = QString::number(i);
        add(WmAction::WorkspaceSwitch, i - 1, QStringLiteral("workspace-") + suffix,
            QStringLiteral("Win+") + key, true);
        add(WmAction::WorkspaceSwitch, i - 1, QStringLiteral("workspace-") + suffix,
            QStringLiteral("Ctrl+Alt+") + key, false);
        add(WmAction::WorkspaceMove, i - 1, QStringLiteral("move-workspace-") + suffix,
            QStringLiteral("Win+Alt+") + key, true);
        add(WmAction::WorkspaceMove, i - 1, QStringLiteral("move-workspace-") + suffix,
            QStringLiteral("Ctrl+Alt+Shift+") + key, false);
    }
    return specs;
}

struct ActiveBinding {
    WmAction action = WmAction::FocusLeft;
    int argument = 0;
    QString name;
    QString key;
    bool original = false;
    bool custom = false;
    int id = 0;
    bool registered = false;
};

// A Win+key combination Windows refused to RegisterHotKey because Explorer
// owns it (Win+Q, Win+1…0, Win+E, Win+Enter, Win+arrows, …). A low-level
// keyboard hook sees the key before Explorer's hotkey processing, so the
// shell can claim it exactly like Hyprland's SUPER bindings.
struct HookBinding {
    UINT mods = 0;
    UINT vk = 0;
    WmAction action = WmAction::FocusLeft;
    int argument = 0;
    QString name;
};

struct SavedWindow {
    WINDOWPLACEMENT placement = {};
    LONG style = 0;
    LONG exStyle = 0;
    QRect rect;
    bool valid = false;
};

struct Animation {
    QRect from;
    QRect to;
    qint64 start = 0;
    // The desktop slide runs longer than a tiling adjustment: sweeping a
    // window across the screen in 180 ms only lands a couple of frames.
    int duration = kAnimationDurationMs;
};

struct MonitorState {
    QString key;
    QRect work;
    TilingEngine engine;
};

struct FullscreenState {
    HWND hwnd = nullptr;
    LONG style = 0;
    LONG exStyle = 0;
    WINDOWPLACEMENT placement = {};
    QRect rect;
    bool wasTopmost = false;
    bool valid = false;
};

void CALLBACK winEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG idObject,
    LONG idChild, DWORD, DWORD)
{
    if (!g_messageWindow || !hwnd || idObject != OBJID_WINDOW
        || idChild != CHILDID_SELF) {
        return;
    }
    if (GetAncestor(hwnd, GA_ROOT) != hwnd)
        return;
    PostMessageW(g_messageWindow, kWinEventMessage,
        static_cast<WPARAM>(event), reinterpret_cast<LPARAM>(hwnd));
}
} // namespace

struct WindowManagerService::Impl {
    enum class CommandType { SetEnabled, SetStrict, SetLayout, CycleLayout, SetDesktop, SetFloating };

    struct Command {
        CommandType type = CommandType::SetEnabled;
        bool boolValue = false;
        QString stringValue;
        int index = -1;
        int count = 0;
        bool backward = false;
    };

    explicit Impl(WindowManagerService* owner)
        : q(owner)
        , enabled(qEnvironmentVariableIsEmpty("CAELESTIA_TILING")
              || qEnvironmentVariable("CAELESTIA_TILING") != QStringLiteral("0"))
    {
    }

    WindowManagerService* q = nullptr;
    std::thread thread;
    DWORD threadId = 0;
    HWND messageWindow = nullptr;
    HINSTANCE instance = nullptr;
    std::atomic<bool> stopRequested { false };
    std::atomic<bool> ready { false };
    std::mutex readyMutex;
    std::condition_variable readyCv;
    std::mutex queueMutex;
    std::deque<Command> queue;
    bool running = false;
    bool cleaned = false;

    bool enabled = true;
    bool strictMode = true;
    bool animationsEnabled = true;
    // Tiling is off on this port: windows keep the size and position Windows
    // gave them. Focus, close, workspaces, the dock and the desktop slide all
    // keep working; only the automatic placement is gone. Re-enable with
    // `tiling=true` in the window-manager INI.
    bool tilingEnabled = false;
    bool interacting = false;
    bool animationTimerRunning = false;
    QString defaultLayout = QStringLiteral("dwindle");
    QString currentDesktopId;
    // Set by applyDesktop, consumed by the first placement pass that actually
    // moves a window: +1 when moving down the column, -1 when moving up.
    int slideDirection = 0;
    bool slidePending = false;
    bool slideConsumed = false;
    qint64 slideArmedAt = 0;
    int desktopIndex = -1;
    int desktopCount = 0;
    HWND activeWindow = nullptr;
    HWND fullscreenWindow = nullptr;
    HWND closingWindow = nullptr;
    HWND focusAfterClose = nullptr;
    FullscreenState fullscreen;

    QHash<QString, std::shared_ptr<MonitorState>> monitors;
    QHash<QString, QHash<QString, QVariantMap>> savedLayouts;
    QHash<quintptr, SavedWindow> savedWindows;
    QSet<quintptr> floating;
    QSet<quintptr> freeMoved;
    QHash<quintptr, Animation> animations;
    QHash<quintptr, qint64> suppressUntil;
    QVector<ActiveBinding> activeBindings;
    QHash<int, int> hotkeyMap;
    QVector<HookBinding> hookBindings;
    QSet<UINT> hookHeldKeys;
    HHOOK keyboardHook = nullptr;
    HHOOK mouseHook = nullptr;
    // Win + left button drag state. The hook and the message loop share the
    // window-manager thread, so these need no locking.
    bool dragActive = false;
    HWND dragWindow = nullptr;
    POINT dragGrab = {};
    POINT dragCursor = {};
    QPoint dragOffset;
    // Bare Windows key: Win is held and nothing else was pressed or clicked
    // since it went down. On release the shell opens the launcher.
    bool winDown = false;
    bool winAlone = false;
    // The low-level hook is a free function; this points it at the one Impl.
    static inline Impl* s_hookImpl = nullptr;
    QVariantList publishedBindings;
    QString error;

    QVector<HWINEVENTHOOK> hooks;
    ComPtr<IVirtualDesktopManager> desktopManager;

    void start()
    {
        thread = std::thread([this]() { run(); });
        std::unique_lock lock(readyMutex);
        readyCv.wait_for(lock, std::chrono::seconds(4), [this]() { return ready.load(); });
    }

    void stop()
    {
        if (!thread.joinable())
            return;
        stopRequested = true;
        if (messageWindow) {
            if (!PostMessageW(messageWindow, kStopMessage, 0, 0) && threadId)
                PostThreadMessageW(threadId, WM_QUIT, 0, 0);
        } else {
            PostThreadMessageW(threadId, WM_QUIT, 0, 0);
        }
        thread.join();
    }

    void enqueue(Command command)
    {
        {
            std::lock_guard lock(queueMutex);
            queue.push_back(std::move(command));
        }
        if (messageWindow)
            PostMessageW(messageWindow, kCommandMessage, 0, 0);
    }

    void setEnabled(bool value)
    {
        Command command;
        command.type = CommandType::SetEnabled;
        command.boolValue = value;
        enqueue(std::move(command));
    }

    void setStrict(bool value)
    {
        Command command;
        command.type = CommandType::SetStrict;
        command.boolValue = value;
        enqueue(std::move(command));
    }

    void setLayout(const QString& layout)
    {
        Command command;
        command.type = CommandType::SetLayout;
        command.stringValue = layout;
        enqueue(std::move(command));
    }

    void cycleLayout()
    {
        Command command;
        command.type = CommandType::CycleLayout;
        enqueue(std::move(command));
    }

    void setDesktop(const QString& id, int index, int count)
    {
        Command command;
        command.type = CommandType::SetDesktop;
        command.stringValue = id;
        command.index = index;
        command.count = count;
        enqueue(std::move(command));
    }

    void setFloating(const QString& windowId, bool value)
    {
        Command command;
        command.type = CommandType::SetFloating;
        command.stringValue = windowId;
        command.boolValue = value;
        enqueue(std::move(command));
    }

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Impl* self = reinterpret_cast<Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<Impl*>(create->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            return TRUE;
        }
        if (!self)
            return DefWindowProcW(hwnd, message, wParam, lParam);
        return self->handleMessage(hwnd, message, wParam, lParam);
    }

    LRESULT handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message) {
        case kCommandMessage:
            drainCommands();
            return 0;
        case kWinEventMessage:
            handleWinEvent(static_cast<DWORD>(wParam), reinterpret_cast<HWND>(lParam));
            return 0;
        case kStopMessage:
            stopRequested = true;
            cleanupResources();
            PostQuitMessage(0);
            return 0;
        case WM_HOTKEY:
            handleHotkey(static_cast<int>(wParam));
            return 0;
        case kHookActionMessage:
            if (static_cast<int>(wParam) >= 0
                && static_cast<int>(wParam) < hookBindings.size()) {
                const HookBinding& binding = hookBindings.at(static_cast<int>(wParam));
                handleAction(binding.action, binding.argument);
            }
            return 0;
        case kDragMessage:
            handleDrag(wParam);
            return 0;
        case kLauncherMessage:
            if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM"))
                qInfo("WM bare Win -> launcher");
            handleAction(WmAction::Launcher, 0);
            return 0;
        case WM_TIMER:
            handleTimer(static_cast<UINT_PTR>(wParam));
            return 0;
        case WM_DISPLAYCHANGE:
        case WM_SETTINGCHANGE:
            scheduleRescan(250);
            return 0;
        case WM_CLOSE:
            if (hwnd == messageWindow)
                DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            if (hwnd == messageWindow)
                PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }

    void run()
    {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        timeBeginPeriod(1);
        instance = GetModuleHandleW(nullptr);

        WNDCLASSEXW windowClass = {};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.hInstance = instance;
        windowClass.lpfnWndProc = &Impl::windowProc;
        windowClass.lpszClassName = L"CaelestiaWindowManager";
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClassExW(&windowClass);

        messageWindow = CreateWindowExW(WS_EX_TOOLWINDOW, windowClass.lpszClassName,
            L"Caelestia Window Manager", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
            instance, this);
        if (!messageWindow) {
            error = QStringLiteral("Fenêtre de gestion des raccourcis indisponible");
            markReady();
            CoUninitialize();
            timeEndPeriod(1);
            return;
        }
        threadId = GetCurrentThreadId();

        CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_ALL,
            IID_PPV_ARGS(&desktopManager));

        installHooks();
        registerHotkeys();
        installKeyboardHook();
        installMouseHook();
        SetTimer(messageWindow, kRescanTimer, 1000, nullptr);

        g_messageWindow = messageWindow;
        running = true;
        markReady();
        publishState();

        MSG message = {};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        if (!cleaned)
            cleanupResources();
        if (messageWindow) {
            DestroyWindow(messageWindow);
            messageWindow = nullptr;
        }
        running = false;
        g_messageWindow = nullptr;
        CoUninitialize();
        timeEndPeriod(1);
    }

    void markReady()
    {
        ready = true;
        readyCv.notify_all();
    }

    void drainCommands()
    {
        std::deque<Command> pending;
        {
            std::lock_guard lock(queueMutex);
            pending.swap(queue);
        }
        for (const Command& command : pending) {
            switch (command.type) {
            case CommandType::SetEnabled:
                applyEnabled(command.boolValue);
                break;
            case CommandType::SetStrict:
                strictMode = command.boolValue;
                if (strictMode)
                    freeMoved.clear();
                if (enabled)
                    scheduleRescan(100);
                publishState();
                break;
            case CommandType::SetLayout: {
                const TilingEngine::Layout layout = TilingEngine::layoutFromName(command.stringValue);
                defaultLayout = TilingEngine::layoutName(layout);
                freeMoved.clear();
                for (auto& monitor : monitors)
                    monitor->engine.setLayout(layout);
                if (enabled)
                    scheduleRescan(80);
                publishState();
                break;
            }
            case CommandType::CycleLayout: {
                const TilingEngine::Layout current = TilingEngine::layoutFromName(defaultLayout);
                TilingEngine::Layout next = TilingEngine::Layout::Dwindle;
                switch (current) {
                case TilingEngine::Layout::Dwindle:
                    next = TilingEngine::Layout::Master;
                    break;
                case TilingEngine::Layout::Master:
                    next = TilingEngine::Layout::SplitHorizontal;
                    break;
                case TilingEngine::Layout::SplitHorizontal:
                    next = TilingEngine::Layout::SplitVertical;
                    break;
                case TilingEngine::Layout::SplitVertical:
                    next = TilingEngine::Layout::Dwindle;
                    break;
                }
                defaultLayout = TilingEngine::layoutName(next);
                for (auto& monitor : monitors)
                    monitor->engine.setLayout(next);
                if (enabled)
                    scheduleRescan(80);
                publishState();
                break;
            }
            case CommandType::SetDesktop:
                applyDesktop(command.stringValue, command.index, command.count);
                break;
            case CommandType::SetFloating:
                applyFloating(command.stringValue.toULongLong(), command.boolValue);
                break;
            }
        }
    }

    void applyDesktop(const QString& id, int index, int count)
    {
        if (id.isEmpty())
            return;
        if (id == currentDesktopId && count == desktopCount && !monitors.isEmpty()) {
            if (enabled)
                scheduleRescan(80);
            return;
        }

        if (fullscreen.valid)
            exitFullscreen();
        if (!currentDesktopId.isEmpty())
            saveCurrentLayouts();

        // A desktop change slides its windows into place. The direction comes
        // from the column in the bar: going down means the new desktop enters
        // from below, going up from above.
        if (desktopIndex >= 0 && index >= 0 && index != desktopIndex) {
            slideDirection = index > desktopIndex ? 1 : -1;
            slidePending = true;
            slideConsumed = false;
            slideArmedAt = nowMs();
        }

        currentDesktopId = id;
        desktopIndex = index;
        desktopCount = count;
        activeWindow = nullptr;
        fullscreenWindow = nullptr;
        fullscreen = {};
        monitors.clear();

        const auto saved = savedLayouts.value(id);
        for (auto it = saved.constBegin(); it != saved.constEnd(); ++it) {
            auto state = std::make_shared<MonitorState>();
            state->key = it.key();
            state->engine.restore(it.value());
            state->engine.setInnerGap(5);
            monitors.insert(state->key, state);
        }

        if (enabled) {
            // Tight: the switch itself is instantaneous as far as Explorer is
            // concerned, so the slide has to follow within a few frames or it
            // reads as a delayed twitch instead of a transition.
            scheduleRescan(60);
        } else {
            publishState();
        }
    }

    void saveCurrentLayouts()
    {
        if (currentDesktopId.isEmpty() || monitors.isEmpty())
            return;
        QHash<QString, QVariantMap> saved;
        for (auto it = monitors.constBegin(); it != monitors.constEnd(); ++it)
            saved.insert(it.key(), it.value()->engine.save());
        savedLayouts.insert(currentDesktopId, saved);
    }

    void scheduleRescan(int delay = 120)
    {
        if (!messageWindow || !enabled)
            return;
        KillTimer(messageWindow, kDebounceTimer);
        SetTimer(messageWindow, kDebounceTimer, static_cast<UINT>(qMax(1, delay)), nullptr);
    }

    void handleTimer(UINT_PTR timer)
    {
        switch (timer) {
        case kAnimationTimer:
            tickAnimations();
            break;
        case kRescanTimer:
            if (enabled && !currentDesktopId.isEmpty())
                rescan();
            break;
        case kDebounceTimer:
            KillTimer(messageWindow, kDebounceTimer);
            if (enabled && !currentDesktopId.isEmpty())
                rescan();
            break;
        case kFocusTimer:
            KillTimer(messageWindow, kFocusTimer);
            if (focusAfterClose && IsWindow(focusAfterClose)
                && isWindowOnCurrentDesktop(focusAfterClose))
                focusWindow(focusAfterClose);
            closingWindow = nullptr;
            focusAfterClose = nullptr;
            break;
        default:
            break;
        }
    }

    void installHooks()
    {
        const auto install = [this](DWORD minimum, DWORD maximum) {
            HWINEVENTHOOK hook = SetWinEventHook(minimum, maximum, nullptr, winEventProc,
                0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
            if (hook)
                hooks.append(hook);
        };
        install(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND);
        install(EVENT_OBJECT_CREATE, EVENT_OBJECT_HIDE);
        install(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE);
        install(EVENT_SYSTEM_MOVESIZESTART, EVENT_SYSTEM_MINIMIZEEND);
        install(EVENT_OBJECT_CLOAKED, EVENT_OBJECT_UNCLOAKED);
    }

    void uninstallHooks()
    {
        for (HWINEVENTHOOK hook : hooks)
            UnhookWinEvent(hook);
        hooks.clear();
    }

    void registerHotkeys()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope,
            QStringLiteral("Caelestia"), QStringLiteral("window-manager"));
        if (settings.contains(QStringLiteral("layout"))) {
            const QString configured = settings.value(QStringLiteral("layout")).toString();
            const TilingEngine::Layout layout = TilingEngine::layoutFromName(configured);
            defaultLayout = TilingEngine::layoutName(layout);
        }
        if (settings.contains(QStringLiteral("animations")))
            animationsEnabled = settings.value(QStringLiteral("animations"), true).toBool();
        if (settings.contains(QStringLiteral("strict")))
            strictMode = settings.value(QStringLiteral("strict"), true).toBool();
        if (settings.contains(QStringLiteral("tiling")))
            tilingEnabled = settings.value(QStringLiteral("tiling"), false).toBool();

        QVector<BindingSpec> specs = defaultBindingSpecs(tilingEnabled);
        QVector<BindingSpec> customSpecs;
        for (const BindingSpec& spec : std::as_const(specs)) {
            if (spec.original)
                continue;
            const QString value = settings.value(
                QStringLiteral("bindings/") + spec.name).toString();
            if (value.isEmpty())
                continue;
            customSpecs.append({ spec.action, spec.argument, spec.name, value, false, true });
        }
        specs = customSpecs + specs;

        QSet<QString> registeredKeys;
        QSet<QString> hookKeys;
        int fallbackFailures = 0;
        for (const BindingSpec& spec : std::as_const(specs)) {
            const ParsedKey key = parseKeyBinding(spec.key);
            if (!key.valid) {
                if (!spec.original)
                    ++fallbackFailures;
                continue;
            }
            const QString canonical = QString::number(key.mods) + QLatin1Char(':')
                + QString::number(key.vk);
            if (registeredKeys.contains(canonical)) {
                ActiveBinding duplicate;
                duplicate.action = spec.action;
                duplicate.argument = spec.argument;
                duplicate.name = spec.name;
                duplicate.key = spec.key;
                duplicate.original = spec.original;
                duplicate.custom = spec.custom;
                duplicate.registered = false;
                activeBindings.append(duplicate);
                continue;
            }

            const int id = kHotkeyBase + activeBindings.size();
            const BOOL ok = RegisterHotKey(messageWindow, id, key.mods | MOD_NOREPEAT, key.vk);
            ActiveBinding binding;
            binding.action = spec.action;
            binding.argument = spec.argument;
            binding.name = spec.name;
            binding.key = spec.key;
            binding.original = spec.original;
            binding.custom = spec.custom;
            binding.id = id;
            binding.registered = ok;
            if (ok) {
                registeredKeys.insert(canonical);
                hotkeyMap.insert(id, activeBindings.size());
            } else if (!spec.original) {
                ++fallbackFailures;
                if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM")) {
                    qInfo().noquote() << "WM hotkey failed" << spec.name << spec.key
                                      << "error" << GetLastError();
                }
            }
            // A Win combination Explorer owns cannot be registered, but the
            // low-level hook can still deliver it. Only combinations Windows
            // reserves need this; the Ctrl+Alt+… fallbacks register normally.
            if (!ok && (key.mods & MOD_WIN) && !hookKeys.contains(canonical)) {
                hookKeys.insert(canonical);
                hookBindings.append({ key.mods, key.vk, spec.action, spec.argument, spec.name });
                binding.registered = true;
            }
            activeBindings.append(binding);
        }

        if (fallbackFailures > 0)
            error = QStringLiteral("%1 raccourci(s) de secours indisponible(s)")
                        .arg(fallbackFailures);

        publishedBindings.clear();
        for (const ActiveBinding& binding : std::as_const(activeBindings)) {
            QVariantMap item;
            item.insert(QStringLiteral("action"), binding.name);
            item.insert(QStringLiteral("key"), binding.key);
            item.insert(QStringLiteral("registered"), binding.registered);
            item.insert(QStringLiteral("original"), binding.original);
            item.insert(QStringLiteral("custom"), binding.custom);
            publishedBindings.append(item);
        }

        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM")) {
            for (const ActiveBinding& binding : std::as_const(activeBindings)) {
                qInfo().noquote() << "WM binding" << binding.name << binding.key
                                  << (binding.registered ? "registered" : "unavailable");
            }
            qInfo().noquote() << "WM layout" << defaultLayout << "strict" << strictMode
                              << "enabled" << enabled;
        }
    }

    void unregisterHotkeys()
    {
        for (const ActiveBinding& binding : std::as_const(activeBindings)) {
            if (binding.registered && binding.id != 0)
                UnregisterHotKey(messageWindow, binding.id);
        }
        activeBindings.clear();
        hotkeyMap.clear();
    }

    void installKeyboardHook()
    {
        if (keyboardHook || hookBindings.isEmpty())
            return;

        s_hookImpl = this;
        keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, &Impl::keyboardProc,
            GetModuleHandleW(nullptr), 0);
        if (!keyboardHook) {
            if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM")) {
                qInfo().noquote() << "WM keyboard hook failed, error" << GetLastError();
            }
            s_hookImpl = nullptr;
        } else if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM")) {
            qInfo().noquote() << "WM keyboard hook installed for"
                              << hookBindings.size() << "Win combinations";
        }
    }

    void uninstallKeyboardHook()
    {
        if (keyboardHook) {
            UnhookWindowsHookEx(keyboardHook);
            keyboardHook = nullptr;
        }
        s_hookImpl = nullptr;
        hookHeldKeys.clear();
        winDown = false;
        winAlone = false;
    }

    static LRESULT CALLBACK keyboardProc(int code, WPARAM wParam, LPARAM lParam)
    {
        Impl* self = s_hookImpl;
        if (code == HC_ACTION && self
            && self->consumeKey(static_cast<int>(wParam),
                   reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam))) {
            return 1;
        }
        return CallNextHookEx(nullptr, code, wParam, lParam);
    }

    // Hyprland's Super + left button: the window under the cursor moves with
    // the pointer. The gesture is claimed so the click never reaches the app.
    void installMouseHook()
    {
        if (mouseHook)
            return;

        s_hookImpl = this;
        mouseHook = SetWindowsHookExW(WH_MOUSE_LL, &Impl::mouseProc,
            GetModuleHandleW(nullptr), 0);
        if (!mouseHook && !qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM")) {
            qInfo().noquote() << "WM mouse hook failed, error" << GetLastError();
        }
    }

    void uninstallMouseHook()
    {
        if (mouseHook) {
            UnhookWindowsHookEx(mouseHook);
            mouseHook = nullptr;
        }
        dragActive = false;
        dragWindow = nullptr;
    }

    static LRESULT CALLBACK mouseProc(int code, WPARAM wParam, LPARAM lParam)
    {
        Impl* self = s_hookImpl;
        if (code == HC_ACTION && self
            && self->handleMouse(static_cast<UINT>(wParam),
                   *reinterpret_cast<const MSLLHOOKSTRUCT*>(lParam))) {
            return 1;
        }
        return CallNextHookEx(nullptr, code, wParam, lParam);
    }

    // Runs inside the hook, on the window-manager thread, so it only records
    // the gesture and the cursor; every window operation is posted to the
    // message loop to stay clear of the low-level hook timeout.
    bool handleMouse(UINT message, const MSLLHOOKSTRUCT& info)
    {
        // A click while Windows is held turns the key into a combination; the
        // release must not open the launcher afterwards.
        if (winDown)
            winAlone = false;

        if (dragActive) {
            if (message == WM_MOUSEMOVE) {
                dragCursor = info.pt;
                PostMessageW(messageWindow, kDragMessage, kDragMove, 0);
                return true;
            }
            if (message == WM_LBUTTONUP) {
                dragActive = false;
                PostMessageW(messageWindow, kDragMessage, kDragEnd, 0);
                return true;
            }
            // A second press or a wheel turn during the drag must not reach
            // the application either.
            return message == WM_LBUTTONDOWN || message == WM_MOUSEWHEEL;
        }

        if (message != WM_LBUTTONDOWN || !winDown)
            return false;

        HWND target = WindowFromPoint(info.pt);
        if (!target)
            return false;
        target = GetAncestor(target, GA_ROOT);
        if (!target || !isEligible(target))
            return false;

        dragActive = true;
        dragWindow = target;
        dragGrab = info.pt;
        dragCursor = info.pt;
        PostMessageW(messageWindow, kDragMessage, kDragBegin, 0);
        return true;
    }

    void handleDrag(WPARAM phase)
    {
        HWND hwnd = dragWindow;
        if (!hwnd || !IsWindow(hwnd)) {
            endDrag();
            return;
        }

        switch (phase) {
        case kDragBegin: {
            if (!isEligible(hwnd)) {
                endDrag();
                return;
            }
            if (fullscreenWindow == hwnd)
                exitFullscreen();
            saveWindowState(hwnd);
            const QRect rect = windowRect(hwnd);
            dragOffset = QPoint(dragGrab.x - rect.x(), dragGrab.y - rect.y());
            // Dragging is an explicit placement: the window leaves the tiling
            // layout and keeps the spot it was dropped on, exactly like a
            // tiled window dragged in Hyprland becomes floating.
            const quintptr value = reinterpret_cast<quintptr>(hwnd);
            floating.insert(value);
            animations.remove(value);
            freeMoved.remove(value);
            focusWindow(hwnd);
            publishState();
            if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM")) {
                qInfo().noquote() << "WM drag" << value;
            }
            break;
        }
        case kDragMove:
            SetWindowPos(hwnd, nullptr, dragCursor.x - dragOffset.x(),
                dragCursor.y - dragOffset.y(), 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            break;
        default:
            endDrag();
            break;
        }
    }

    void endDrag()
    {
        dragActive = false;
        dragWindow = nullptr;
        scheduleRescan(120);
    }

    // Called from the hook, on the window-manager thread before the key enters
    // any message queue. Returns true when the shell claims the key.
    bool consumeKey(int message, const KBDLLHOOKSTRUCT* info)
    {
        if (!info)
            return false;

        const UINT vk = info->vkCode;
        const bool up = message == WM_KEYUP || message == WM_SYSKEYUP;

        // Repeats and the key-up of a combination already claimed: swallow
        // them so neither Explorer nor the focused app sees a stray key.
        if (hookHeldKeys.contains(vk)) {
            if (up)
                hookHeldKeys.remove(vk);
            return true;
        }

        if (vk == VK_LWIN || vk == VK_RWIN) {
            // The shell owns the Windows key: press and release are both
            // swallowed, so Windows never sees the modifier at all. Letting
            // the press through and claiming only the release — what the
            // first version did — left the system's keyboard state with Win
            // still down, and the next bare key arrived as Win+key: typing "s"
            // opened the Start menu search.
            if (up) {
                const bool bare = winDown && winAlone && enabled;
                winDown = false;
                winAlone = false;
                if (bare)
                    PostMessageW(messageWindow, kLauncherMessage, 0, 0);
                return true;
            }
            if (!winDown) {
                winDown = true;
                // Windows held together with another modifier is a
                // combination, not a tap.
                winAlone = !(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                    && !(GetAsyncKeyState(VK_MENU) & 0x8000)
                    && !(GetAsyncKeyState(VK_SHIFT) & 0x8000);
            }
            return true;
        }

        // Any other key means the Windows key is part of a combination.
        winAlone = false;

        if (up || !enabled || hookBindings.isEmpty())
            return false;

        const bool win = winDown;
        if (!win)
            return false;

        UINT mods = MOD_WIN;
        if (GetAsyncKeyState(VK_MENU) & 0x8000)
            mods |= MOD_ALT;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            mods |= MOD_CONTROL;
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            mods |= MOD_SHIFT;

        for (int i = 0; i < hookBindings.size(); ++i) {
            const HookBinding& binding = hookBindings.at(i);
            if (binding.vk != vk || binding.mods != mods)
                continue;
            hookHeldKeys.insert(vk);
            if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM")) {
                qInfo().noquote() << "WM hook claimed" << binding.name;
            }
            PostMessageW(messageWindow, kHookActionMessage,
                static_cast<WPARAM>(i), 0);
            return true;
        }
        return false;
    }

    void handleHotkey(int id)
    {
        const auto it = hotkeyMap.constFind(id);
        if (it == hotkeyMap.constEnd())
            return;
        const ActiveBinding& binding = activeBindings.at(it.value());
        handleAction(binding.action, binding.argument);
    }

    bool isOwnWindow(HWND hwnd) const
    {
        return hwnd == messageWindow;
    }

    bool isExternalWindow(HWND hwnd) const
    {
        if (!hwnd || !IsWindow(hwnd) || !IsWindowVisible(hwnd))
            return false;
        if (isOwnWindow(hwnd))
            return false;
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (!pid || pid == GetCurrentProcessId())
            return false;
        return !isShellWindowClass(windowClassName(hwnd));
    }

    bool isWindowOnCurrentDesktop(HWND hwnd) const
    {
        if (!desktopManager)
            return true;
        BOOL onCurrent = TRUE;
        if (FAILED(desktopManager->IsWindowOnCurrentVirtualDesktop(hwnd, &onCurrent)))
            return true;
        return onCurrent != FALSE;
    }

    // DWM cloaking is how Explorer hides the windows of inactive desktops.
    // Win32 visibility says nothing about it, which is why a slide seeded
    // during the switch would play invisibly.
    static bool isCloaked(HWND hwnd)
    {
        BOOL cloaked = FALSE;
        return SUCCEEDED(DwmGetWindowAttribute(
                   hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))
            && cloaked;
    }

    bool isEligible(HWND hwnd) const
    {        if (!isExternalWindow(hwnd))
            return false;
        if (GetAncestor(hwnd, GA_ROOT) != hwnd)
            return false;
        const LONG style = static_cast<LONG>(GetWindowLongPtrW(hwnd, GWL_STYLE));
        const LONG exStyle = static_cast<LONG>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
        if (style & WS_CHILD)
            return false;
        if (style & WS_DISABLED)
            return false;
        if (!(style & WS_THICKFRAME))
            return false;
        if (exStyle & (WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE
                | WS_EX_TRANSPARENT)) {
            return false;
        }
        if (GetWindow(hwnd, GW_OWNER))
            return false;
        if (IsIconic(hwnd))
            return false;
        if (GetWindowTextLengthW(hwnd) == 0)
            return false;
        const QString className = windowClassName(hwnd);
        if (className == QStringLiteral("#32770") || isShellWindowClass(className))
            return false;

        BOOL cloaked = FALSE;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))
            && cloaked) {
            return false;
        }
        if (!isWindowOnCurrentDesktop(hwnd))
            return false;
        return true;
    }

    void handleWinEvent(DWORD event, HWND hwnd)
    {
        switch (event) {
        case EVENT_SYSTEM_FOREGROUND:
            if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM"))
                qInfo().noquote() << "WM foreground event" << reinterpret_cast<quintptr>(hwnd);
            updateActiveWindow();
            break;
        case EVENT_OBJECT_CREATE:
        case EVENT_OBJECT_SHOW:
        case EVENT_SYSTEM_MINIMIZEEND:
            scheduleRescan(100);
            break;
        case EVENT_OBJECT_DESTROY:
            handleDestroyedWindow(hwnd);
            break;
        case EVENT_SYSTEM_MINIMIZESTART:
            if (fullscreenWindow == hwnd)
                exitFullscreen();
            removeWindowFromEngines(hwnd);
            scheduleRescan(80);
            break;
        case EVENT_SYSTEM_MOVESIZESTART:
            interacting = true;
            break;
        case EVENT_SYSTEM_MOVESIZEEND:
            interacting = false;
            scheduleRescan(80);
            break;
        case EVENT_OBJECT_LOCATIONCHANGE:
            if (hwnd == activeWindow)
            onLocationChange(hwnd);
            break;
        case EVENT_OBJECT_CLOAKED:
        case EVENT_OBJECT_UNCLOAKED:
            scheduleRescan(120);
            break;
        default:
            break;
        }
    }

    void updateActiveWindow()
    {
        HWND foreground = GetForegroundWindow();
        if (isExternalWindow(foreground)) {
            activeWindow = foreground;
        } else if (activeWindow && (!IsWindow(activeWindow)
                       || !IsWindowVisible(activeWindow)
                       || !isWindowOnCurrentDesktop(activeWindow))) {
            activeWindow = nullptr;
        }

        if (activeWindow) {
            for (auto& state : monitors) {
                if (state->engine.contains(reinterpret_cast<quintptr>(activeWindow)))
                    state->engine.setFocused(reinterpret_cast<quintptr>(activeWindow));
            }
        }
        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM")) {
            qInfo().noquote() << "WM focus"
                              << (activeWindow ? QString::number(
                                                     reinterpret_cast<quintptr>(activeWindow))
                                               : QStringLiteral("<none>"));
        }
    }

    void handleDestroyedWindow(HWND hwnd)
    {
        const quintptr value = reinterpret_cast<quintptr>(hwnd);
        if (fullscreenWindow == hwnd)
            fullscreen = {}, fullscreenWindow = nullptr;
        for (auto& state : monitors)
            state->engine.removeWindow(value);
        savedWindows.remove(value);
        floating.remove(value);
        freeMoved.remove(value);
        animations.remove(value);
        suppressUntil.remove(value);
        if (activeWindow == hwnd)
            activeWindow = nullptr;
        if (closingWindow == hwnd && focusAfterClose) {
            KillTimer(messageWindow, kFocusTimer);
            SetTimer(messageWindow, kFocusTimer, 180, nullptr);
        }
        scheduleRescan(80);
    }

    void onLocationChange(HWND hwnd)
    {
        if (!enabled || interacting || !animations.isEmpty())
            return;
        if (!hwnd || !IsWindow(hwnd) || IsIconic(hwnd))
            return;
        const quintptr value = reinterpret_cast<quintptr>(hwnd);
        if (floating.contains(value) || fullscreenWindow == hwnd)
            return;
        if (nowMs() < suppressUntil.value(value, 0))
            return;
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
            return;

        const QString key = monitorFor(hwnd).key;
        const auto stateIt = monitors.constFind(key);
        if (stateIt == monitors.constEnd())
            return;
        const MonitorState& state = *stateIt.value();
        if (!state.engine.contains(value))
            return;

        const QRect target = state.engine.geometryFor(contentRect(state.work,
                                            state.engine.count()),
            value);
        if (!target.isValid())
            return;
        const QRect actual = windowRect(hwnd);
        if (qAbs(actual.x() - target.x()) <= 2 && qAbs(actual.y() - target.y()) <= 2
            && qAbs(actual.width() - target.width()) <= 2
            && qAbs(actual.height() - target.height()) <= 2) {
            return;
        }

        if (!strictMode) {
            freeMoved.insert(value);
            if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM")) {
                qInfo().noquote() << "WM free move" << value;
            }
            return;
        }
        scheduleRescan(120);
    }

    void removeWindowFromEngines(HWND hwnd)
    {
        const quintptr value = reinterpret_cast<quintptr>(hwnd);
        for (auto& state : monitors)
            state->engine.removeWindow(value);
        animations.remove(value);
        suppressUntil.remove(value);
    }

    void rescan()
    {
        if (!enabled || currentDesktopId.isEmpty()) {
            return;
        }
        updateActiveWindow();

        QVector<HWND> eligible;
        enumTarget = &eligible;
        EnumWindows(
            [](HWND hwnd, LPARAM data) -> BOOL {
                auto* self = reinterpret_cast<Impl*>(data);
                if (!self->enumTarget)
                    return TRUE;
                const bool fullscreen = self->fullscreenWindow == hwnd;
                if ((self->isEligible(hwnd) || fullscreen)
                    && !self->floating.contains(reinterpret_cast<quintptr>(hwnd))) {
                    self->enumTarget->append(hwnd);
                }
                return TRUE;
            },
            reinterpret_cast<LPARAM>(this));
        enumTarget = nullptr;

        QVector<HWND> ordered;
        if (activeWindow && eligible.contains(activeWindow))
            ordered.append(activeWindow);
        for (HWND hwnd : std::as_const(eligible)) {
            if (hwnd != activeWindow)
                ordered.append(hwnd);
        }

        // Every managed window loses its caption buttons. It runs before the
        // geometry pass so the frame change is already in place when the tiler
        // measures the window.
        for (HWND hwnd : std::as_const(eligible)) {
            saveWindowState(hwnd);
            stripCaptionButtons(hwnd);
        }

        QHash<QString, QVector<HWND>> byMonitor;
        QHash<QString, QRect> workAreas;
        for (HWND hwnd : std::as_const(ordered)) {
            const MonitorInfo info = monitorFor(hwnd);
            if (!info.valid)
                continue;
            workAreas.insert(info.key, info.work);
            byMonitor[info.key].append(hwnd);
        }

        for (auto it = monitors.begin(); it != monitors.end();) {
            if (!byMonitor.contains(it.key()))
                it = monitors.erase(it);
            else
                ++it;
        }

        for (auto mit = byMonitor.begin(); mit != byMonitor.end(); ++mit) {
            std::shared_ptr<MonitorState> state = monitors.value(mit.key());
            if (!state) {
                state = std::make_shared<MonitorState>();
                state->key = mit.key();
                state->engine.setLayout(TilingEngine::layoutFromName(defaultLayout));
                state->engine.setInnerGap(5);
                monitors.insert(mit.key(), state);
            }
            state->work = workAreas.value(mit.key(), monitorFor(nullptr).work);

            const QVector<HWND>& wanted = mit.value();
            QSet<quintptr> wantedSet;
            for (HWND hwnd : wanted)
                wantedSet.insert(reinterpret_cast<quintptr>(hwnd));

            for (quintptr hwnd : state->engine.windows()) {
                if (!wantedSet.contains(hwnd)) {
                    state->engine.removeWindow(hwnd);
                    freeMoved.remove(hwnd);
                }
            }
            for (auto& other : monitors) {
                if (other == state)
                    continue;
                for (HWND hwnd : wanted) {
                    const quintptr value = reinterpret_cast<quintptr>(hwnd);
                    if (other->engine.contains(value))
                        other->engine.removeWindow(value);
                }
            }
            for (HWND hwnd : wanted) {
                const quintptr value = reinterpret_cast<quintptr>(hwnd);
                if (!state->engine.contains(value))
                    state->engine.addWindow(value, true);
            }
            if (activeWindow
                && state->engine.contains(reinterpret_cast<quintptr>(activeWindow))) {
                state->engine.setFocused(reinterpret_cast<quintptr>(activeWindow));
            }
            applyMonitor(*state);
        }

        // The slide stays armed until a pass actually moved something: the
        // rescan fired right after the switch often runs while the new
        // desktop's windows are still cloaked, and dropping the flag there
        // meant the slide never happened at all. It also expires, so a later
        // unrelated rescan cannot resurrect it.
        if (slidePending && (slideConsumed || nowMs() - slideArmedAt > 1500))
            slidePending = false;

        publishState();
    }

    void applyMonitor(MonitorState& state)
    {
        if (!enabled || state.engine.empty())
            return;
        const QRect content = contentRect(state.work, state.engine.count());
        const QHash<quintptr, QRect> planned = state.engine.geometry(content);
        // A pending desktop slide starts every window one screen to the side
        // of its tile and lets the animation sweep it in, even when the tile
        // itself did not change. Forward means the incoming desktop arrives
        // from the right, backward from the left — the Hyprland sweep the
        // reference recording shows.
        const bool slide = slidePending;
        const int slideOffset = slide
            ? slideDirection * (state.work.width() + 40) : 0;
        for (auto it = planned.constBegin(); it != planned.constEnd(); ++it) {
            HWND hwnd = reinterpret_cast<HWND>(it.key());
            if (!hwnd || !IsWindow(hwnd) || IsIconic(hwnd))
                continue;
            if (fullscreenWindow == hwnd || floating.contains(it.key())
                || freeMoved.contains(it.key())) {
                continue;
            }
            if (!isEligible(hwnd))
                continue;
            // A window of a desktop that has just become active is still
            // cloaked by DWM for a moment: Win32 calls it visible, DWM does
            // not paint it. Seeding the slide now would play it behind the
            // cloak and the window would simply appear in place afterwards.
            if (slide && isCloaked(hwnd))
                continue;
            saveWindowState(hwnd);
            // Un-maximising is part of tiling: it hands the window to the
            // layout. Without tiling the window must keep the state Windows
            // gave it.
            if (tilingEnabled && IsZoomed(hwnd))
                ShowWindow(hwnd, SW_RESTORE);

            const QRect actual = windowRect(hwnd);
            // Without tiling the layout is never applied: each window's own
            // rect is its target, so the desktop slide is the only movement
            // this pass can produce.
            const QRect target = tilingEnabled ? it.value() : actual;
            if (!slide && qAbs(actual.x() - target.x()) <= 1
                && qAbs(actual.y() - target.y()) <= 1
                && qAbs(actual.width() - target.width()) <= 1
                && qAbs(actual.height() - target.height()) <= 1) {
                continue;
            }

            if (!animationsEnabled) {
                setWindowRect(hwnd, target);
                continue;
            }
            Animation animation;
            animation.from = slide ? target.translated(slideOffset, 0) : actual;
            animation.to = target;
            animation.start = nowMs();
            if (slide)
                animation.duration = kDesktopSlideDurationMs;
            animations.insert(it.key(), animation);
            if (slide) {
                // The windows of a desktop that has just become active are
                // still cloaked for a moment; moving one is what proves the
                // slide really started.
                setWindowRect(hwnd, animation.from);
                slideConsumed = true;
            }
            if (!animationTimerRunning) {
                SetTimer(messageWindow, kAnimationTimer, kAnimationTickMs, nullptr);
                animationTimerRunning = true;
            }
        }
    }

    void tickAnimations()
    {
        if (animations.isEmpty()) {
            if (animationTimerRunning) {
                KillTimer(messageWindow, kAnimationTimer);
                animationTimerRunning = false;
            }
            return;
        }

        const qint64 now = nowMs();
        QVector<quintptr> finished;
        for (auto it = animations.begin(); it != animations.end(); ++it) {
            HWND hwnd = reinterpret_cast<HWND>(it.key());
            if (!hwnd || !IsWindow(hwnd) || IsIconic(hwnd)) {
                finished.append(it.key());
                continue;
            }
            const qreal progress = qBound(0.0,
                static_cast<qreal>(now - it.value().start) / qMax(1, it.value().duration),
                1.0);
            // Ease-out: the sweep spends its first third covering most of the
            // screen and then settles, exactly like the reference recording.
            const qreal eased = 1.0 - std::pow(1.0 - progress, 3.0);
            const QRect from = it.value().from;
            const QRect to = it.value().to;
            const QRect current(
                qRound(from.x() + (to.x() - from.x()) * eased),
                qRound(from.y() + (to.y() - from.y()) * eased),
                qRound(from.width() + (to.width() - from.width()) * eased),
                qRound(from.height() + (to.height() - from.height()) * eased));
            setWindowRect(hwnd, current);
            if (hwnd == activeWindow)
            if (progress >= 1.0)
                finished.append(it.key());
        }
        for (quintptr hwnd : std::as_const(finished))
            animations.remove(hwnd);
        DwmFlush();
        if (animations.isEmpty() && animationTimerRunning) {
            KillTimer(messageWindow, kAnimationTimer);
            animationTimerRunning = false;
        }
    }

    void setWindowRect(HWND hwnd, const QRect& rect)
    {
        if (!hwnd || !IsWindow(hwnd) || rect.width() <= 0 || rect.height() <= 0)
            return;
        suppressUntil.insert(reinterpret_cast<quintptr>(hwnd), nowMs() + 250);
        SetWindowPos(hwnd, nullptr, rect.x(), rect.y(), rect.width(), rect.height(),
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }

    // The shell closes windows with Win+C and tiles them itself, so the three
    // caption buttons are dead weight and a way to fight the layout. Clearing
    // the matching style bits removes them from the title bar; WS_CAPTION
    // stays (the title bar still drags) and so does WS_THICKFRAME, which the
    // window needs to stay resizable and eligible. The original style is kept
    // in savedWindows, so disabling the manager gives the buttons back.
    void stripCaptionButtons(HWND hwnd)
    {
        const LONG style = static_cast<LONG>(GetWindowLongPtrW(hwnd, GWL_STYLE));
        const LONG wanted = style
            & ~(WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        if (wanted == style)
            return;
        SetWindowLongPtrW(hwnd, GWL_STYLE, wanted);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    void saveWindowState(HWND hwnd)
    {
        const quintptr value = reinterpret_cast<quintptr>(hwnd);
        if (savedWindows.contains(value))
            return;
        SavedWindow saved;
        saved.placement.length = sizeof(saved.placement);
        GetWindowPlacement(hwnd, &saved.placement);
        saved.style = static_cast<LONG>(GetWindowLongPtrW(hwnd, GWL_STYLE));
        saved.exStyle = static_cast<LONG>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
        saved.rect = windowRect(hwnd);
        saved.valid = true;
        savedWindows.insert(value, saved);
    }

    void restoreAllWindows()
    {
        const auto keys = savedWindows.keys();
        for (quintptr value : keys) {
            HWND hwnd = reinterpret_cast<HWND>(value);
            if (!IsWindow(hwnd))
                continue;
            const SavedWindow saved = savedWindows.value(value);
            if (!saved.valid)
                continue;
            SetWindowLongPtrW(hwnd, GWL_STYLE, saved.style);
            SetWindowLongPtrW(hwnd, GWL_EXSTYLE, saved.exStyle);
            if (!SetWindowPlacement(hwnd, &saved.placement)) {
                if (saved.rect.isValid()) {
                    SetWindowPos(hwnd, nullptr, saved.rect.x(), saved.rect.y(),
                        saved.rect.width(), saved.rect.height(),
                        SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
                }
            } else {
                SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
                        | SWP_FRAMECHANGED);
            }
        }
        savedWindows.clear();
        floating.clear();
        freeMoved.clear();
        animations.clear();
        suppressUntil.clear();
        if (animationTimerRunning) {
            KillTimer(messageWindow, kAnimationTimer);
            animationTimerRunning = false;
        }
    }

    void applyEnabled(bool value)
    {
        if (enabled == value)
            return;
        enabled = value;
        if (!enabled) {
            if (fullscreen.valid)
                exitFullscreen();
            restoreAllWindows();
            for (auto& state : monitors)
                state->engine.clear();
            savedLayouts.clear();
            publishState();
            if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM"))
                qInfo("WM disabled; original window geometry restored");
            return;
        }
        freeMoved.clear();
        scheduleRescan(400);
        publishState();
        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM"))
            qInfo("WM enabled");
    }

    void setEnabledOnWorker(bool value) { applyEnabled(value); }

    void focusWindow(HWND hwnd)
    {
        if (!isExternalWindow(hwnd))
            return;
        if (IsIconic(hwnd))
            ShowWindow(hwnd, SW_RESTORE);

        const HWND previous = GetForegroundWindow();
        if (previous != hwnd) {
            DWORD foregroundThread = GetWindowThreadProcessId(previous, nullptr);
            DWORD targetThread = GetWindowThreadProcessId(hwnd, nullptr);
            const bool attached = foregroundThread && targetThread
                && foregroundThread != targetThread
                && AttachThreadInput(foregroundThread, GetCurrentThreadId(), TRUE);
            SetForegroundWindow(hwnd);
            if (attached)
                AttachThreadInput(foregroundThread, GetCurrentThreadId(), FALSE);

            if (GetForegroundWindow() != hwnd) {
                // A hidden hotkey window is not always allowed to take the
                // foreground; this is the documented Win32 fallback used by
                // task switchers.
                using SwitchToThisWindowFn = void(WINAPI*)(HWND, BOOL);
                if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
                    auto* switchWindow = reinterpret_cast<SwitchToThisWindowFn>(
                        GetProcAddress(user32, "SwitchToThisWindow"));
                    if (switchWindow)
                        switchWindow(hwnd, TRUE);
                }
            }

            if (GetForegroundWindow() != hwnd) {
                keybd_event(VK_MENU, 0, 0, 0);
                SetForegroundWindow(hwnd);
                keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
            }
        }

        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM")) {
            qInfo().noquote() << "WM focusWindow" << reinterpret_cast<quintptr>(hwnd)
                              << "previous" << reinterpret_cast<quintptr>(previous)
                              << "now" << reinterpret_cast<quintptr>(GetForegroundWindow())
                              << "error" << GetLastError();
        }

        activeWindow = hwnd;
        for (auto& state : monitors) {
            if (state->engine.contains(reinterpret_cast<quintptr>(hwnd)))
                state->engine.setFocused(reinterpret_cast<quintptr>(hwnd));
        }
    }

    void toggleFloating()
    {
        if (!activeWindow || fullscreenWindow == activeWindow)
            return;
        const quintptr value = reinterpret_cast<quintptr>(activeWindow);
        if (floating.contains(value)) {
            floating.remove(value);
        } else {
            bool contained = false;
            for (auto& state : monitors) {
                if (state->engine.contains(value)) {
                    state->engine.removeWindow(value);
                    contained = true;
                }
            }
            if (!contained)
                return;
            floating.insert(value);
        }
        freeMoved.remove(value);
        if (enabled)
            scheduleRescan(80);
        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM")) {
            qInfo().noquote() << "WM floating" << value
                              << (floating.contains(value) ? "on" : "off");
        }
        publishState();
    }

    // Same effect as toggleFloating, but addressed by HWND so a panel can act
    // on the window it describes instead of whatever the WM considers active.
    void applyFloating(quintptr hwndValue, bool floatValue)
    {
        const HWND hwnd = reinterpret_cast<HWND>(hwndValue);
        if (!hwndValue || !IsWindow(hwnd) || fullscreenWindow == hwnd)
            return;

        if (floatValue) {
            if (floating.contains(hwndValue))
                return;
            for (auto& state : monitors)
                state->engine.removeWindow(hwndValue);
            floating.insert(hwndValue);
        } else {
            if (!floating.remove(hwndValue))
                return;
        }
        freeMoved.remove(hwndValue);
        if (enabled)
            scheduleRescan(80);
        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM")) {
            qInfo().noquote() << "WM floating" << hwndValue
                              << (floating.contains(hwndValue) ? "on" : "off");
        }
        publishState();
    }

    void toggleFullscreen()
    {
        if (!activeWindow || !isExternalWindow(activeWindow))
            return;
        if (fullscreenWindow == activeWindow) {
            exitFullscreen();
            return;
        }
        if (fullscreen.valid)
            exitFullscreen();

        FullscreenState state;
        state.hwnd = activeWindow;
        state.style = static_cast<LONG>(GetWindowLongPtrW(activeWindow, GWL_STYLE));
        state.exStyle = static_cast<LONG>(GetWindowLongPtrW(activeWindow, GWL_EXSTYLE));
        state.wasTopmost = (state.exStyle & WS_EX_TOPMOST) != 0;
        state.placement.length = sizeof(state.placement);
        GetWindowPlacement(activeWindow, &state.placement);
        state.rect = windowRect(activeWindow);

        const MonitorInfo monitor = monitorFor(activeWindow);
        if (!monitor.valid)
            return;

        saveWindowState(activeWindow);
        SetWindowLongPtrW(activeWindow, GWL_STYLE,
            state.style
                & ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX
                    | WS_MAXIMIZEBOX));
        SetWindowLongPtrW(activeWindow, GWL_EXSTYLE, state.exStyle | WS_EX_TOPMOST);
        SetWindowPos(activeWindow, HWND_TOPMOST, monitor.bounds.x(), monitor.bounds.y(),
            monitor.bounds.width(), monitor.bounds.height(),
            SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        fullscreen = state;
        fullscreen.valid = true;
        fullscreenWindow = activeWindow;
        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM"))
            qInfo().noquote() << "WM fullscreen" << reinterpret_cast<quintptr>(activeWindow);
    }

    void exitFullscreen()
    {
        if (!fullscreen.valid || !fullscreenWindow)
            return;
        HWND hwnd = fullscreenWindow;
        if (IsWindow(hwnd)) {
            SetWindowLongPtrW(hwnd, GWL_STYLE, fullscreen.style);
            SetWindowLongPtrW(hwnd, GWL_EXSTYLE, fullscreen.exStyle);
            if (!fullscreen.wasTopmost) {
                SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            } else {
                SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
                        | SWP_FRAMECHANGED);
            }
            const quintptr value = reinterpret_cast<quintptr>(hwnd);
            bool tracked = false;
            for (auto& state : monitors) {
                if (state->engine.contains(value)) {
                    tracked = true;
                    break;
                }
            }
            if (enabled && tracked && !floating.contains(value)) {
                scheduleRescan(60);
            } else if (SetWindowPlacement(hwnd, &fullscreen.placement)) {
                SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
                        | SWP_FRAMECHANGED);
            } else if (fullscreen.rect.isValid()) {
                SetWindowPos(hwnd, nullptr, fullscreen.rect.x(), fullscreen.rect.y(),
                    fullscreen.rect.width(), fullscreen.rect.height(),
                    SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
            }
        }
        fullscreen = {};
        fullscreenWindow = nullptr;
    }

    MonitorState* monitorStateFor(HWND hwnd)
    {
        const QString key = monitorFor(hwnd).key;
        const auto it = monitors.find(key);
        return it == monitors.end() ? nullptr : it.value().get();
    }

    void handleAction(WmAction action, int argument)
    {
        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM"))
            qInfo().noquote() << "WM action" << static_cast<int>(action) << argument;

        if (action == WmAction::ToggleTiling) {
            applyEnabled(!enabled);
            return;
        }
        if (action == WmAction::EmergencyDisable) {
            applyEnabled(false);
            return;
        }
        if (action == WmAction::ToggleStrict) {
            strictMode = !strictMode;
            if (strictMode)
                freeMoved.clear();
            if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM"))
                qInfo().noquote() << "WM strict" << (strictMode ? "on" : "off");
            publishState();
            if (enabled)
                scheduleRescan(100);
            return;
        }
        if (action == WmAction::CycleLayout) {
            const TilingEngine::Layout current = TilingEngine::layoutFromName(defaultLayout);
            TilingEngine::Layout next = TilingEngine::Layout::Dwindle;
            switch (current) {
            case TilingEngine::Layout::Dwindle:
                next = TilingEngine::Layout::Master;
                break;
            case TilingEngine::Layout::Master:
                next = TilingEngine::Layout::SplitHorizontal;
                break;
            case TilingEngine::Layout::SplitHorizontal:
                next = TilingEngine::Layout::SplitVertical;
                break;
            case TilingEngine::Layout::SplitVertical:
                next = TilingEngine::Layout::Dwindle;
                break;
            }
            defaultLayout = TilingEngine::layoutName(next);
            freeMoved.clear();
            for (auto& state : monitors)
                state->engine.setLayout(next);
            if (enabled)
                scheduleRescan(80);
            publishState();
            return;
        }
        if (action == WmAction::CycleLayoutBack) {
            const TilingEngine::Layout current = TilingEngine::layoutFromName(defaultLayout);
            TilingEngine::Layout previous = TilingEngine::Layout::SplitVertical;
            switch (current) {
            case TilingEngine::Layout::Dwindle:
                previous = TilingEngine::Layout::SplitVertical;
                break;
            case TilingEngine::Layout::Master:
                previous = TilingEngine::Layout::Dwindle;
                break;
            case TilingEngine::Layout::SplitHorizontal:
                previous = TilingEngine::Layout::Master;
                break;
            case TilingEngine::Layout::SplitVertical:
                previous = TilingEngine::Layout::SplitHorizontal;
                break;
            }
            defaultLayout = TilingEngine::layoutName(previous);
            freeMoved.clear();
            for (auto& state : monitors)
                state->engine.setLayout(previous);
            if (enabled)
                scheduleRescan(80);
            publishState();
            return;
        }
        if (action == WmAction::WorkspaceSwitch) {
            if (argument >= 0 && argument < desktopCount) {
                const int target = argument;
                QMetaObject::invokeMethod(
                    q, [owner = q, target]() { emit owner->switchDesktopRequested(target); },
                    Qt::QueuedConnection);
            }
            return;
        }
        if (action == WmAction::WorkspaceMove) {
            if (activeWindow && argument >= 0 && argument < desktopCount
                && argument != desktopIndex) {
                const QString hwnd = QString::number(
                    reinterpret_cast<quintptr>(activeWindow));
                const int target = argument;
                QMetaObject::invokeMethod(
                    q, [owner = q, hwnd, target]() {
                        emit owner->moveWindowToDesktopRequested(hwnd, target);
                    },
                    Qt::QueuedConnection);
            }
            return;
        }
        if (action == WmAction::WorkspacePrev || action == WmAction::WorkspaceNext) {
            if (desktopCount <= 0)
                return;
            int target = desktopIndex + (action == WmAction::WorkspaceNext ? 1 : -1);
            if (target < 0)
                target = desktopCount - 1;
            if (target >= desktopCount)
                target = 0;
            QMetaObject::invokeMethod(
                q, [owner = q, target]() { emit owner->switchDesktopRequested(target); },
                Qt::QueuedConnection);
            return;
        }
        if (action == WmAction::Launcher) {
            QMetaObject::invokeMethod(
                q, [owner = q]() { emit owner->launcherRequested(); }, Qt::QueuedConnection);
            return;
        }
        if (action == WmAction::Sidebar) {
            QMetaObject::invokeMethod(
                q, [owner = q]() { emit owner->sidebarRequested(); }, Qt::QueuedConnection);
            return;
        }
        if (action == WmAction::Session) {
            QMetaObject::invokeMethod(
                q, [owner = q]() { emit owner->sessionRequested(); }, Qt::QueuedConnection);
            return;
        }
        if (action == WmAction::LockScreen) {
            QMetaObject::invokeMethod(
                q, [owner = q]() { emit owner->lockRequested(); }, Qt::QueuedConnection);
            return;
        }
        if (action == WmAction::Screenshot || action == WmAction::ScreenshotClip) {
            const bool clipboardOnly = action == WmAction::ScreenshotClip;
            QMetaObject::invokeMethod(
                q,
                [owner = q, clipboardOnly]() {
                    emit owner->screenshotRequested(clipboardOnly);
                },
                Qt::QueuedConnection);
            return;
        }
        if (action == WmAction::LaunchTerminal) {
            launchApplication(QStringLiteral("terminal"));
            return;
        }
        if (action == WmAction::LaunchExplorer) {
            launchApplication(QStringLiteral("explorer"));
            return;
        }
        if (action == WmAction::LaunchBrowser) {
            launchApplication(QStringLiteral("browser"));
            return;
        }
        if (action == WmAction::LaunchEditor) {
            launchApplication(QStringLiteral("editor"));
            return;
        }

        updateActiveWindow();
        if (!activeWindow || !IsWindow(activeWindow))
            return;

        if (action == WmAction::ToggleFloating) {
            toggleFloating();
            return;
        }
        if (action == WmAction::ToggleFullscreen) {
            toggleFullscreen();
            return;
        }
        if (action == WmAction::CloseWindow) {
            MonitorState* state = monitorStateFor(activeWindow);
            if (state) {
                const quintptr value = reinterpret_cast<quintptr>(activeWindow);
                if (state->engine.focused() == value) {
                    focusAfterClose = reinterpret_cast<HWND>(
                        state->engine.nextWindow());
                    if (focusAfterClose == activeWindow)
                        focusAfterClose = reinterpret_cast<HWND>(
                            state->engine.previousWindow());
                    closingWindow = activeWindow;
                }
            }
            PostMessageW(activeWindow, WM_CLOSE, 0, 0);
            return;
        }

        MonitorState* state = monitorStateFor(activeWindow);
        if (!state)
            return;
        const QRect content = contentRect(state->work, state->engine.count());
        const quintptr value = reinterpret_cast<quintptr>(activeWindow);

        if (action == WmAction::FocusLeft || action == WmAction::FocusRight
            || action == WmAction::FocusUp || action == WmAction::FocusDown) {
            int dx = 0;
            int dy = 0;
            if (action == WmAction::FocusLeft)
                dx = -1;
            else if (action == WmAction::FocusRight)
                dx = 1;
            else if (action == WmAction::FocusUp)
                dy = -1;
            else
                dy = 1;
            const quintptr target = state->engine.windowInDirection(content, dx, dy);
            if (target)
                focusWindow(reinterpret_cast<HWND>(target));
            return;
        }
        if (action == WmAction::FocusNext || action == WmAction::FocusPrev) {
            const quintptr target = action == WmAction::FocusNext
                ? state->engine.nextWindow()
                : state->engine.previousWindow();
            if (target)
                focusWindow(reinterpret_cast<HWND>(target));
            return;
        }
        if (action == WmAction::MoveLeft || action == WmAction::MoveRight
            || action == WmAction::MoveUp || action == WmAction::MoveDown) {
            int dx = 0;
            int dy = 0;
            if (action == WmAction::MoveLeft)
                dx = -1;
            else if (action == WmAction::MoveRight)
                dx = 1;
            else if (action == WmAction::MoveUp)
                dy = -1;
            else
                dy = 1;
            if (state->engine.swapWithDirection(content, dx, dy)) {
                freeMoved.clear();
                applyMonitor(*state);
                focusWindow(activeWindow);
            }
            return;
        }
        if (action == WmAction::PromoteMaster) {
            if (state->engine.promoteMaster()) {
                freeMoved.clear();
                applyMonitor(*state);
                publishState();
            }
            return;
        }
        if (action == WmAction::ResizeWidthDecrease || action == WmAction::ResizeWidthIncrease
            || action == WmAction::ResizeHeightDecrease
            || action == WmAction::ResizeHeightIncrease) {
            const bool horizontal = action == WmAction::ResizeWidthDecrease
                || action == WmAction::ResizeWidthIncrease;
            const qreal delta = (action == WmAction::ResizeWidthIncrease
                                    || action == WmAction::ResizeHeightIncrease)
                ? 0.05
                : -0.05;
            if (state->engine.resizeFocused(content, horizontal, delta)) {
                freeMoved.clear();
                applyMonitor(*state);
            }
            return;
        }
    }

    void launchApplication(const QString& kind)
    {
        bool launched = false;
        if (kind == QStringLiteral("terminal")) {
            const QString wt = QStandardPaths::findExecutable(QStringLiteral("wt.exe"));
            launched = !wt.isEmpty() && QProcess::startDetached(wt, {});
            if (!launched)
                launched = QProcess::startDetached(QStringLiteral("cmd.exe"), {});
        } else if (kind == QStringLiteral("explorer")) {
            launched = QProcess::startDetached(QStringLiteral("explorer.exe"), {});
        } else if (kind == QStringLiteral("browser")) {
            const HINSTANCE result = ShellExecuteW(nullptr, L"open", L"https://www.google.com",
                nullptr, nullptr, SW_SHOWNORMAL);
            launched = reinterpret_cast<qintptr>(result) > 32;
        } else if (kind == QStringLiteral("editor")) {
            const QString code = QStandardPaths::findExecutable(QStringLiteral("code.exe"));
            const QString codium = QStandardPaths::findExecutable(QStringLiteral("codium.exe"));
            if (!code.isEmpty())
                launched = QProcess::startDetached(code, {});
            else if (!codium.isEmpty())
                launched = QProcess::startDetached(codium, {});
            else
                launched = QProcess::startDetached(QStringLiteral("notepad.exe"), {});
        }
        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_WM")) {
            qInfo().noquote() << "WM launch" << kind << (launched ? "ok" : "failed");
        }
    }

    void publishState()
    {
        QString layout = defaultLayout;
        int count = 0;
        for (auto& state : monitors) {
            count += state->engine.count();
            if (activeWindow
                && state->engine.contains(reinterpret_cast<quintptr>(activeWindow))) {
                layout = TilingEngine::layoutName(state->engine.layout());
            }
        }
        const bool enabledValue = enabled;
        const bool strictValue = strictMode;
        const int countValue = count;
        const QString errorValue = error;
        const QVariantList bindingsValue = publishedBindings;
        QStringList floatingValue;
        floatingValue.reserve(floating.size());
        for (quintptr value : floating)
            floatingValue.append(QString::number(value));
        floatingValue.sort();

        QMetaObject::invokeMethod(
            q,
            [owner = q, enabledValue, strictValue, layout, countValue, errorValue,
                bindingsValue, floatingValue]() {
                bool changed = false;
                if (owner->m_enabled != enabledValue) {
                    owner->m_enabled = enabledValue;
                    changed = true;
                }
                if (owner->m_strict != strictValue) {
                    owner->m_strict = strictValue;
                    changed = true;
                }
                if (owner->m_layout != layout) {
                    owner->m_layout = layout;
                    changed = true;
                }
                if (owner->m_windowCount != countValue) {
                    owner->m_windowCount = countValue;
                    changed = true;
                }
                if (owner->m_error != errorValue) {
                    owner->m_error = errorValue;
                    changed = true;
                }
                if (!bindingsValue.isEmpty() && owner->m_bindings != bindingsValue) {
                    owner->m_bindings = bindingsValue;
                    changed = true;
                }
                if (owner->m_floating != floatingValue) {
                    owner->m_floating = floatingValue;
                    changed = true;
                }
                if (changed)
                    emit owner->changed();
            },
            Qt::QueuedConnection);
    }

    void cleanupResources()
    {
        if (cleaned)
            return;
        cleaned = true;
        enabled = false;
        if (animationTimerRunning) {
            KillTimer(messageWindow, kAnimationTimer);
            animationTimerRunning = false;
        }
        if (messageWindow) {
            KillTimer(messageWindow, kRescanTimer);
            KillTimer(messageWindow, kDebounceTimer);
            KillTimer(messageWindow, kFocusTimer);
        }
        if (fullscreen.valid)
            exitFullscreen();
        restoreAllWindows();
        uninstallMouseHook();
        uninstallKeyboardHook();
        unregisterHotkeys();
        uninstallHooks();
        g_messageWindow = nullptr;
        if (messageWindow) {
            ShowWindow(messageWindow, SW_HIDE);
        }
    }

    // Set by rescan() immediately before EnumWindows so the C callback below
    // can append eligible windows.
    QVector<HWND>* enumTarget = nullptr;
};

WindowManagerService::WindowManagerService(VirtualDesktopService* desktops, QObject* parent)
    : QObject(parent)
    , m_impl(std::make_unique<Impl>(this))
{
    m_enabled = qEnvironmentVariableIsEmpty("CAELESTIA_TILING")
        || qEnvironmentVariable("CAELESTIA_TILING") != QStringLiteral("0");
    m_impl->start();

    if (desktops) {
        const auto sync = [this, desktops]() {
            const QString id = desktops->currentId();
            const int count = desktops->count();
            int index = -1;
            const QVariantList list = desktops->desktops();
            for (int i = 0; i < list.size(); ++i) {
                if (list.at(i).toMap().value(QStringLiteral("active")).toBool()) {
                    index = i;
                    break;
                }
            }
            if (id == m_desktopId && count == m_desktopCount)
                return;
            m_desktopId = id;
            m_desktopIndex = index;
            m_desktopCount = count;
            m_impl->setDesktop(id, index, count);
        };
        connect(desktops, &VirtualDesktopService::changed, this, sync);
        sync();
    }
}

WindowManagerService::~WindowManagerService()
{
    if (m_impl)
        m_impl->stop();
}

void WindowManagerService::setEnabled(bool enabled)
{
    m_impl->setEnabled(enabled);
}

void WindowManagerService::setStrict(bool strict)
{
    m_impl->setStrict(strict);
}

void WindowManagerService::setLayout(const QString& layout)
{
    m_impl->setLayout(layout);
}

void WindowManagerService::cycleLayout()
{
    m_impl->cycleLayout();
}

void WindowManagerService::setFloating(const QString& windowId, bool floating)
{
    if (m_impl)
        m_impl->setFloating(windowId, floating);
}

void WindowManagerService::shutdown()
{
    if (m_impl)
        m_impl->stop();
}

QString WindowManagerService::screenName() const
{
    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    return screen ? screen->name() : QString();
}
