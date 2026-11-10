#include "wallpaperservice.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSettings>
#include <QSet>

#include <algorithm>
#include <QStandardPaths>

#include <windows.h>

namespace {
QStringList wallpaperRoots()
{
    QStringList roots;
    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (!pictures.isEmpty())
        roots.append(pictures);

    const QString windows = qEnvironmentVariable("WINDIR");
    if (!windows.isEmpty())
        roots.append(windows + QStringLiteral("/Web/Wallpaper"));

    const QString themes = qEnvironmentVariable("APPDATA")
        + QStringLiteral("/Microsoft/Windows/Themes");
    if (!themes.isEmpty())
        roots.append(themes);

    return roots;
}
} // namespace

WallpaperService::WallpaperService(QObject* parent)
    : QObject(parent)
{
    QSettings settings;
    m_currentPath = settings.value(QStringLiteral("wallpaper/path")).toString();
    scan();
}

void WallpaperService::scan()
{
    m_wallpapers.clear();
    QSet<QString> seen;

    for (const QString& root : wallpaperRoots()) {
        if (!QFileInfo::exists(root))
            continue;
        QDirIterator it(root,
            { QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.png"),
                QStringLiteral("*.bmp"), QStringLiteral("*.webp") },
            QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = QDir::cleanPath(it.next());
            if (seen.contains(path))
                continue;
            const QFileInfo info(path);
            if (info.size() < 16 * 1024)
                continue;
            seen.insert(path);

            QVariantMap item;
            item.insert(QStringLiteral("path"), path);
            item.insert(QStringLiteral("name"), info.completeBaseName());
            item.insert(QStringLiteral("folder"), info.dir().dirName());
            m_wallpapers.append(item);
            if (m_wallpapers.size() >= 160)
                break;
        }
        if (m_wallpapers.size() >= 160)
            break;
    }

    std::sort(m_wallpapers.begin(), m_wallpapers.end(),
        [](const QVariant& a, const QVariant& b) {
            return QString::localeAwareCompare(a.toMap().value(QStringLiteral("name")).toString(),
                       b.toMap().value(QStringLiteral("name")).toString())
                < 0;
        });
    emit changed();
}

bool WallpaperService::apply(const QString& path)
{
    const QString native = QDir::toNativeSeparators(path);
    if (!QFileInfo::exists(native))
        return false;

    const bool ok = SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0,
        reinterpret_cast<void*>(const_cast<wchar_t*>(
            reinterpret_cast<const wchar_t*>(native.utf16()))),
        SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    if (!ok)
        return false;

    m_currentPath = native;
    QSettings settings;
    settings.setValue(QStringLiteral("wallpaper/path"), native);
    settings.sync();
    emit changed();
    return true;
}

