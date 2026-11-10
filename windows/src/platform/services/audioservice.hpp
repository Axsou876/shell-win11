#pragma once

#include <QObject>
#include <QString>

#include <memory>

class AudioService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool present READ present NOTIFY changed)
    Q_PROPERTY(bool known READ known NOTIFY changed)
    Q_PROPERTY(int volume READ volume NOTIFY changed)
    Q_PROPERTY(bool muted READ muted NOTIFY changed)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY changed)

public:
    explicit AudioService(QObject* parent = nullptr);
    ~AudioService() override;

    [[nodiscard]] bool present() const;
    [[nodiscard]] bool known() const;
    [[nodiscard]] int volume() const;
    [[nodiscard]] bool muted() const;
    [[nodiscard]] QString deviceName() const;

    void refreshDefaultDevice();
    void refreshVolume();
    void setVolume(double scalar, bool muted);

    Q_INVOKABLE void setVolumePercent(int percent);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void refresh();

signals:
    void changed();

private:
    void setState(bool present, bool known, int volume, bool muted, const QString& deviceName);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_present = false;
    bool m_known = false;
    int m_volume = 0;
    bool m_muted = false;
    QString m_deviceName;
};
