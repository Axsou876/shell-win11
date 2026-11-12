#pragma once

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include <functional>

class QTimer;

// Top-level application windows on the current Windows virtual desktop. Used by
// the Workspaces tab and by the window info panel; clicking a card activates
// the real window.
class WindowService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList windows READ windows NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    // Details of the last external foreground window. The map survives the
    // shell's own overlays taking the focus, which is exactly what happens
    // while the window info panel is open.
    Q_PROPERTY(QVariantMap focused READ focused NOTIFY focusedChanged)
    // Bumped after every captured frame. QML keeps one provider id and changes
    // the source string so the Image reloads without a growing URL cache.
    Q_PROPERTY(int previewRevision READ previewRevision NOTIFY previewChanged)
    Q_PROPERTY(QString actionError READ actionError NOTIFY actionErrorChanged)

public:
    explicit WindowService(QObject* parent = nullptr);
    ~WindowService() override;

    [[nodiscard]] QVariantList windows() const { return m_windows; }
    [[nodiscard]] int count() const { return m_windows.size(); }
    [[nodiscard]] QVariantMap focused() const { return m_focused; }
    [[nodiscard]] int previewRevision() const { return m_previewRevision; }
    [[nodiscard]] QString actionError() const { return m_actionError; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void focusWindow(const QString& id);

    // The window info panel lives only while it is open: the fast poll and the
    // PrintWindow capture are both too expensive to run for the whole session.
    Q_INVOKABLE void trackFocused(bool enabled, const QString& windowId = QString());
    Q_INVOKABLE void minimizeFocused();
    Q_INVOKABLE void closeFocused();

    // Called by WindowPreviewProvider, possibly from the image loading thread.
    [[nodiscard]] QImage previewImage() const;

signals:
    void changed();
    void focusedChanged();
    void previewChanged();
    void actionErrorChanged();

private:
    void refreshFocused();
    void capturePreview();
    void runAction(const char* name, const std::function<bool(quintptr)>& action);
    [[nodiscard]] quintptr focusedHwnd() const;
    void setActionError(const QString& message);

    QVariantList m_windows;
    QVariantMap m_focused;
    QTimer* m_timer = nullptr;
    QTimer* m_focusedTimer = nullptr;
    quintptr m_focusedHwnd = 0;
    // Set right after an action that makes Windows activate another window,
    // so the panel keeps describing the window it just acted on.
    qint64 m_pinnedUntil = 0;
    int m_previewRevision = 0;
    int m_capturePending = 0;
    int m_errorToken = 0;
    QString m_actionError;
    QImage m_preview;
    mutable QMutex m_previewMutex;
    struct Impl;
    Impl* m_impl = nullptr;
};
