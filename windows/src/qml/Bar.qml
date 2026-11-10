import QtQuick
import QtQuick.Layouts
import "bar"
import "components"

Item {
    id: root

    required property var theme
    required property var panelController
    required property string screenName
    required property real screenY

    ColumnLayout {
        anchors.fill: parent
        spacing: root.theme.spacingMedium

        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            Layout.topMargin: root.theme.barVPadding - 6

            Logo {
                id: barLogo

                anchors.centerIn: parent
                width: 26
                height: 26
                theme: root.theme
                scale: logoArea.containsMouse ? 1.08 : 1

                Behavior on scale {
                    Anim {
                        type: Anim.FastSpatial
                    }
                }
            }

            StateLayer {
                id: logoArea

                anchors.fill: parent
                theme: root.theme
                radius: width / 2
                onClicked: root.panelController.togglePanel("launcher", root.screenName, 0)
            }
        }

        BarWorkspaces {
            Layout.alignment: Qt.AlignHCenter
            theme: root.theme
        }

        Item {
            Layout.fillHeight: true
        }

        BarActiveWindow {
            Layout.alignment: Qt.AlignHCenter
            theme: root.theme
        }

        Item {
            Layout.fillHeight: true
        }

        BarTray {
            Layout.alignment: Qt.AlignHCenter
            theme: root.theme
        }

        BarClock {
            Layout.alignment: Qt.AlignHCenter
            theme: root.theme
        }

        BarStatusIcons {
            Layout.alignment: Qt.AlignHCenter
            theme: root.theme
            panelController: root.panelController
            screenName: root.screenName
            screenY: root.screenY
        }

        BarPower {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: root.theme.barVPadding
            theme: root.theme
            panelController: root.panelController
            screenName: root.screenName
            screenY: root.screenY
            screenHeight: root.height
        }
    }

    Shortcut {
        sequence: "F1"
        onActivated: root.panelController.togglePanel("launcher", root.screenName, 0)
    }
}

