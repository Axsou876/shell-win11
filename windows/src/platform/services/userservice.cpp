#include "userservice.hpp"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSysInfo>

#include <windows.h>
#include <lm.h>
#include <shlobj.h>

namespace {
QString fullName(const QString& user)
{
    if (user.isEmpty())
        return {};

    USER_INFO_2* info = nullptr;
    if (NetUserGetInfo(nullptr, reinterpret_cast<LPCWSTR>(user.utf16()), 2,
            reinterpret_cast<LPBYTE*>(&info))
            == NERR_Success
        && info) {
        QString result = QString::fromWCharArray(info->usri2_full_name).trimmed();
        NetApiBufferFree(info);
        return result;
    }
    return {};
}

QString fullDisplayName(const QString& user)
{
    const QString full = fullName(user);
    return full.isEmpty() ? user : full;
}

QString accountPicture()
{
    PWSTR path = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_AccountPictures, KF_FLAG_DEFAULT, nullptr, &path)))
        return {};

    const QString root = QString::fromWCharArray(path);
    CoTaskMemFree(path);

    QString newest;
    QDateTime newestTime;
    QDirIterator it(root, { QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"),
                              QStringLiteral("*.png"), QStringLiteral("*.bmp") },
        QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString file = it.next();
        const QFileInfo info(file);
        if (info.size() <= 0)
            continue;
        if (newest.isEmpty() || info.lastModified() > newestTime) {
            newest = file;
            newestTime = info.lastModified();
        }
    }
    return newest;
}
} // namespace

UserService::UserService(QObject* parent)
    : QObject(parent)
    , m_userName(qEnvironmentVariable("USERNAME"))
    , m_windowsVersion(QSysInfo::prettyProductName())
    , m_hostName(QSysInfo::machineHostName())
{
    m_displayName = fullDisplayName(m_userName);
    loadAvatar();
}

void UserService::loadAvatar()
{
    m_avatarPath = accountPicture();

    // Windows keeps a generic account picture in ProgramData. It is a real
    // system-provided image, used only when no personal account picture exists.
    if (m_avatarPath.isEmpty()) {
        const QString generic = QDir::fromNativeSeparators(
            qEnvironmentVariable("ProgramData"))
            + QStringLiteral("/Microsoft/User Account Pictures/user.bmp");
        if (QFileInfo::exists(generic))
            m_avatarPath = generic;
    }
}

