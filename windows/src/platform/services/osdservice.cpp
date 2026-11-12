#include "osdservice.hpp"

#include "audioservice.hpp"
#include "brightnessservice.hpp"

#include <QCoreApplication>
#include <QDebug>

#include <algorithm>

#include <windows.h>

namespace {
constexpr int kHideDelayMs = 1600;
constexpr int kStep = 5;

QString volumeIcon(int volume, bool muted)
{
    if (muted || volume <= 0)
        return QStringLiteral("volume_off");
    if (volume >= 50)
        return QStringLiteral("volume_up");
    return QStringLiteral("volume_down");
}

QString brightnessIcon(int percent, bool available)
{
    if (!available)
        return QStringLiteral("brightness_alert");
    if (percent <= 33)
        return QStringLiteral("brightness_low");
    if (percent <= 66)
        return QStringLiteral("brightness_medium");
    return QStringLiteral("brightness_high");
}
} // namespace

OsdService::OsdService(AudioService* audio, BrightnessService* brightness, QObject* parent)
    : QObject(parent)
    , m_audio(audio)
    , m_brightness(brightness)
{
    m_hideTimer.setSingleShot(true);
    m_hideTimer.setInterval(kHideDelayMs);
    connect(&m_hideTimer, &QTimer::timeout, this, &OsdService::hide);

    registerHotkeys();
    if (QCoreApplication::instance())
        QCoreApplication::instance()->installNativeEventFilter(this);
}

OsdService::~OsdService()
{
    unregisterHotkeys();
    if (QCoreApplication::instance())
        QCoreApplication::instance()->removeNativeEventFilter(this);
}

bool OsdService::visible() const
{
    return m_visible;
}

QString OsdService::mode() const
{
    return m_mode;
}

QString OsdService::icon() const
{
    return m_icon;
}

QString OsdService::label() const
{
    return m_label;
}

int OsdService::value() const
{
    return m_value;
}

bool OsdService::muted() const
{
    return m_muted;
}

void OsdService::showVolume()
{
    if (!m_audio || !m_audio->present() || !m_audio->known()) {
        show(QStringLiteral("message"), QStringLiteral("volume_off"),
            QStringLiteral("Aucune sortie audio"), -1, false);
        return;
    }

    show(QStringLiteral("volume"), volumeIcon(m_audio->volume(), m_audio->muted()),
        QStringLiteral("Volume"), m_audio->volume(), m_audio->muted());
}

void OsdService::showBrightness()
{
    if (!m_brightness || !m_brightness->available() || !m_brightness->known()) {
        show(QStringLiteral("message"), brightnessIcon(0, false),
            QStringLiteral("Luminosité indisponible"), -1, false);
        return;
    }

    show(QStringLiteral("brightness"),
        brightnessIcon(m_brightness->percent(), true), QStringLiteral("Luminosité"),
        m_brightness->percent(), false);
}

void OsdService::showMessage(const QString& icon, const QString& label)
{
    show(QStringLiteral("message"), icon, label, -1, false);
}

void OsdService::show(
    const QString& mode, const QString& icon, const QString& label, int value, bool muted)
{
    const bool same = m_visible && m_mode == mode && m_icon == icon && m_label == label
        && m_value == value && m_muted == muted;
    m_visible = true;
    m_mode = mode;
    m_icon = icon;
    m_label = label;
    m_value = value;
    m_muted = muted;
    m_hideTimer.start();
    if (!same)
        emit changed();
}

void OsdService::hide()
{
    if (!m_visible)
        return;
    m_visible = false;
    emit changed();
}

void OsdService::registerHotkeys()
{
    struct Binding {
        int id;
        UINT vk;
        const char* name;
    };
    const Binding bindings[] = {
        { BrightnessUp, VK_F12, "brightness-up" },
        { BrightnessDown, VK_F11, "brightness-down" },
        { VolumeUp, VK_F10, "volume-up" },
        { VolumeDown, VK_F9, "volume-down" },
    };

    for (const Binding& binding : bindings) {
        if (RegisterHotKey(nullptr, binding.id, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, binding.vk))
            continue;
        if (!qEnvironmentVariableIsEmpty("CAELESTIA_DEBUG_OSD")) {
            qInfo().noquote() << "OSD hotkey unavailable" << binding.name
                              << "error" << GetLastError();
        }
    }
}

void OsdService::unregisterHotkeys()
{
    for (int id = BrightnessUp; id <= VolumeDown; ++id)
        UnregisterHotKey(nullptr, id);
}

bool OsdService::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
{
    Q_UNUSED(result);

    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
        return false;

    const auto* msg = static_cast<MSG*>(message);
    if (!msg || msg->message != WM_HOTKEY)
        return false;

    switch (static_cast<int>(msg->wParam)) {
    case BrightnessUp:
        if (m_brightness)
            m_brightness->increase(kStep);
        showBrightness();
        return true;
    case BrightnessDown:
        if (m_brightness)
            m_brightness->decrease(kStep);
        showBrightness();
        return true;
    case VolumeUp:
        if (m_audio)
            m_audio->setVolumePercent(std::min(100, m_audio->volume() + kStep));
        showVolume();
        return true;
    case VolumeDown:
        if (m_audio)
            m_audio->setVolumePercent(std::max(0, m_audio->volume() - kStep));
        showVolume();
        return true;
    default:
        return false;
    }
}
