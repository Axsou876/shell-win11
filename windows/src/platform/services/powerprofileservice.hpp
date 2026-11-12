#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class QTimer;

// Power profile selection, the Windows counterpart of
// org.freedesktop.UPower.PowerProfiles.profile.
//
// Windows 11 exposes "Mode d'alimentation" as a *power overlay* on top of the
// legacy power scheme (plan). The overlay is the three-way power-saver /
// balanced / performance switch the Settings app shows, so it is what this
// service drives when powrprof exports PowerSetActiveOverlayScheme. The
// current overlay is read from HKLM (ActiveOverlayAcPowerScheme /
// ActiveOverlayDcPowerScheme): PowerGetActiveOverlayScheme only exists on
// Windows 11 22621+, and PowerGetEffectiveOverlayScheme reports what the
// active scheme yields, not what the user picked.
//
// When the overlay API is absent, the service falls back to enumerating and
// switching power schemes, which every Windows version supports. Both paths
// publish the same shape so the popout does not have to care: `profiles` is a
// list of `{ id, name, icon }` and `activeProfileId` is the id in force.
class PowerProfileService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool available READ available NOTIFY changed)
    // True when the overlay is in use, false when the legacy schemes are.
    Q_PROPERTY(bool overlayMode READ overlayMode NOTIFY changed)
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY changed)
    Q_PROPERTY(QString activeProfileId READ activeProfileId NOTIFY changed)

public:
    explicit PowerProfileService(QObject* parent = nullptr);
    ~PowerProfileService() override;

    [[nodiscard]] bool available() const;
    [[nodiscard]] bool overlayMode() const;
    [[nodiscard]] QVariantList profiles() const;
    [[nodiscard]] QString activeProfileId() const;

    Q_INVOKABLE void setProfile(const QString& id);
    Q_INVOKABLE void refresh();

signals:
    void changed();
    void actionFailed(const QString& message);

private:
    void readOverlay();
    void readSchemes();
    void publish(bool available, bool overlayMode, const QVariantList& profiles,
        const QString& activeProfileId);
    bool applyOverlay(const QString& id);
    bool applyScheme(const QString& id);

    QTimer* m_poll = nullptr;
    bool m_available = false;
    bool m_overlayMode = false;
    QVariantList m_profiles;
    QString m_activeProfileId;
};
