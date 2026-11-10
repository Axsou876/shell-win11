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
            Layout.preferredWidth: root.theme.barInnerWidth
            Layout.preferredHeight: 19
            Layout.topMargin: root.theme.barVPadding

            Logo {
                anchors.centerIn: parent
                width: 26
                height: 26
                theme: root.theme
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
        }
    }
}

