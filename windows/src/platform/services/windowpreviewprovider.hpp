#pragma once

#include <QQuickImageProvider>

class WindowService;

// Serves the PrintWindow capture of the tracked window to QML under
// "image://windowpreview/<revision>". The id is only a cache-buster: every
// capture bumps the revision so the same provider returns the fresh frame.
class WindowPreviewProvider final : public QQuickImageProvider {
public:
    explicit WindowPreviewProvider(WindowService* service);

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    WindowService* m_service;
};
