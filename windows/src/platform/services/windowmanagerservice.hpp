#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include <memory>

class VirtualDesktopService;

// Real Win32 tiling window manager. All window tracking, hotkeys and geometry
// work happens on one worker thread with a Win32 message loop; the Qt object is
// a facade for QML and the rest of the shell.
class WindowManagerService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool enabled READ enabled NOTIFY changed)
    Q_PROPERTY(bool strict READ strict NOTIFY changed)
    // An application is presenting itself fullscreen on some monitor. The QML
    // side hides the shell while it is true, like Caelestia's bar.
    Q_PROPERTY(bool appFullscreen READ appFullscreen NOTIFY changed)
    Q_PROPERTY(QString layout READ layout NOTIFY changed)
    Q_PROPERTY(int windowCount READ windowCount NOTIFY changed)
    // HWND ids currently detached from the tiling layouts. The window info
    // panel mirrors this to label its Float/Tile button and its state row.
    Q_PROPERTY(QStringList floating READ floating NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QVariantList bindings READ bindings NOTIFY changed)

public:
    explicit WindowManagerService(VirtualDesktopService* desktops,
        QObject* parent = nullptr);
    ~WindowManagerService() override;

    [[nodiscard]] bool enabled() const { return m_enabled; }
    [[nodiscard]] bool strict() const { return m_strict; }
    [[nodiscard]] bool appFullscreen() const { return m_appFullscreen; }
    [[nodiscard]] QString layout() const { return m_layout; }
    [[nodiscard]] int windowCount() const { return m_windowCount; }
    [[nodiscard]] QStringList floating() const { return m_floating; }
    [[nodiscard]] QString error() const { return m_error; }
    [[nodiscard]] QVariantList bindings() const { return m_bindings; }

    Q_INVOKABLE void setEnabled(bool enabled);
    Q_INVOKABLE void setStrict(bool strict);
    Q_INVOKABLE void setLayout(const QString& layout);
    Q_INVOKABLE void cycleLayout();
    // Detaches one window from the tiling layouts (or hands it back to them).
    // The hotkey path only knows the active window; the window info panel can
    // describe any window, so it addresses them by id.
    Q_INVOKABLE void setFloating(const QString& windowId, bool floating);
    Q_INVOKABLE void shutdown();
    [[nodiscard]] Q_INVOKABLE QString screenName() const;

signals:
    void changed();
    void launcherRequested();
    void sidebarRequested();
    void sessionRequested();
    // The shell lock screen (Win+Alt+L). It is a session lock, not the Windows
    // secure lock; the screen itself offers the handover to LockWorkStation.
    void lockRequested();
    // Emitted by the screenshot hotkeys; the picker decides between saving the
    // region to disk and putting it on the clipboard.
    void screenshotRequested(bool clipboardOnly);
    void switchDesktopRequested(int index);
    void moveWindowToDesktopRequested(const QString& hwnd, int index);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    bool m_enabled = true;
    bool m_strict = true;
    bool m_appFullscreen = false;
    QString m_layout = QStringLiteral("dwindle");
    int m_windowCount = 0;
    QStringList m_floating;
    QString m_error;
    QVariantList m_bindings;

    QString m_desktopId;
    int m_desktopIndex = -1;
    int m_desktopCount = 0;
};
