pragma ComponentBehavior: Bound

import QtQuick
import ".."
import "../components"

// Functional dashboard: the four tabs are real components backed by Windows
// services. The tab row keeps the original Caelestia indicator animation.
Item {
    id: root

    required property var theme

    property real deformScaleX: 1
    property real deformScaleY: 1

    transform: Scale {
        xScale: root.deformScaleX
        yScale: root.deformScaleY
        origin.x: root.width / 2
        origin.y: root.height / 2
    }

    property int activeTab: 0
    readonly property var tabs: [
        { icon: "dashboard", label: "Dashboard" },
        { icon: "queue_music", label: "Media" },
        { icon: "speed", label: "Performance" },
        { icon: "apps", label: "Workspaces" }
    ]

    MouseArea {
        anchors.fill: parent
        onClicked: {}
    }

    Column {
        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        spacing: root.theme.spacingMedium

        Item {
            id: header

            width: parent.width
            height: 50

            Row {
                id: tabRow

                anchors.fill: parent

                Repeater {
                    model: root.tabs

                    Item {
                        id: tabItem

                        required property int index
                        required property var modelData

                        width: tabRow.width / root.tabs.length
                        height: tabRow.height

                        Row {
                            anchors.centerIn: parent
                            spacing: root.theme.spacingSmall

                            MaterialIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                theme: root.theme
                                text: tabItem.modelData.icon
                                iconSize: 18
                                fill: root.activeTab === tabItem.index ? 1 : 0
                                color: root.activeTab === tabItem.index
                                    ? root.theme.primary : root.theme.onSurfaceVariantColour
                            }

                            StyledText {
                                anchors.verticalCenter: parent.verticalCenter
                                theme: root.theme
                                role: "bodySmall"
                                weight: root.activeTab === tabItem.index
                                    ? Font.DemiBold : Font.Medium
                                color: root.activeTab === tabItem.index
                                    ? root.theme.primary : root.theme.onSurfaceVariantColour
                                text: tabItem.modelData.label
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.activeTab = tabItem.index
                        }
                    }
                }
            }

            Rectangle {
                id: indicator

                readonly property real tabWidth: header.width / root.tabs.length

                x: root.activeTab * tabWidth + tabWidth / 2 - width / 2
                y: header.height - 4
                width: tabWidth - root.theme.spacingExtraLarge
                height: 3
                radius: height / 2
                color: root.theme.primary

                Behavior on x {
                    Anim {
                        type: Anim.DefaultSpatial
                    }
                }

                Behavior on width {
                    Anim {
                        type: Anim.DefaultSpatial
                    }
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: root.theme.outlineVariant
                opacity: 0.6
            }
        }

        Flickable {
            id: view

            width: parent.width
            height: parent.height - header.height - root.theme.spacingMedium
            contentWidth: width * root.tabs.length
            contentHeight: height
            interactive: false
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            contentX: root.activeTab * width

            Behavior on contentX {
                Anim {
                    type: Anim.DefaultSpatial
                }
            }

            Row {
                width: view.contentWidth
                height: view.height

                Loader {
                    width: view.width
                    height: view.height
                    sourceComponent: homeComponent
                    asynchronous: false
                }

                Loader {
                    width: view.width
                    height: view.height
                    sourceComponent: mediaComponent
                    asynchronous: false
                }

                Loader {
                    width: view.width
                    height: view.height
                    sourceComponent: performanceComponent
                    asynchronous: false
                }

                Loader {
                    width: view.width
                    height: view.height
                    sourceComponent: workspaceComponent
                    asynchronous: false
                }
            }
        }
    }

    Component {
        id: homeComponent

        DashboardHome {
            theme: root.theme
        }
    }

    Component {
        id: mediaComponent

        DashboardMedia {
            theme: root.theme
        }
    }

    Component {
        id: performanceComponent

        DashboardPerformance {
            theme: root.theme
        }
    }

    Component {
        id: workspaceComponent

        DashboardWorkspaces {
            theme: root.theme
        }
    }

    Keys.onLeftPressed: root.activeTab = Math.max(0, root.activeTab - 1)
    Keys.onRightPressed: root.activeTab = Math.min(root.tabs.length - 1, root.activeTab + 1)
}
