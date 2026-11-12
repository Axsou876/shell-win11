#pragma once

#include <QQuickImageProvider>

class ScreenCaptureService;

// Serves one frozen monitor capture to the area picker under
// "image://screencapture/<token>". The token is assigned per capture, so a
// repeat screenshot always asks for a different URL.
class ScreenCaptureProvider final : public QQuickImageProvider {
public:
    explicit ScreenCaptureProvider(ScreenCaptureService* service);

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    ScreenCaptureService* m_service;
};
