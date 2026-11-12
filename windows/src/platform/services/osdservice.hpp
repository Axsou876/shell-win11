#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QString>
#include <QTimer>

class AudioService;
class BrightnessService;

// Drives the on-screen display (OSD) for volume and brightness.
//
// The Linux shell reacts to XF86 volume/brightness keys, and its OSD is the
// only volume indicator on screen. On Windows the media keys already raise the
// native OSD, so this service must *not* echo every endpoint change: it only
// shows the pill for actions the shell itself originated (the Ctrl+Alt+F9..F12
// hotkeys, or a QML caller that invokes showVolume()/showBrightness() after
// changing a value). The pill lives inside the always-on-top overlay, never
// takes input, and hides itself after a delay.
class OsdService final : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

    Q_PROPERTY(bool visible READ visible NOTIFY changed)
    Q_PROPERTY(QString mode READ mode NOTIFY changed)
    Q_PROPERTY(QString icon READ icon NOTIFY changed)
    Q_PROPERTY(QString label READ label NOTIFY changed)
    Q_PROPERTY(int value READ value NOTIFY changed)
    Q_PROPERTY(bool muted READ muted NOTIFY changed)

public:
    OsdService(AudioService* audio, BrightnessService* brightness, QObject* parent = nullptr);
    ~OsdService() override;

    [[nodiscard]] bool visible() const;
    [[nodiscard]] QString mode() const;
    [[nodiscard]] QString icon() const;
    [[nodiscard]] QString label() const;
    [[nodiscard]] int value() const;
    [[nodiscard]] bool muted() const;

    Q_INVOKABLE void showVolume();
    Q_INVOKABLE void showBrightness();
    Q_INVOKABLE void showMessage(const QString& icon, const QString& label);

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void changed();

private:
    enum HotkeyId {
        BrightnessUp = 0x4F01,
        BrightnessDown = 0x4F02,
        VolumeUp = 0x4F03,
        VolumeDown = 0x4F04,
    };

    void registerHotkeys();
    void unregisterHotkeys();
    void show(const QString& mode, const QString& icon, const QString& label, int value,
        bool muted);
    void hide();

    AudioService* m_audio = nullptr;
    BrightnessService* m_brightness = nullptr;
    QTimer m_hideTimer;
    bool m_visible = false;
    QString m_mode;
    QString m_icon;
    QString m_label;
    int m_value = -1;
    bool m_muted = false;
};
