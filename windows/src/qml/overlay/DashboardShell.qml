pragma ComponentBehavior: Bound

import QtQuick
import ".."
import "../components"

// Dashboard reconstructed from the Linux composition (Content.qml / Tabs.qml):
// stacked icon + label tabs, a content-width indicator, horizontal page slide
// and the five Windows pages (Weather is a real tab, Workspaces is kept).
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
        { icon: "cloud", label: "Weather" },
        { icon: "apps", label: "Workspaces" }
    ]

    // Pages are instantiated the first time their tab is opened. Building all
    // five transparent, fullscreen pages at start-up meant every per-second
    // system reading re-evaluated the bindings of four invisible pages and
    // repainted the whole overlay window for nothing.
    property var visited: [true, false, false, false, false]

    function visit(tab) {
        if (tab < 0 || tab >= root.visited.length || root.visited[tab])
            return;
        const next = root.visited.slice();
        next[tab] = true;
        root.visited = next;
    }

    onActiveTabChanged: root.visit(root.activeTab)

    focus: true

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
            height: 48

            // Invisible text metrics so the indicator can use the active tab's
            // real content width, like the Linux Tabs.qml indicator.
            Item {
                visible: false

                Repeater {
                    id: metrics

                    model: root.tabs

                    Text {
                        required property var modelData

                        text: modelData.label
                        font.family: root.theme.bodyFont
                        font.pointSize: 11
                        font.weight: Font.Medium
                    }
                }
            }

            Row {
                id: tabRow

                width: parent.width
                height: 38

                Repeater {
                    model: root.tabs

                    Item {
                        id: tabItem

                        required property int index
                        required property var modelData

                        width: tabRow.width / root.tabs.length
                        height: tabRow.height

                        readonly property bool current: root.activeTab === tabItem.index
                        readonly property real contentWidth: Math.max(20, tabLabel.implicitWidth)

                        StateLayer {
                            anchors.centerIn: parent
                            width: Math.min(tabItem.width - 8, tabItem.contentWidth + 18)
                            height: 34
                            radius: root.theme.roundingMedium
                            theme: root.theme
                            colour: tabItem.current ? root.theme.primary : root.theme.onSurfaceColour
                            onClicked: root.activeTab = tabItem.index
                        }

                        Column {
                            anchors.centerIn: parent
                            spacing: 1

                            MaterialIcon {
                                id: tabIcon

                                anchors.horizontalCenter: parent.horizontalCenter
                                theme: root.theme
                                text: tabItem.modelData.icon
                                iconSize: 20
                                fill: tabItem.current ? 1 : 0
                                color: tabItem.current
                                    ? root.theme.primary : root.theme.onSurfaceVariantColour
                            }

                            StyledText {
                                id: tabLabel

                                anchors.horizontalCenter: parent.horizontalCenter
                                theme: root.theme
                                role: "labelSmall"
                                weight: tabItem.current ? Font.DemiBold : Font.Medium
                                color: tabItem.current
                                    ? root.theme.primary : root.theme.onSurfaceVariantColour
                                text: tabItem.modelData.label
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.activeTab = tabItem.index
                            onWheel: event => {
                                if (event.angleDelta.y < 0)
                                    root.activeTab = Math.min(root.activeTab + 1, root.tabs.length - 1);
                                else if (event.angleDelta.y > 0)
                                    root.activeTab = Math.max(root.activeTab - 1, 0);
                            }
                        }
                    }
                }
            }

            Rectangle {
                id: indicator

                readonly property real tabWidth: header.width / root.tabs.length
                readonly property var activeMetrics: metrics.itemAt(root.activeTab)

                width: activeMetrics ? Math.max(20, activeMetrics.implicitWidth) : 48
                height: 3
                radius: height / 2
                x: root.activeTab * tabWidth + (tabWidth - width) / 2
                y: header.height - 6
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
                    active: root.visited[0]
                }

                Loader {
                    width: view.width
                    height: view.height
                    sourceComponent: mediaComponent
                    asynchronous: false
                    active: root.visited[1]
                }

                Loader {
                    width: view.width
                    height: view.height
                    sourceComponent: performanceComponent
                    asynchronous: false
                    active: root.visited[2]
                }

                Loader {
                    width: view.width
                    height: view.height
                    sourceComponent: weatherComponent
                    asynchronous: false
                    active: root.visited[3]
                }

                Loader {
                    width: view.width
                    height: view.height
                    sourceComponent: workspaceComponent
                    asynchronous: false
                    active: root.visited[4]
                }
            }
        }
    }

    Component {
        id: homeComponent

        DashboardHome {
            theme: root.theme
            animations: root.visible
            onOpenWeather: root.activeTab = 3
        }
    }

    Component {
        id: mediaComponent

        DashboardMedia {
            theme: root.theme
            pageActive: root.visible && root.activeTab === 1
        }
    }

    Component {
        id: performanceComponent

        DashboardPerformance {
            theme: root.theme
            animations: root.visible
        }
    }

    Component {
        id: weatherComponent

        DashboardWeather {
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
