#include "applauncherservice.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImage>
#include <QMetaObject>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QtConcurrent>

#include <algorithm>

#include <windows.h>
#include <objbase.h>
#include <ole2.h>
#include <shellapi.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <propkey.h>
#include <wrl/client.h>

namespace {
QString knownFolder(REFKNOWNFOLDERID id)
{
    PWSTR path = nullptr;
    QString result;
    if (SUCCEEDED(SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &path))) {
        result = QString::fromWCharArray(path);
        CoTaskMemFree(path);
    }
    return result;
}

QString winError(DWORD code)
{
    wchar_t* buffer = nullptr;
    const DWORD length = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    QString message;
    if (length && buffer)
        message = QString::fromWCharArray(buffer, static_cast<int>(length)).trimmed();
    if (buffer)
        LocalFree(buffer);
    return message.isEmpty() ? QStringLiteral("Windows error %1").arg(code) : message;
}

QImage iconImage(const QString& path)
{
    if (path.isEmpty())
        return {};

    SHFILEINFOW info = {};
    if (!SHGetFileInfoW(reinterpret_cast<LPCWSTR>(path.utf16()), 0, &info, sizeof(info),
            SHGFI_ICON | SHGFI_LARGEICON)
        || !info.hIcon) {
        return {};
    }

    ICONINFO iconInfo = {};
    QImage image;
    if (GetIconInfo(info.hIcon, &iconInfo)) {
        BITMAP bitmap = {};
        if (GetObjectW(iconInfo.hbmColor, sizeof(bitmap), &bitmap) == sizeof(bitmap)
            && bitmap.bmWidth > 0 && bitmap.bmHeight > 0) {
            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = bitmap.bmWidth;
            bmi.bmiHeader.biHeight = -bitmap.bmHeight;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            image = QImage(bitmap.bmWidth, bitmap.bmHeight, QImage::Format_ARGB32);
            HDC dc = GetDC(nullptr);
            const int lines = GetDIBits(dc, iconInfo.hbmColor, 0,
                static_cast<UINT>(bitmap.bmHeight), image.bits(), &bmi, DIB_RGB_COLORS);
            ReleaseDC(nullptr, dc);
            if (lines != bitmap.bmHeight)
                image = {};

            bool hasAlpha = false;
            if (!image.isNull()) {
                for (int y = 0; y < image.height() && !hasAlpha; ++y) {
                    const auto* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
                    for (int x = 0; x < image.width(); ++x) {
                        if (qAlpha(line[x]) != 0) {
                            hasAlpha = true;
                            break;
                        }
                    }
                }
            }

            // Legacy 24-bit icons carry transparency in the AND mask.
            if (!image.isNull() && !hasAlpha && iconInfo.hbmMask) {
                BITMAP maskBitmap = {};
                if (GetObjectW(iconInfo.hbmMask, sizeof(maskBitmap), &maskBitmap) == sizeof(maskBitmap)
                    && maskBitmap.bmWidth == bitmap.bmWidth
                    && maskBitmap.bmHeight >= bitmap.bmHeight) {
                    QImage mask(bitmap.bmWidth, bitmap.bmHeight, QImage::Format_Mono);
                    HDC maskDc = GetDC(nullptr);
                    BITMAPINFO maskInfo = {};
                    maskInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    maskInfo.bmiHeader.biWidth = bitmap.bmWidth;
                    maskInfo.bmiHeader.biHeight = -bitmap.bmHeight;
                    maskInfo.bmiHeader.biPlanes = 1;
                    maskInfo.bmiHeader.biBitCount = 1;
                    const int maskLines = GetDIBits(maskDc, iconInfo.hbmMask, 0,
                        static_cast<UINT>(bitmap.bmHeight), mask.bits(), &maskInfo, DIB_RGB_COLORS);
                    ReleaseDC(nullptr, maskDc);
                    if (maskLines == bitmap.bmHeight) {
                        for (int y = 0; y < image.height(); ++y) {
                            auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
                            for (int x = 0; x < image.width(); ++x) {
                                if (mask.constScanLine(y)[x / 8] & (0x80 >> (x % 8)))
                                    line[x] = qRgba(0, 0, 0, 0);
                            }
                        }
                    }
                }
            }
        }

        if (iconInfo.hbmColor)
            DeleteObject(iconInfo.hbmColor);
        if (iconInfo.hbmMask)
            DeleteObject(iconInfo.hbmMask);
    }

    DestroyIcon(info.hIcon);
    if (image.isNull())
        return {};
    return image.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QImage shellItemImage(const QString& parsingName)
{
    if (parsingName.isEmpty())
        return {};

    Microsoft::WRL::ComPtr<IShellItem> item;
    if (FAILED(SHCreateItemFromParsingName(reinterpret_cast<LPCWSTR>(parsingName.utf16()),
            nullptr, IID_PPV_ARGS(&item)))) {
        return {};
    }

    Microsoft::WRL::ComPtr<IShellItemImageFactory> factory;
    if (FAILED(item.As(&factory)))
        return {};

    HBITMAP bitmap = nullptr;
    const SIZE size { 64, 64 };
    if (FAILED(factory->GetImage(size, SIIGBF_ICONONLY | SIIGBF_BIGGERSIZEOK, &bitmap))
        || !bitmap) {
        return {};
    }

    QImage image = QImage::fromHBITMAP(bitmap);
    DeleteObject(bitmap);
    if (image.isNull())
        return {};
    return image.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

AppEntry resolveShortcut(const QString& path)
{
    AppEntry entry;
    entry.launchPath = QDir::toNativeSeparators(path);
    entry.id = QString::fromLatin1(
        QCryptographicHash::hash(entry.launchPath.toUtf8(), QCryptographicHash::Sha1).toHex());
    entry.name = QFileInfo(path).completeBaseName();

    Microsoft::WRL::ComPtr<IShellLinkW> link;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
            __uuidof(IShellLinkW), reinterpret_cast<void**>(link.GetAddressOf())))) {
        return {};
    }

    Microsoft::WRL::ComPtr<IPersistFile> persist;
    if (FAILED(link.As(&persist))
        || FAILED(persist->Load(reinterpret_cast<LPCWSTR>(entry.launchPath.utf16()), STGM_READ))) {
        return {};
    }

    wchar_t buffer[32768] = {};
    if (SUCCEEDED(link->GetPath(buffer, 32768, nullptr, SLGP_RAWPATH)))
        entry.target = QString::fromWCharArray(buffer);
    if (SUCCEEDED(link->GetArguments(buffer, 32768)))
        entry.arguments = QString::fromWCharArray(buffer);
    if (SUCCEEDED(link->GetWorkingDirectory(buffer, 32768)))
        entry.workingDirectory = QString::fromWCharArray(buffer);
    if (SUCCEEDED(link->GetDescription(buffer, 32768)))
        entry.description = QString::fromWCharArray(buffer).trimmed();

    int iconIndex = 0;
    if (SUCCEEDED(link->GetIconLocation(buffer, 32768, &iconIndex)) && buffer[0])
        entry.iconSource = QString::fromWCharArray(buffer);
    if (entry.iconSource.isEmpty())
        entry.iconSource = entry.target;
    if (entry.iconSource.isEmpty())
        entry.iconSource = entry.launchPath;

    if (entry.description.isEmpty())
        entry.description = entry.target.isEmpty() ? QStringLiteral("Application") : entry.target;

    return entry;
}

void scanShortcutDir(const QString& root, QHash<QString, AppEntry>* byName)
{
    if (root.isEmpty() || !QFileInfo::exists(root))
        return;

    QDirIterator it(root, QStringList { QStringLiteral("*.lnk") }, QDir::Files,
        QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const QString base = QFileInfo(path).completeBaseName();
        const QString lower = base.toLower();
        if (lower.startsWith(QStringLiteral("uninstall"))
            || lower.startsWith(QStringLiteral("dÃ©sinstaller"))
            || lower.startsWith(QStringLiteral("desinstaller"))
            || lower.startsWith(QStringLiteral("remove "))) {
            continue;
        }

        AppEntry entry = resolveShortcut(path);
        if (entry.name.isEmpty() || entry.launchPath.isEmpty())
            continue;

        // The user Start Menu is scanned last and replaces a common entry
        // with the same visible name.
        (*byName)[entry.name.toLower()] = entry;
    }
}

QList<AppEntry> scanStartMenu()
{
    QHash<QString, AppEntry> byName;
    scanShortcutDir(knownFolder(FOLDERID_CommonPrograms), &byName);
    scanShortcutDir(knownFolder(FOLDERID_Programs), &byName);

    // Registered Windows/UWP applications. They are launched through
    // shell:AppsFolder and are only added when no Start Menu shortcut of the
    // same visible name exists.
    Microsoft::WRL::ComPtr<IShellItem> folder;
    if (SUCCEEDED(SHGetKnownFolderItem(FOLDERID_AppsFolder, KF_FLAG_DEFAULT, nullptr,
            IID_PPV_ARGS(&folder)))) {
        Microsoft::WRL::ComPtr<IEnumShellItems> items;
        if (SUCCEEDED(folder->BindToHandler(nullptr, BHID_EnumItems, IID_PPV_ARGS(&items)))) {
            Microsoft::WRL::ComPtr<IShellItem> child;
            while (items->Next(1, &child, nullptr) == S_OK) {
                Microsoft::WRL::ComPtr<IShellItem2> item2;
                PWSTR displayName = nullptr;
                PWSTR aumid = nullptr;
                PWSTR parsingName = nullptr;
                if (SUCCEEDED(child.As(&item2))
                    && SUCCEEDED(item2->GetDisplayName(SIGDN_NORMALDISPLAY, &displayName))
                    && SUCCEEDED(item2->GetString(PKEY_AppUserModel_ID, &aumid))) {
                    const QString name = QString::fromWCharArray(displayName);
                    if (!name.isEmpty() && !byName.contains(name.toLower())) {
                        item2->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &parsingName);
                        AppEntry entry;
                        entry.name = name;
                        entry.description = QStringLiteral("Application Windows");
                        entry.launchPath = QStringLiteral("shell:AppsFolder\\")
                            + QString::fromWCharArray(aumid);
                        entry.target = parsingName ? QString::fromWCharArray(parsingName) : entry.launchPath;
                        entry.iconSource = entry.launchPath;
                        entry.id = QString::fromLatin1(
                            QCryptographicHash::hash(entry.launchPath.toUtf8(),
                                QCryptographicHash::Sha1)
                                .toHex());
                        byName.insert(entry.name.toLower(), entry);
                    }
                }
                if (displayName)
                    CoTaskMemFree(displayName);
                if (aumid)
                    CoTaskMemFree(aumid);
                if (parsingName)
                    CoTaskMemFree(parsingName);
                child.Reset();
            }
        }
    }

    QList<AppEntry> entries;
    entries.reserve(byName.size());
    for (const auto& entry : std::as_const(byName))
        entries.append(entry);
    std::sort(entries.begin(), entries.end(), [](const AppEntry& a, const AppEntry& b) {
        return QString::localeAwareCompare(a.name, b.name) < 0;
    });
    return entries;
}

int fuzzyScore(const QString& query, const AppEntry& entry)
{
    if (query.isEmpty())
        return 1;

    const QString name = entry.name.toLower();
    if (name == query)
        return 2000;
    if (name.startsWith(query))
        return 1600 - name.size();
    if (const int position = name.indexOf(query); position >= 0)
        return 1300 - position * 4 - name.size();

    // Subsequence match, with a penalty for gaps. This catches "vsc" for
    // "Visual Studio Code" without a heavyweight fuzzy library.
    int index = 0;
    int gaps = 0;
    int last = -1;
    for (const QChar c : query) {
        const int found = name.indexOf(c, index);
        if (found < 0)
            return 0;
        if (last >= 0)
            gaps += found - last - 1;
        last = found;
        index = found + 1;
    }
    return 900 - gaps * 8 - name.size();

}

QString iconCachePath(const QString& id)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + QStringLiteral("/app-icons");
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + id + QStringLiteral(".png");
}
} // namespace

AppListModel::AppListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int AppListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant AppListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const AppEntry& entry = m_entries.at(index.row());
    switch (role) {
    case IdRole:
        return entry.id;
    case NameRole:
        return entry.name;
    case DescriptionRole:
        return entry.description;
    case IconRole:
        return entry.iconPath;
    case FavoriteRole:
        return entry.favorite;
    case RecentRole:
        return entry.lastUsed;
    default:
        return {};
    }
}

QHash<int, QByteArray> AppListModel::roleNames() const
{
    return {
        { IdRole, "appId" },
        { NameRole, "name" },
        { DescriptionRole, "description" },
        { IconRole, "icon" },
        { FavoriteRole, "favorite" },
        { RecentRole, "lastUsed" },
    };
}

int AppListModel::indexOf(const QString& id) const
{
    return m_index.value(id, -1);
}

void AppListModel::setEntries(QList<AppEntry> entries)
{
    beginResetModel();
    m_entries = std::move(entries);
    m_index.clear();
    for (int i = 0; i < m_entries.size(); ++i)
        m_index.insert(m_entries.at(i).id, i);
    endResetModel();
}

void AppListModel::setIconPath(const QString& id, const QString& path)
{
    const int row = indexOf(id);
    if (row < 0 || m_entries.at(row).iconPath == path)
        return;
    m_entries[row].iconPath = path;
    const QModelIndex changed = index(row, 0);
    emit dataChanged(changed, changed, { IconRole });
}

void AppListModel::setFavorite(const QString& id, bool favorite)
{
    const int row = indexOf(id);
    if (row < 0 || m_entries.at(row).favorite == favorite)
        return;
    m_entries[row].favorite = favorite;
    const QModelIndex changed = index(row, 0);
    emit dataChanged(changed, changed, { FavoriteRole });
}

void AppListModel::touch(const QString& id)
{
    const int row = indexOf(id);
    if (row < 0)
        return;
    m_entries[row].lastUsed = QDateTime::currentSecsSinceEpoch();
    ++m_entries[row].frequency;
    const QModelIndex changed = index(row, 0);
    emit dataChanged(changed, changed, { RecentRole });
}

AppSearchModel::AppSearchModel(AppListModel* source, QObject* parent)
    : QAbstractListModel(parent)
    , m_source(source)
{
    connect(source, &QAbstractItemModel::modelReset, this, &AppSearchModel::rebuild);
    connect(source, &QAbstractItemModel::dataChanged, this,
        [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles) {
            Q_UNUSED(topLeft)
            Q_UNUSED(bottomRight)
            Q_UNUSED(roles)
            rebuild();
        });
}

int AppSearchModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant AppSearchModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    return m_source->data(m_source->index(m_rows.at(index.row()), 0), role);
}

QHash<int, QByteArray> AppSearchModel::roleNames() const
{
    return m_source->roleNames();
}

void AppSearchModel::setQuery(const QString& query)
{
    const QString normalized = query.trimmed().toLower();
    if (m_query == normalized)
        return;
    m_query = normalized;
    emit queryChanged();
    rebuild();
}

void AppSearchModel::rebuild()
{
    struct Scored {
        int row;
        int score;
    };
    QList<Scored> matches;
    const auto& entries = m_source->entries();
    matches.reserve(entries.size());

    for (int i = 0; i < entries.size(); ++i) {
        const AppEntry& entry = entries.at(i);
        const int base = fuzzyScore(m_query, entry);
        if (base <= 0)
            continue;
        const int score = base + (entry.favorite ? 250 : 0)
            + qMin(entry.frequency * 5, 80);
        matches.append({ i, score });
    }

    std::sort(matches.begin(), matches.end(), [](const Scored& a, const Scored& b) {
        if (a.score != b.score)
            return a.score > b.score;
        return a.row < b.row;
    });

    beginResetModel();
    m_rows.clear();
    m_scores.clear();
    m_rows.reserve(matches.size());
    m_scores.reserve(matches.size());
    for (const Scored& match : std::as_const(matches)) {
        m_rows.append(match.row);
        m_scores.append(match.score);
    }
    endResetModel();
    emit countChanged();
}

QString AppSearchModel::idAt(int row) const
{
    if (row < 0 || row >= m_rows.size())
        return {};
    return m_source->entries().at(m_rows.at(row)).id;
}

QString AppSearchModel::nameAt(int row) const
{
    if (row < 0 || row >= m_rows.size())
        return {};
    return m_source->entries().at(m_rows.at(row)).name;
}

AppLauncherService::AppLauncherService(QObject* parent)
    : QObject(parent)
    , m_apps(new AppListModel(this))
    , m_search(new AppSearchModel(m_apps, this))
    , m_iconTimer(new QTimer(this))
{
    m_iconTimer->setInterval(8);
    m_iconTimer->setSingleShot(false);
    connect(m_iconTimer, &QTimer::timeout, this, &AppLauncherService::extractNextIcons);
    connect(m_search, &AppSearchModel::countChanged, this,
        &AppLauncherService::resultCountChanged);

    loadSettings();
    refresh();
}

AppLauncherService::~AppLauncherService() = default;

QObject* AppLauncherService::searchModel() const
{
    return m_search;
}

int AppLauncherService::totalCount() const
{
    return m_apps->entries().size();
}

int AppLauncherService::resultCount() const
{
    return m_search->count();
}

void AppLauncherService::loadSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("launcher"));
    m_favorites = settings.value(QStringLiteral("favorites")).toStringList();
    settings.endGroup();

}

void AppLauncherService::saveStats()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("launcher"));
    settings.setValue(QStringLiteral("favorites"), m_favorites);
    settings.endGroup();
    settings.sync();
}

void AppLauncherService::setScanning(bool scanning)
{
    if (m_scanning == scanning)
        return;
    m_scanning = scanning;
    emit scanningChanged();
}

void AppLauncherService::setError(const QString& error)
{
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

void AppLauncherService::refresh()
{
    if (m_scanning)
        return;
    setError({});
    setScanning(true);
    const quint64 generation = ++m_scanGeneration;

    QtConcurrent::run([this, generation]() {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        QList<AppEntry> entries = scanStartMenu();
        CoUninitialize();
        QMetaObject::invokeMethod(
            this,
            [this, generation, entries = std::move(entries)]() mutable {
                if (generation != m_scanGeneration)
                    return;
                applyEntries(std::move(entries));
            },
            Qt::QueuedConnection);
    });
}

void AppLauncherService::applyEntries(QList<AppEntry> entries)
{
    QSettings settings;
    for (AppEntry& entry : entries) {
        settings.beginGroup(QStringLiteral("launcher/apps/") + entry.id);
        entry.frequency = settings.value(QStringLiteral("frequency"), 0).toInt();
        entry.lastUsed = settings.value(QStringLiteral("lastUsed"), 0).toLongLong();
        settings.endGroup();
        entry.favorite = m_favorites.contains(entry.id);
        if (entry.iconPath.isEmpty()) {
            const QString cached = iconCachePath(entry.id);
            if (QFileInfo::exists(cached))
                entry.iconPath = cached;
        }
    }

    m_apps->setEntries(std::move(entries));
    m_search->rebuild();
    m_ready = true;
    emit readyChanged();
    emit totalCountChanged();
    emit favoritesChanged();
    setScanning(false);
    startIconQueue();
}

void AppLauncherService::startIconQueue()
{
    m_iconQueue.clear();
    m_iconCursor = 0;
    for (const AppEntry& entry : m_apps->entries()) {
        if (entry.iconPath.isEmpty())
            m_iconQueue.append(entry.id);
    }
    if (!m_iconQueue.isEmpty())
        m_iconTimer->start();
}

void AppLauncherService::extractNextIcons()
{
    if (m_iconCursor >= m_iconQueue.size()) {
        m_iconTimer->stop();
        return;
    }

    constexpr int batch = 4;
    const int end = qMin(m_iconCursor + batch, m_iconQueue.size());
    const auto& entries = m_apps->entries();
    for (int i = m_iconCursor; i < end; ++i) {
        const QString id = m_iconQueue.at(i);
        const auto found = std::find_if(entries.begin(), entries.end(),
            [&id](const AppEntry& entry) { return entry.id == id; });
        if (found == entries.end())
            continue;

        const QString source = found->iconSource.isEmpty() ? found->launchPath : found->iconSource;
        QImage image = iconImage(source);
        if (image.isNull() && !found->target.isEmpty())
            image = iconImage(found->target);
        if (image.isNull() && !found->target.isEmpty())
            image = shellItemImage(found->target);
        if (image.isNull() && found->launchPath.startsWith(QStringLiteral("shell:AppsFolder")))
            image = shellItemImage(found->launchPath);
        if (image.isNull())
            continue;
        const QString path = iconCachePath(id);
        if (image.save(path, "PNG"))
            m_apps->setIconPath(id, path);
    }
    m_iconCursor = end;
    if (m_iconCursor >= m_iconQueue.size())
        m_iconTimer->stop();
    emit iconChanged();
}

void AppLauncherService::launch(int searchRow)
{
    const QString id = m_search->idAt(searchRow);
    if (id.isEmpty()) {
        emit launchFailed({}, QStringLiteral("Aucune application sÃ©lectionnÃ©e"));
        return;
    }

    const auto& entries = m_apps->entries();
    const auto found = std::find_if(entries.begin(), entries.end(),
        [&id](const AppEntry& entry) { return entry.id == id; });
    if (found == entries.end())
        return;
    launchEntry(*found);
}

void AppLauncherService::launchById(const QString& id)
{
    const auto& entries = m_apps->entries();
    const auto found = std::find_if(entries.begin(), entries.end(),
        [&id](const AppEntry& entry) { return entry.id == id; });
    if (found != entries.end())
        launchEntry(*found);
}

void AppLauncherService::launchEntry(const AppEntry& entry)
{
    QtConcurrent::run([this, entry]() {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        SHELLEXECUTEINFOW info = {};
        info.cbSize = sizeof(info);
        info.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
        info.lpVerb = L"open";
        info.lpFile = reinterpret_cast<LPCWSTR>(entry.launchPath.utf16());
        info.nShow = SW_SHOWNORMAL;

        const bool ok = ShellExecuteExW(&info);
        const DWORD error = ok ? ERROR_SUCCESS : GetLastError();
        if (ok && info.hProcess)
            CloseHandle(info.hProcess);
        CoUninitialize();

        QMetaObject::invokeMethod(
            this,
            [this, ok, error, entry]() {
                if (!ok) {
                    const QString message = QStringLiteral("Impossible de lancer %1 : %2")
                                                .arg(entry.name, winError(error));
                    emit launchFailed(entry.id, message);
                    return;
                }
                m_apps->touch(entry.id);
                QSettings settings;
                settings.beginGroup(QStringLiteral("launcher/apps/") + entry.id);
                settings.setValue(QStringLiteral("frequency"),
                    settings.value(QStringLiteral("frequency"), 0).toInt() + 1);
                settings.setValue(QStringLiteral("lastUsed"),
                    QDateTime::currentSecsSinceEpoch());
                settings.endGroup();
                settings.sync();
                m_search->rebuild();
                emit launchSucceeded(entry.id, entry.name);
            },
            Qt::QueuedConnection);
    });
}

void AppLauncherService::toggleFavorite(int searchRow)
{
    const QString id = m_search->idAt(searchRow);
    if (id.isEmpty())
        return;
    const bool favorite = !m_favorites.contains(id);
    applyFavorite(id, favorite);
}

void AppLauncherService::applyFavorite(const QString& id, bool favorite)
{
    if (favorite) {
        if (!m_favorites.contains(id))
            m_favorites.append(id);
    } else {
        m_favorites.removeAll(id);
    }
    m_apps->setFavorite(id, favorite);
    saveStats();
    m_search->rebuild();
    emit favoritesChanged();
}

void AppLauncherService::clearQuery()
{
    m_search->setQuery({});
}

QVariantList AppLauncherService::favoriteApps() const
{
    QVariantList result;
    for (const AppEntry& entry : m_apps->entries()) {
        if (!entry.favorite)
            continue;
        QVariantMap item;
        item.insert(QStringLiteral("id"), entry.id);
        item.insert(QStringLiteral("name"), entry.name);
        item.insert(QStringLiteral("icon"), entry.iconPath);
        result.append(item);
        if (result.size() >= 4)
            break;
    }
    return result;
}

void AppLauncherService::setQuery(const QString& query)
{
    m_search->setQuery(query);
}

QStringList AppLauncherService::knownAppNames() const
{
    QStringList names;
    for (const AppEntry& entry : m_apps->entries())
        names.append(entry.name);
    names.sort(Qt::CaseInsensitive);
    return names;
}

bool AppLauncherService::launchByName(const QString& name, QString* errorMessage)
{
    const QString wanted = name.trimmed().toLower();
    const auto& entries = m_apps->entries();
    auto found = std::find_if(entries.begin(), entries.end(), [&wanted](const AppEntry& entry) {
        return entry.name.toLower() == wanted;
    });
    if (found == entries.end()) {
        found = std::find_if(entries.begin(), entries.end(), [&wanted](const AppEntry& entry) {
            return entry.name.toLower().startsWith(wanted);
        });
    }
    if (found == entries.end()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Application introuvable : %1").arg(name);
        return false;
    }

    launchEntry(*found);
    return true;
}
