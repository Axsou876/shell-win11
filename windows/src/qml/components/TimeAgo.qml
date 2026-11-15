import QtQuick

// A notification timestamp rendered as a short relative label. The tick lives
// here so a panel that stays open does not show I18n.t("à l'instant") an hour later,
// and so every consumer does not need a timer of its own.
StyledText {
    id: root

    // Milliseconds since the epoch, as exposed by NotificationService.
    property double timestamp: 0
    property double now: Date.now()

    readonly property int ageSeconds: root.timestamp > 0
        ? Math.max(0, Math.round((root.now - root.timestamp) / 1000)) : -1

    text: {
        if (root.ageSeconds < 0)
            return "";
        if (root.ageSeconds < 45)
            return I18n.t("à l’instant");
        if (root.ageSeconds < 3600)
            return "il y a " + Math.max(1, Math.round(root.ageSeconds / 60)) + " min";
        if (root.ageSeconds < 86400)
            return "il y a " + Math.round(root.ageSeconds / 3600) + " h";
        return Qt.formatDateTime(new Date(root.timestamp), "dd/MM HH:mm");
    }

    // Coarse on purpose: the label only changes at minute scale.
    Timer {
        interval: 20000
        repeat: true
        running: root.timestamp > 0
        onTriggered: root.now = Date.now()
    }
}
