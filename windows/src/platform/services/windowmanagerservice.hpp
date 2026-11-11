#pragma once

#include <QObject>
#include <QString>
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
    Q_PROPERTY(QString layout READ layout NOTIFY changed)
    Q_PROPERTY(int windowCount READ windowCount NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QVariantList bindings READ bindings NOTIFY changed)

public:
    explicit WindowManagerService(VirtualDesktopService* desktops,
        QObject* parent = nullptr);
    ~WindowManagerService() override;

    [[nodiscard]] bool enabled() const { return m_enabled; }
    [[nodiscard]] bool strict() const { return m_strict; }
    [[nodiscard]] QString layout() const { return m_layout; }
    [[nodiscard]] int windowCount() const { return m_windowCount; }
    [[nodiscard]] QString error() const { return m_error; }
    [[nodiscard]] QVariantList bindings() const { return m_bindings; }

    Q_INVOKABLE void setEnabled(bool enabled);
    Q_INVOKABLE void setStrict(bool strict);
    Q_INVOKABLE void setLayout(const QString& layout);
    Q_INVOKABLE void cycleLayout();
    Q_INVOKABLE void shutdown();
    [[nodiscard]] Q_INVOKABLE QString screenName() const;

signals:
    void changed();
    void launcherRequested();
    void sidebarRequested();
    void sessionRequested();
    void switchDesktopRequested(int index);
    void moveWindowToDesktopRequested(const QString& hwnd, int index);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    bool m_enabled = true;
    bool m_strict = true;
    QString m_layout = QStringLiteral("dwindle");
    int m_windowCount = 0;
    QString m_error;
    QVariantList m_bindings;

    QString m_desktopId;
    int m_desktopIndex = -1;
    int m_desktopCount = 0;
};
