#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <memory>

class AudioService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool present READ present NOTIFY changed)
    Q_PROPERTY(bool known READ known NOTIFY changed)
    Q_PROPERTY(int volume READ volume NOTIFY changed)
    Q_PROPERTY(bool muted READ muted NOTIFY changed)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY changed)

    // Every active endpoint, as a list of { id, name } maps. The default
    // device is exposed separately so the UI can compare ids instead of
    // rebuilding the list whenever the default changes.
    Q_PROPERTY(QVariantList outputs READ outputs NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList inputs READ inputs NOTIFY devicesChanged)
    Q_PROPERTY(QString defaultOutputId READ defaultOutputId NOTIFY changed)
    Q_PROPERTY(QString defaultInputId READ defaultInputId NOTIFY changed)

public:
    explicit AudioService(QObject* parent = nullptr);
    ~AudioService() override;

    [[nodiscard]] bool present() const;
    [[nodiscard]] bool known() const;
    [[nodiscard]] int volume() const;
    [[nodiscard]] bool muted() const;
    [[nodiscard]] QString deviceName() const;
    [[nodiscard]] QVariantList outputs() const;
    [[nodiscard]] QVariantList inputs() const;
    [[nodiscard]] QString defaultOutputId() const;
    [[nodiscard]] QString defaultInputId() const;

    void refreshDefaultDevice();
    void refreshVolume();
    void setVolume(double scalar, bool muted);

    Q_INVOKABLE void setVolumePercent(int percent);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshDevices();
    // Return false when Windows refuses the switch (device gone, policy
    // locked by another app); the UI keeps the previous default then.
    Q_INVOKABLE bool setDefaultOutput(const QString& id);
    Q_INVOKABLE bool setDefaultInput(const QString& id);

signals:
    void changed();
    void devicesChanged();

private:
    void setState(bool present, bool known, int volume, bool muted, const QString& deviceName);
    void refreshDefaultInput();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_present = false;
    bool m_known = false;
    int m_volume = 0;
    bool m_muted = false;
    QString m_deviceName;
    QVariantList m_outputs;
    QVariantList m_inputs;
    QString m_defaultOutputId;
    QString m_defaultInputId;
};
