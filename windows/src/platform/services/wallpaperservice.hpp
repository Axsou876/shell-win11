#pragma once

#include <QObject>
#include <QVariantList>

// Wallpaper discovery and real Windows desktop-wallpaper switching. Used by the
// launcher's ">wal" mode.
class WallpaperService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList wallpapers READ wallpapers NOTIFY changed)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY changed)

public:
    explicit WallpaperService(QObject* parent = nullptr);

    [[nodiscard]] QVariantList wallpapers() const { return m_wallpapers; }
    [[nodiscard]] QString currentPath() const { return m_currentPath; }

    Q_INVOKABLE bool apply(const QString& path);

signals:
    void changed();

private:
    void scan();

    QVariantList m_wallpapers;
    QString m_currentPath;
};
