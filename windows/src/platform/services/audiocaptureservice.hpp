#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

#include <atomic>
#include <thread>

// WASAPI loopback capture + FFT spectrum for the Media visualiser. The worker
// owns the audio client, converts each packet to mono float, analyses it and
// publishes only 64 smoothed band values at about 60 Hz. No samples are kept
// and nothing is written to disk.
class AudioCaptureService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList bands READ bands NOTIFY bandsChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY changed)
    Q_PROPERTY(int bandCount READ bandCount CONSTANT)

public:
    explicit AudioCaptureService(QObject* parent = nullptr);
    ~AudioCaptureService() override;

    [[nodiscard]] QVariantList bands() const { return m_bands; }
    [[nodiscard]] bool active() const { return m_active.load(); }
    [[nodiscard]] bool available() const { return m_available; }
    [[nodiscard]] QString error() const { return m_error; }
    [[nodiscard]] QString deviceName() const { return m_deviceName; }
    [[nodiscard]] int bandCount() const;

    Q_INVOKABLE void setActive(bool active);

signals:
    void bandsChanged();
    void activeChanged();
    void changed();

private:
    void startCapture();
    void stopCapture();
    void captureLoop();
    void publishBands(const QVector<float>& bands);
    void publishStatus(bool available, const QString& error, const QString& deviceName);

    std::thread m_thread;
    std::atomic<bool> m_stop { false };
    std::atomic<bool> m_active { false };

    QVariantList m_bands;
    bool m_available = false;
    QString m_error;
    QString m_deviceName;
};
