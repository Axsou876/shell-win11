#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>

class BatteryService final : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

    Q_PROPERTY(bool present READ present NOTIFY changed)
    Q_PROPERTY(bool known READ known NOTIFY changed)
    Q_PROPERTY(int level READ level NOTIFY changed)
    Q_PROPERTY(bool charging READ charging NOTIFY changed)
    Q_PROPERTY(bool onAc READ onAc NOTIFY changed)
    Q_PROPERTY(int minutesRemaining READ minutesRemaining NOTIFY changed)

public:
    explicit BatteryService(QObject* parent = nullptr);
    ~BatteryService() override;

    [[nodiscard]] bool present() const;
    [[nodiscard]] bool known() const;
    [[nodiscard]] int level() const;
    [[nodiscard]] bool charging() const;
    [[nodiscard]] bool onAc() const;
    [[nodiscard]] int minutesRemaining() const;

    void refresh();

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void changed();

private:
    void setState(bool present, bool known, int level, bool charging, bool onAc, int minutesRemaining);

    bool m_present = false;
    bool m_known = false;
    int m_level = -1;
    bool m_charging = false;
    bool m_onAc = false;
    int m_minutesRemaining = -1;
};
