#pragma once

#include <QAbstractNativeEventFilter>
#include <QColor>
#include <QHash>
#include <QMetaObject>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QScreen>
#include <QString>
#include <QTimer>
#include <QVariantList>

class QWindow;

class Platform final : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(int windowsBuild READ windowsBuild CONSTANT)
    Q_PROPERTY(bool darkMode READ darkMode NOTIFY darkModeChanged)
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY accentColorChanged)
    Q_PROPERTY(QString wallpaperPath READ wallpaperPath NOTIFY wallpaperPathChanged)
    Q_PROPERTY(bool wallpaperAvailable READ wallpaperAvailable NOTIFY wallpaperAvailableChanged)
    Q_PROPERTY(bool animationsEnabled READ animationsEnabled NOTIFY animationsEnabledChanged)
    Q_PROPERTY(qreal systemScale READ systemScale NOTIFY systemScaleChanged)
    Q_PROPERTY(QString rhiBackend READ rhiBackend NOTIFY rhiBackendChanged)
    Q_PROPERTY(bool appBarRegistered READ appBarRegistered NOTIFY appBarRegisteredChanged)
    Q_PROPERTY(int appBarWidth READ appBarWidth NOTIFY appBarWidthChanged)
    Q_PROPERTY(int appBarHeight READ appBarHeight NOTIFY appBarHeightChanged)
    Q_PROPERTY(QString activeWindowTitle READ activeWindowTitle NOTIFY activeWindowChanged)
    Q_PROPERTY(QString activeWindowIcon READ activeWindowIcon NOTIFY activeWindowChanged)
    // HWND of the last external foreground window. Opening a panel moves the
    // focus to the shell, so the window info panel needs this to know which
    // window it should describe.
    Q_PROPERTY(QString activeWindowId READ activeWindowId NOTIFY activeWindowChanged)
    Q_PROPERTY(QVariantList screens READ screens NOTIFY screensChanged)
    Q_PROPERTY(QString primaryScreenName READ primaryScreenName NOTIFY primaryScreenNameChanged)

public:
    explicit Platform(QObject* parent = nullptr);
    ~Platform() override;

    [[nodiscard]] QString name() const;
    [[nodiscard]] QString version() const;
    [[nodiscard]] int windowsBuild() const;
    [[nodiscard]] bool darkMode() const;
    [[nodiscard]] QColor accentColor() const;
    [[nodiscard]] QString wallpaperPath() const;
    [[nodiscard]] bool wallpaperAvailable() const;
    [[nodiscard]] bool animationsEnabled() const;
    [[nodiscard]] qreal systemScale() const;
    [[nodiscard]] QString rhiBackend() const;
    [[nodiscard]] bool appBarRegistered() const;
    [[nodiscard]] int appBarWidth() const;
    [[nodiscard]] int appBarHeight() const;
    [[nodiscard]] QString activeWindowTitle() const;
    [[nodiscard]] QString activeWindowIcon() const;
    [[nodiscard]] QString activeWindowId() const;
    [[nodiscard]] QVariantList screens() const;
    [[nodiscard]] QString primaryScreenName() const;

    // Icon of the window as a data: URL, cached per window handle. The dock
    // asks once per entry; extraction costs a WM_GETICON round trip plus an
    // encode, which is too much to repeat on every refresh.
    Q_INVOKABLE QString windowIcon(const QString& windowId);

    // Pointer position in global logical coordinates. Polled by the dock's
    // edge test: WM_INPUT only reports real device movement, so a synthetic or
    // scripted pointer move never reached it.
    Q_INVOKABLE QPoint cursorPos() const;

    Q_INVOKABLE bool registerLeftAppBar(QWindow* window, int logicalWidth = 60);
    // Reserves the bottom edge for the application dock, the same way the left
    // bar reserves its column: maximized windows stop above the dock.
    Q_INVOKABLE bool registerBottomAppBar(QWindow* window, int logicalHeight = 76);
    Q_INVOKABLE void unregisterAppBar();
    Q_INVOKABLE void unregisterAppBar(QWindow* window);
    Q_INVOKABLE void refreshAppBar();

    // Returns focus to the last non-shell foreground window. Used after a
    // panel closes so opening a popout does not steal the user's app.
    Q_INVOKABLE void restoreLastExternalWindow();

    // Keeps the AppBar windows above the full-screen visual overlays while
    // leaving normal windows below them.
    Q_INVOKABLE void raiseAppBars();

    // WS_EX_TRANSPARENT is the only reliable cross-process click-through for
    // a layered top-level window. WS_EX_NOACTIVATE keeps the idle overlay from
    // stealing keyboard focus; both are toggled together when a panel opens.
    Q_INVOKABLE void setWindowInputState(QWindow* window, bool clickThrough, bool focusable);

    // Registers Escape while a panel is open so a hover-opened panel can be
    // dismissed without stealing keyboard focus from the user's app.
    Q_INVOKABLE void setEscapeHotkey(QWindow* window, bool enabled);

    // Removes WS_SYSMENU so Alt+F4 has nothing to post: DefWindowProc drops
    // SC_CLOSE when the window has no system menu. The lock surface cannot
    // reject close events instead, because a QQuickWindow that refuses to close
    // also blocks QCoreApplication::quit() and Windows session end.
    Q_INVOKABLE void disableWindowClose(QWindow* window);

    // Sizes a window to the monitor's full bounds and makes Qt adopt that
    // monitor's DPI. Tool windows created by the QML engine land on the
    // primary screen first, so Qt lays them out at the primary's scale unless
    // the screen is assigned before the geometry.
    Q_INVOKABLE void fitWindowToScreen(QWindow* window, QScreen* screen);

    // ShellExecuteW resolves a bare executable through PATH and understands
    // registered URI schemes (ms-settings:, https:, ...), which QProcess
    // cannot do. Returns false when Windows refuses to start the target.
    Q_INVOKABLE bool openExternal(const QString& target);

    Q_INVOKABLE void copyText(const QString& text);

    // Small persistent store for shell preferences owned by QML (the colour
    // scheme today). Keys live under HKCU\Software\<app>\shell and survive
    // restarts; QSettings handles the encoding of colours and lists.
    [[nodiscard]] Q_INVOKABLE QVariant shellSetting(const QString& key,
        const QVariant& fallback = QVariant()) const;
    // Emits shellSettingChanged so QML can rebind after the write. The settings
    // are read through an invokable, so a plain binding on shellSetting() has
    // no dependency to invalidate: without this signal a scheme picked in Nexus
    // only reached the palette if the panel also assigned theme.scheme itself,
    // which broke the binding and lost the value on the next read.
    Q_INVOKABLE void setShellSetting(const QString& key, const QVariant& value);

    void setRhiBackend(const QString& backend);
    void handleWinEvent(quint32 event, quintptr hwnd, qint32 idObject, qint32 idChild);

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void darkModeChanged();
    void accentColorChanged();
    void shellSettingChanged(const QString& key);
    void wallpaperPathChanged();
    void wallpaperAvailableChanged();
    void animationsEnabledChanged();
    void systemScaleChanged();
    void rhiBackendChanged();
    void appBarRegisteredChanged();
    void appBarWidthChanged();
    void appBarHeightChanged();
    void activeWindowChanged();
    void screensChanged();
    void primaryScreenNameChanged();
    void escapePressed();
    void globalMouseMoved(const QPoint& position);

private:
    void refresh();

    enum class BarEdge { Left, Bottom };

    struct AppBarEntry {
        QWindow* window = nullptr;
        quintptr hwnd = 0;
        QPointer<QScreen> screen;
        int logicalSize = 60;
        BarEdge edge = BarEdge::Left;
        QMetaObject::Connection windowScreenConnection;
        QMetaObject::Connection geometryConnection;
        QMetaObject::Connection dpiConnection;
        QRect lastRect;
    };

    bool registerAppBar(QWindow* window, int logicalSize, BarEdge edge);
    void refreshAppBarEntry(AppBarEntry& entry);
    void connectEntryScreen(AppBarEntry& entry, QScreen* screen);
    void disconnectEntryScreen(AppBarEntry& entry);
    void handleWindowScreenChanged(QWindow* window, QScreen* screen);
    void handleScreensChanged();
    void handleScreenRemoved(QScreen* screen);

    // Windows taskbar auto-hide while the shell owns the desktop. The
    // previous AppBar state is restored on exit so the user's setting survives.
    void hideTaskbar();
    void restoreTaskbar();
    void enforceTaskbarHidden();

    // Desktop icon list view, hidden while the shell runs for the same reason
    // as the taskbar. Explorer recreates it on refresh, so visibility is
    // re-applied by the same timer.
    void hideDesktopIcons();
    void restoreDesktopIcons();
    void enforceDesktopIconsHidden();

    void startActiveWindowTracking();
    void stopActiveWindowTracking();
    void updateActiveWindow(quintptr hwnd, bool refreshIcon);
    void clearActiveWindow();
    // Returns true when the change was signalled, so the caller can emit for
    // an id change that leaves title and icon identical.
    bool setActiveWindowInfo(const QString& title, const QString& icon);
    [[nodiscard]] QString readWindowIcon(quintptr hwnd) const;
    [[nodiscard]] static QString windowClassName(quintptr hwnd);

    QTimer m_refreshTimer;
    bool m_darkMode = true;
    QColor m_accentColor = QColor(QStringLiteral("#ffb0ca"));
    QString m_wallpaperPath;
    bool m_wallpaperAvailable = false;
    bool m_animationsEnabled = true;
    qreal m_systemScale = 1.0;
    int m_windowsBuild = 0;
    QString m_rhiBackend = QStringLiteral("pending");

    QHash<QWindow*, AppBarEntry> m_appBars;
    quint32 m_appBarMessage = 0;
    bool m_appBarUpdating = false;
    int m_appBarWidth = 60;
    int m_appBarHeight = 0;
    // WinEvent hook that hides the taskbar again the moment Explorer reveals
    // it on the bottom edge. Stored as void* so the header stays free of
    // windows.h.
    void* m_taskbarHook = nullptr;
    QHash<quintptr, QString> m_windowIconCache;

    quintptr m_rawInputHwnd = 0;
    quintptr m_escapeHotkeyHwnd = 0;

    QTimer m_taskbarTimer;
    bool m_taskbarHidden = false;
    bool m_taskbarWasVisible = false;
    quint32 m_taskbarState = 0;

    bool m_desktopIconsHidden = false;
    bool m_desktopIconsWereVisible = false;

    bool m_activeWindowTracking = false;
    quintptr m_activeWindowHwnd = 0;
    quintptr m_foregroundHook = 0;
    quintptr m_nameChangeHook = 0;
    quintptr m_minimizeHook = 0;
    quintptr m_destroyHook = 0;
    QString m_activeWindowTitle;
    QString m_activeWindowIcon;
};
