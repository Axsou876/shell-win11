#pragma once

#include <QObject>
#include <QVariantList>

class QTimer;

// Top-level application windows on the current Windows virtual desktop. Used by
// the Workspaces tab; clicking a card activates the real window.
class WindowService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList windows READ windows NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)

public:
    explicit WindowService(QObject* parent = nullptr);
    ~WindowService() override;

    [[nodiscard]] QVariantList windows() const { return m_windows; }
    [[nodiscard]] int count() const { return m_windows.size(); }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void focusWindow(const QString& id);

signals:
    void changed();

private:
    QVariantList m_windows;
    QTimer* m_timer = nullptr;
    struct Impl;
    Impl* m_impl = nullptr;
};
