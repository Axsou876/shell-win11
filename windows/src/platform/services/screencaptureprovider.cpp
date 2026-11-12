#include "screencaptureprovider.hpp"

#include "screencaptureservice.hpp"

ScreenCaptureProvider::ScreenCaptureProvider(ScreenCaptureService* service)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_service(service)
{
}

QImage ScreenCaptureProvider::requestImage(const QString& id, QSize* size,
    const QSize& requestedSize)
{
    // The id is the capture token; anything after '-' is only a cache-buster
    // used when the same token is refreshed after a screen change.
    const QString token = id.section(QLatin1Char('-'), 0, 0);
    QImage image = m_service ? m_service->imageForToken(token.toInt()) : QImage();
    if (image.isNull())
        return image;

    if (size)
        *size = image.size();
    if (requestedSize.isValid() && requestedSize != image.size())
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return image;
}
