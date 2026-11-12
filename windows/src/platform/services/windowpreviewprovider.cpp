#include "windowpreviewprovider.hpp"

#include "windowsservice.hpp"

WindowPreviewProvider::WindowPreviewProvider(WindowService* service)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_service(service)
{
}

// Qt may call this from the image loading thread, never from the GUI thread,
// so previewImage() hands out a copy under the service's mutex.
QImage WindowPreviewProvider::requestImage(
    const QString&, QSize* size, const QSize& requestedSize)
{
    const QImage image = m_service ? m_service->previewImage() : QImage();
    if (image.isNull()) {
        if (size)
            *size = QSize();
        return QImage();
    }

    if (size)
        *size = image.size();
    if (requestedSize.isValid() && requestedSize != image.size())
        return image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return image;
}
