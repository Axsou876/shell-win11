#include "powerservice.hpp"

#include <windows.h>
#include <powrprof.h>

namespace {
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
} // namespace

PowerService::PowerService(QObject* parent)
    : QObject(parent)
{
}

bool PowerService::lock()
{
    if (LockWorkStation())
        return true;
    fail(QStringLiteral("verrouillage"), GetLastError());
    return false;
}

bool PowerService::sleep()
{
    if (SetSuspendState(FALSE, FALSE, FALSE))
        return true;
    fail(QStringLiteral("mise en veille"), GetLastError());
    return false;
}

bool PowerService::restart()
{
    if (!enableShutdownPrivilege())
        return false;
    if (ExitWindowsEx(EWX_REBOOT | EWX_FORCEIFHUNG,
            SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED))
        return true;
    fail(QStringLiteral("redémarrage"), GetLastError());
    return false;
}

bool PowerService::shutdown()
{
    if (!enableShutdownPrivilege())
        return false;
    if (ExitWindowsEx(EWX_POWEROFF | EWX_FORCEIFHUNG,
            SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED))
        return true;
    fail(QStringLiteral("arrêt"), GetLastError());
    return false;
}

bool PowerService::signOut()
{
    if (ExitWindowsEx(EWX_LOGOFF | EWX_FORCEIFHUNG, SHTDN_REASON_MAJOR_OTHER))
        return true;
    fail(QStringLiteral("déconnexion"), GetLastError());
    return false;
}

bool PowerService::enableShutdownPrivilege()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        fail(QStringLiteral("privilège d'arrêt"), GetLastError());
        return false;
    }

    LUID luid = {};
    if (!LookupPrivilegeValueW(nullptr, SE_SHUTDOWN_NAME, &luid)) {
        const DWORD error = GetLastError();
        CloseHandle(token);
        fail(QStringLiteral("privilège d'arrêt"), error);
        return false;
    }

    TOKEN_PRIVILEGES privileges = {};
    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Luid = luid;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    const BOOL ok = AdjustTokenPrivileges(token, FALSE, &privileges, 0, nullptr, nullptr);
    const DWORD error = GetLastError();
    CloseHandle(token);
    if (!ok || error == ERROR_NOT_ALL_ASSIGNED) {
        fail(QStringLiteral("privilège d'arrêt"), error);
        return false;
    }
    return true;
}

void PowerService::fail(const QString& action, unsigned long error)
{
    emit actionFailed(QStringLiteral("Impossible d'effectuer %1 : %2")
                          .arg(action, winError(static_cast<DWORD>(error))));
}
