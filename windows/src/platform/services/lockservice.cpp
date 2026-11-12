#include "lockservice.hpp"

#include <QByteArray>
#include <QMetaObject>
#include <QThreadPool>
#include <QTimer>

#include <windows.h>

#include <string>
#include <utility>
#include <vector>

namespace {
// Credentials must not linger in the process heap after the check. QString
// buffers are shared and immutable, so the copies we own — the UTF-8 input and
// the wide string handed to LogonUserW — are the ones zeroed here.
void wipe(QByteArray& buffer)
{
    if (!buffer.isEmpty())
        SecureZeroMemory(buffer.data(), static_cast<size_t>(buffer.size()));
    buffer.clear();
}

void wipe(std::wstring& text)
{
    if (!text.empty())
        SecureZeroMemory(text.data(), text.size() * sizeof(wchar_t));
    text.clear();
}

std::wstring toWide(const QByteArray& utf8)
{
    if (utf8.isEmpty())
        return {};

    const int length = MultiByteToWideChar(CP_UTF8, 0, utf8.constData(),
        static_cast<int>(utf8.size()), nullptr, 0);
    if (length <= 0)
        return {};

    std::wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.constData(), static_cast<int>(utf8.size()),
        wide.data(), length);
    return wide;
}

QString environment(const char* name)
{
    return QString::fromLocal8Bit(qgetenv(name));
}

struct Attempt {
    bool ok = false;
    unsigned long error = 0;
};

Attempt tryLogon(const QString& user, const QString& domain, const std::wstring& password)
{
    const std::wstring userWide = user.toStdWString();
    const std::wstring domainWide = domain.toStdWString();

    HANDLE token = nullptr;
    const BOOL ok = LogonUserW(userWide.c_str(),
        domainWide.empty() ? nullptr : domainWide.c_str(), password.c_str(),
        LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &token);

    if (ok) {
        // Only the answer matters; the handle itself is never used to start a
        // process or impersonate, so it is closed immediately.
        if (token)
            CloseHandle(token);
        return { true, 0 };
    }
    return { false, GetLastError() };
}

QString failureMessage(unsigned long error)
{
    switch (error) {
    case ERROR_LOGON_FAILURE:
        return QStringLiteral("Mot de passe incorrect.");
    case ERROR_ACCOUNT_RESTRICTION:
        return QStringLiteral("Connexion bloquée par Windows pour ce compte.");
    case ERROR_PASSWORD_EXPIRED:
        return QStringLiteral("Le mot de passe a expiré. Changez-le dans Windows.");
    case ERROR_ACCOUNT_DISABLED:
        return QStringLiteral("Ce compte est désactivé.");
    case ERROR_PASSWORD_MUST_CHANGE:
        return QStringLiteral("Le mot de passe doit être changé dans Windows.");
    case ERROR_ACCOUNT_LOCKED_OUT:
        return QStringLiteral("Le compte est verrouillé.");
    case ERROR_LOGON_TYPE_NOT_GRANTED:
        return QStringLiteral("Ce type de connexion est refusé à ce compte.");
    case ERROR_NO_LOGON_SERVERS:
        return QStringLiteral("Aucun contrôleur de domaine joignable.");
    default:
        return QStringLiteral("Authentification impossible (code %1).").arg(error);
    }
}
} // namespace

LockService::LockService(QObject* parent)
    : QObject(parent)
{
    // Caps Lock still applies to a password typed into the overlay, so the
    // hint has to follow the physical key state while the screen is up.
    m_capsTimer = new QTimer(this);
    m_capsTimer->setInterval(500);
    connect(m_capsTimer, &QTimer::timeout, this, [this]() {
        const bool on = capsLock();
        if (on == m_capsLock)
            return;
        m_capsLock = on;
        emit capsLockChanged();
    });
}

bool LockService::capsLock() const
{
    return (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
}

void LockService::requestLock()
{
    if (m_locked)
        return;
    emit lockRequested();
}

void LockService::show()
{
    if (m_locked)
        return;

    m_locked = true;
    m_attempts = 0;
    m_error.clear();
    m_capsLock = capsLock();
    emit lockedChanged();
    emit blockedChanged();
    emit capsLockChanged();
    emit changed();
    m_capsTimer->start();
}

void LockService::unlock()
{
    if (!m_locked)
        return;

    m_locked = false;
    m_attempts = 0;
    m_error.clear();
    // Any LogonUserW still in flight belongs to the previous visit.
    ++m_requestId;
    m_capsTimer->stop();
    emit lockedChanged();
    emit blockedChanged();
    emit changed();
    emit unlocked();
}

void LockService::authenticate(const QString& password)
{
    if (!m_locked || m_busy || blocked())
        return;

    if (password.isEmpty()) {
        setError(QStringLiteral("Saisissez votre mot de passe."));
        return;
    }

    setBusy(true);

    // Resolve the identity on the GUI thread; the worker only receives plain
    // strings and its own copy of the credentials.
    const QString user = environment("USERNAME");
    const QString domain = environment("USERDOMAIN");
    const QString machine = environment("COMPUTERNAME");

    QByteArray utf8 = password.toUtf8();
    const quint64 id = ++m_requestId;

    // The task outlives nothing: it captures `this`, which is safe because the
    // service lives for the whole session and the queued result is dropped
    // when the request id no longer matches.
    QThreadPool::globalInstance()->start([this, id, user, domain, machine, utf8]() mutable {
        std::wstring wide = toWide(utf8);
        wipe(utf8);

        std::vector<std::pair<QString, QString>> identities;
        identities.emplace_back(user, domain.isEmpty() ? machine : domain);
        if (domain != machine)
            identities.emplace_back(user, machine);
        identities.emplace_back(user, QString());

        bool ok = false;
        unsigned long lastError = ERROR_LOGON_FAILURE;
        for (const auto& identity : identities) {
            const Attempt attempt = tryLogon(identity.first, identity.second, wide);
            if (attempt.ok) {
                ok = true;
                break;
            }
            lastError = attempt.error;
            // Only a plain credential mismatch is worth retrying under another
            // domain spelling. Account-state errors (locked, disabled,
            // expired) would fail identically for every spelling.
            if (attempt.error != ERROR_LOGON_FAILURE
                && attempt.error != ERROR_LOGON_TYPE_NOT_GRANTED
                && attempt.error != ERROR_ACCOUNT_RESTRICTION)
                break;
        }
        wipe(wide);

        const QString message = failureMessage(lastError);
        QMetaObject::invokeMethod(
            this,
            [this, id, ok, message]() {
                if (id != m_requestId)
                    return;
                finishAuthentication(ok, message);
            },
            Qt::QueuedConnection);
    });
}

void LockService::finishAuthentication(bool ok, const QString& message)
{
    setBusy(false);

    // The user may have left the screen through the Windows handover while the
    // worker was checking; the result must not revive anything.
    if (!m_locked)
        return;

    if (ok) {
        unlock();
        return;
    }

    const bool wasBlocked = blocked();
    ++m_attempts;
    setError(m_attempts >= maxAttempts()
            ? QStringLiteral("Trop de tentatives. Utilisez l'écran de verrouillage Windows.")
            : message);
    if (blocked() != wasBlocked)
        emit blockedChanged();
    emit changed();
}

bool LockService::lockWindows()
{
    if (LockWorkStation())
        return true;

    setError(QStringLiteral("Windows a refusé le verrouillage de la session."));
    return false;
}

void LockService::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void LockService::setError(const QString& message)
{
    if (m_error == message)
        return;
    m_error = message;
    emit changed();
}
