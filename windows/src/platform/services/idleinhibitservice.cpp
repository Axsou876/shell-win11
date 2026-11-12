#include "idleinhibitservice.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QTime>

#include <windows.h>

namespace {
void logState(const char* state, const QString& detail = QString())
{
    if (qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_STATUS"))
        return;
    qInfo().noquote() << "Idle inhibit:" << state << detail;
}
} // namespace

IdleInhibitService::IdleInhibitService(QObject* parent)
    : QObject(parent)
{
    // Releasing the request before the process exits keeps `powercfg
    // /requests` clean; the OS would drop it anyway.
    if (QCoreApplication::instance())
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this,
            [this]() { apply(false); });
}

IdleInhibitService::~IdleInhibitService()
{
    apply(false);
}

bool IdleInhibitService::active() const
{
    return m_active;
}

QString IdleInhibitService::since() const
{
    return m_since;
}

void IdleInhibitService::setActive(bool active)
{
    apply(active);
}

void IdleInhibitService::toggle()
{
    apply(!m_active);
}

void IdleInhibitService::apply(bool active)
{
    if (active == m_active)
        return;

    if (!active) {
        if (m_request) {
            PowerClearRequest(m_request, PowerRequestDisplayRequired);
            PowerClearRequest(m_request, PowerRequestSystemRequired);
            CloseHandle(m_request);
            m_request = nullptr;
        }

        m_active = false;
        m_since.clear();
        logState("off");
        emit changed();
        return;
    }

    if (!m_request) {
        REASON_CONTEXT context {};
        context.Version = POWER_REQUEST_CONTEXT_VERSION;
        context.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
        context.Reason.SimpleReasonString
            = const_cast<LPWSTR>(L"Caelestia : garder le PC éveillé");

        m_request = PowerCreateRequest(&context);
        if (!m_request) {
            logState("create failed", QString::number(GetLastError()));
            emit actionFailed(QStringLiteral("Windows n'a pas créé la demande d'éveil."));
            return;
        }
    }

    // SystemRequired alone would keep the machine awake with the screen off;
    // DisplayRequired is what "keep awake" means to the user.
    const bool ok = PowerSetRequest(m_request, PowerRequestSystemRequired)
        && PowerSetRequest(m_request, PowerRequestDisplayRequired);

    if (!ok) {
        const DWORD error = GetLastError();
        PowerClearRequest(m_request, PowerRequestDisplayRequired);
        PowerClearRequest(m_request, PowerRequestSystemRequired);
        CloseHandle(m_request);
        m_request = nullptr;
        logState("set failed", QString::number(error));
        emit actionFailed(QStringLiteral("Windows a refusé de garder le PC éveillé."));
        return;
    }

    m_active = true;
    m_since = QTime::currentTime().toString(QStringLiteral("HH:mm"));
    logState("on", m_since);
    emit changed();
}
