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
    Q_PROPERTY(QVariantList screens READ screens NOTIFY screensChanged)

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
    [[nodiscard]] QVariantList screens() const;

    Q_INVOKABLE bool registerLeftAppBar(QWindow* window, int logicalWidth = 60);
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

    void setRhiBackend(const QString& backend);
    void handleWinEvent(quint32 event, quintptr hwnd, qint32 idObject, qint32 idChild);

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void darkModeChanged();
    void accentColorChanged();
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
    void escapePressed();
    void globalMouseMoved(const QPoint& position);

private:
    void refresh();

    struct AppBarEntry {
        QWindow* window = nullptr;
        quintptr hwnd = 0;
        QPointer<QScreen> screen;
        int logicalWidth = 60;
        QMetaObject::Connection windowScreenConnection;
        QMetaObject::Connection geometryConnection;
        QMetaObject::Connection dpiConnection;
        QRect lastRect;
    };

    void refreshAppBarEntry(AppBarEntry& entry);
    void connectEntryScreen(AppBarEntry& entry, QScreen* screen);
    void disconnectEntryScreen(AppBarEntry& entry);
    void handleWindowScreenChanged(QWindow* window, QScreen* screen);
    void handleScreensChanged();
    void handleScreenRemoved(QScreen* screen);

    void startActiveWindowTracking();
    void stopActiveWindowTracking();
    void updateActiveWindow(quintptr hwnd, bool refreshIcon);
    void clearActiveWindow();
    void setActiveWindowInfo(const QString& title, const QString& icon);
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

    quintptr m_rawInputHwnd = 0;
    quintptr m_escapeHotkeyHwnd = 0;

    bool m_activeWindowTracking = false;
    quintptr m_activeWindowHwnd = 0;
    quintptr m_foregroundHook = 0;
    quintptr m_nameChangeHook = 0;
    quintptr m_minimizeHook = 0;
    quintptr m_destroyHook = 0;
    QString m_activeWindowTitle;
    QString m_activeWindowIcon;
};
