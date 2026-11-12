import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import ".."
import "../components"

// Shell page: what the shell itself can open, and the shortcuts the window
// manager actually managed to register. Windows owns Win+<letter> in many
// cases, so this list is the honest state rather than the intended one.
Flickable {
    id: root

    required property var theme
    required property var controller

    contentWidth: width
    contentHeight: content.implicitHeight
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    readonly property var labels: ({
            "focus-left": "Fenêtre à gauche",
            "focus-right": "Fenêtre à droite",
            "focus-up": "Fenêtre en haut",
            "focus-down": "Fenêtre en bas",
            "focus-next": "Fenêtre suivante",
            "focus-prev": "Fenêtre précédente",
            "move-left": "Déplacer à gauche",
            "move-right": "Déplacer à droite",
            "move-up": "Déplacer en haut",
            "move-down": "Déplacer en bas",
            "resize-width-dec": "Réduire la largeur",
            "resize-width-inc": "Élargir",
            "resize-height-dec": "Réduire la hauteur",
            "resize-height-inc": "Agrandir la hauteur",
            "toggle-floating": "Fenêtre flottante",
            "toggle-fullscreen": "Plein écran",
            "close-window": "Fermer la fenêtre",
            "layout-cycle": "Pavage suivant",
            "layout-back": "Pavage précédent",
            "promote-master": "Promouvoir en maître",
            "toggle-tiling": "Activer/désactiver le pavage",
            "toggle-strict": "Mode strict",
            "emergency-disable": "Désactivation d'urgence",
            "workspace-prev": "Bureau précédent",
            "workspace-next": "Bureau suivant",
            "launcher": "Lanceur",
            "sidebar": "Barre latérale",
            "session": "Menu de session",
            "lock": "Verrouiller",
            "screenshot": "Capture de zone",
            "screenshot-clip": "Capture dans le presse-papiers",
            "terminal": "Terminal",
            "file-manager": "Explorateur",
            "browser": "Navigateur",
            "editor": "Éditeur"
        })

    // The service publishes one entry per default key, so filter to the keys
    // that are actually registered and keep the first (the Windows-native one)
    // per action.
    readonly property var shortcuts: {
        const out = [];
        const seen = {};
        const list = WindowManager.bindings;
        for (let i = 0; i < list.length; ++i) {
            const entry = list[i];
            if (!entry.registered || seen[entry.action])
                continue;
            seen[entry.action] = true;
            out.push(entry);
        }
        return out;
    }

    readonly property int missing: {
        const list = WindowManager.bindings;
        let count = 0;
        for (let i = 0; i < list.length; ++i) {
            if (!list[i].registered && list[i].original)
                ++count;
        }
        return count;
    }

    function label(action) {
        if (root.labels[action] !== undefined)
            return root.labels[action];
        if (action.indexOf("workspace-") === 0)
            return "Bureau " + action.substring(10);
        if (action.indexOf("move-workspace-") === 0)
            return "Déplacer vers le bureau " + action.substring(15);
        return action;
    }

    component ShortcutRow: Item {
        id: row

        required property var theme
        property string title: ""
        property string key: ""

        implicitHeight: 38

        StyledText {
            anchors.left: parent.left
            anchors.right: badge.left
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            theme: row.theme
            role: "bodyMedium"
            elide: Text.ElideRight
            text: row.title
        }

        Rectangle {
            id: badge

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: keyText.implicitWidth + 16
            height: 24
            radius: 6
            color: row.theme.surfaceContainerHigh
            border.width: 1
            border.color: row.theme.outlineVariant

            StyledText {
                id: keyText

                anchors.centerIn: parent
                theme: row.theme
                role: "labelSmall"
                color: row.theme.onSurfaceVariantColour
                text: row.key
            }
        }
    }

    ColumnLayout {
        id: content

        width: root.width
        spacing: root.theme.spacingLarge

        StyledText {
            theme: root.theme
            role: "titleSmall"
            text: "Surfaces du shell"
        }

        Flow {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: [
                    { name: "launcher", label: "Lanceur", icon: "rocket_launch" },
                    { name: "sidebar", label: "Barre latérale", icon: "dock_to_right" },
                    { name: "notifications", label: "Notifications", icon: "notifications" },
                    { name: "dashboard", label: "Tableau de bord", icon: "dashboard" },
                    { name: "power", label: "Session", icon: "power_settings_new" }
                ]

                delegate: M3Button {
                    required property var modelData

                    theme: root.theme
                    text: modelData.label
                    icon: modelData.icon
                    variant: "tonal"
                    onClicked: root.controller.togglePanel(modelData.name,
                        root.controller.currentScreenName, root.controller.anchorY)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 4
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            StyledText {
                Layout.fillWidth: true
                theme: root.theme
                role: "titleSmall"
                text: "Raccourcis actifs"
            }

            StyledText {
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                text: root.shortcuts.length + " enregistrés"
            }

            StyledText {
                theme: root.theme
                role: "labelSmall"
                color: root.theme.error
                visible: root.missing > 0
                text: root.missing + " indisponibles"
            }
        }

        StyledText {
            Layout.fillWidth: true
            theme: root.theme
            role: "labelSmall"
            color: root.theme.onSurfaceVariantColour
            wrapMode: Text.WordWrap
            text: "Windows réserve certains raccourcis ; ceux qui n'ont pas pu être pris "
                + "sont listés comme indisponibles et le shell n'a aucun moyen de les forcer. "
                + "Les raccourcis personnalisés se déclarent dans la configuration du "
                + "gestionnaire de fenêtres."
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Repeater {
                model: root.shortcuts

                delegate: ShortcutRow {
                    required property var modelData

                    Layout.fillWidth: true
                    theme: root.theme
                    title: root.label(modelData.action)
                    key: modelData.key
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
