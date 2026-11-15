pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import ".."
import "../components"

// Windows port of the Linux Nexus settings surface. The Linux module owns the
// whole desktop (themes, plugins, updates, language); on Windows those belong
// to Settings, so this panel covers only what the shell itself controls and
// hands the rest over with ms-settings: deep links.
Item {
    id: root

    required property var theme
    required property var controller

    implicitWidth: 780
    implicitHeight: 600

    property int page: 0

    readonly property var pages: [
        { label: I18n.t("Apparence"), icon: "palette" },
        { label: I18n.t("Système"), icon: "tune" },
        { label: "Shell", icon: "dock_to_bottom" },
        { label: I18n.t("À propos"), icon: "info" }
    ]

    component NavRow: Item {
        id: nav

        required property var theme
        property string icon: ""
        property string label: ""
        property bool selected: false
        signal clicked()

        implicitHeight: 42

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: nav.selected ? nav.theme.secondaryContainer : "transparent"

            Behavior on color {
                CAnim {}
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 10

            MaterialIcon {
                theme: nav.theme
                text: nav.icon
                iconSize: 20
                color: nav.selected ? nav.theme.onSurfaceColour : nav.theme.onSurfaceVariantColour
            }

            StyledText {
                Layout.fillWidth: true
                theme: nav.theme
                role: "bodyMedium"
                weight: nav.selected ? Font.DemiBold : Font.Normal
                elide: Text.ElideRight
                text: nav.label
            }
        }

        StateLayer {
            anchors.fill: parent
            theme: nav.theme
            radius: nav.height / 2
            onClicked: nav.clicked()
        }
    }

    // A plain Item rather than a RowLayout: a ColumnLayout nested in a RowLayout
    // reported its preferred width but was handed all the extra space, which
    // squeezed the page stack to a few pixels. Fixed anchors are deterministic.
    Item {
        anchors.fill: parent

        ColumnLayout {
            id: navColumn

            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 190
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: 12
                spacing: 10

                Rectangle {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    radius: width / 2
                    color: root.theme.surfaceContainerHighest

                    MaterialIcon {
                        anchors.centerIn: parent
                        theme: root.theme
                        text: "tune"
                        iconSize: 18
                        color: root.theme.primary
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    StyledText {
                        theme: root.theme
                        role: "bodyLarge"
                        weight: Font.DemiBold
                        text: I18n.t("Réglages")
                    }

                    StyledText {
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        text: "Caelestia"
                    }
                }
            }

            Repeater {
                model: root.pages

                delegate: NavRow {
                    required property int index
                    required property var modelData

                    Layout.fillWidth: true
                    theme: root.theme
                    icon: modelData.icon
                    label: modelData.label
                    selected: root.page === index
                    onClicked: root.page = index
                }
            }

            Item {
                Layout.fillHeight: true
            }

            StyledText {
                Layout.fillWidth: true
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                wrapMode: Text.WordWrap
                text: "Version " + Platform.version + "\n" + User.windowsVersion
            }

            M3Button {
                Layout.fillWidth: true
                theme: root.theme
                text: "Fermer"
                icon: "close"
                variant: "text"
                implicitHeight: 32
                onClicked: root.controller.closePanel()
            }
        }

        Rectangle {
            id: divider

            anchors.left: navColumn.right
            anchors.leftMargin: root.theme.spacingLarge
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: root.theme.outlineVariant
            opacity: 0.6
        }

        StackLayout {
            id: pagesStack

            anchors.left: divider.right
            anchors.leftMargin: root.theme.spacingLarge
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            currentIndex: root.page

            NexusAppearance {
                theme: root.theme
                controller: root.controller
            }

            NexusSystem {
                theme: root.theme
                controller: root.controller
            }

            NexusShell {
                theme: root.theme
                controller: root.controller
            }

            NexusAbout {
                theme: root.theme
                controller: root.controller
            }
        }
    }
}
