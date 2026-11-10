pragma ComponentBehavior: Bound

import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

// Application launcher. The surface is drawn by ScreenSurface; this item is
// only the content. Discovery, fuzzy matching, icons, favourites and launch
// actions come from the C++ AppLauncherService.
Item {
    id: root

    required property var theme
    required property var controller

    property real deformScaleX: 1
    property real deformScaleY: 1
    property string errorText: ""

    transform: Scale {
        xScale: root.deformScaleX
        yScale: root.deformScaleY
        origin.x: root.width / 2
        origin.y: root.height / 2
    }

    function iconUrl(path) {
        if (!path)
            return "";
        return "file:///" + path.replace(/\\/g, "/");
    }

    function launchCurrent() {
        if (Apps.resultCount <= 0)
            return;
        const row = Math.max(0, Math.min(list.currentIndex, Apps.resultCount - 1));
        Apps.launch(row);
    }

    function focusSearch() {
        Qt.callLater(() => searchInput.forceActiveFocus());
    }

    onVisibleChanged: {
        if (visible) {
            errorText = "";
            focusSearch();
        }
    }

    Connections {
        target: root.controller

        function onCurrentPanelChanged() {
            if (root.controller.currentPanel !== "launcher") {
                searchInput.text = "";
                Apps.clearQuery();
            }
        }
    }

    Connections {
        target: Apps

        function onLaunchSucceeded(id, name) {
            root.controller.closePanel();
        }

        function onLaunchFailed(id, message) {
            root.errorText = message;
            errorTimer.restart();
            root.focusSearch();
        }

        function onResultCountChanged() {
            list.currentIndex = Apps.resultCount > 0 ? 0 : -1;
        }
    }

    Timer {
        id: errorTimer

        interval: 4500
        onTriggered: root.errorText = ""
    }

    Column {
        anchors.fill: parent
        anchors.margins: root.theme.paddingLarge
        spacing: root.theme.spacingMedium

        Item {
            id: header

            width: parent.width
            height: 38

            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: root.theme.spacingSmall

                Logo {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 24
                    height: 24
                    theme: root.theme
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    role: "bodyLarge"
                    weight: Font.DemiBold
                    text: "Caelestia"
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: Apps.scanning ? "Recherche…" : Apps.totalCount + " applications"
                }
            }

            M3Button {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                icon: "refresh"
                variant: "text"
                implicitWidth: 40
                implicitHeight: 40
                enabled: !Apps.scanning
                onClicked: Apps.refresh()
            }
        }

        Rectangle {
            id: searchBox

            width: parent.width
            height: 54
            radius: root.theme.roundingLarge
            color: root.theme.surfaceContainerHigh
            border.width: searchInput.activeFocus ? 2 : 1
            border.color: searchInput.activeFocus
                ? root.theme.primary : root.theme.outlineVariant

            Behavior on border.color {
                CAnim {}
            }

            MaterialIcon {
                anchors.left: parent.left
                anchors.leftMargin: root.theme.paddingMedium
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                text: "search"
                iconSize: 22
                color: root.theme.onSurfaceVariantColour
            }

            TextInput {
                id: searchInput

                anchors.left: parent.left
                anchors.leftMargin: 46
                anchors.right: hint.left
                anchors.rightMargin: root.theme.spacingSmall
                anchors.verticalCenter: parent.verticalCenter
                height: parent.height
                verticalAlignment: TextInput.AlignVCenter
                color: root.theme.onSurfaceColour
                selectionColor: root.theme.primary
                selectedTextColor: root.theme.onPrimaryColour
                font.family: root.theme.bodyFont
                font.pointSize: 15
                font.weight: Font.Medium
                selectByMouse: true
                focus: true
                clip: true

                onTextChanged: Apps.setQuery(text)
                onAccepted: root.launchCurrent()

                Keys.onUpPressed: {
                    if (Apps.resultCount > 0)
                        list.decrementCurrentIndex();
                    event.accepted = true;
                }

                Keys.onDownPressed: {
                    if (Apps.resultCount > 0)
                        list.incrementCurrentIndex();
                    event.accepted = true;
                }

                Keys.onEscapePressed: {
                    root.controller.closePanel();
                    event.accepted = true;
                }

                Text {
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    visible: searchInput.text.length === 0
                    text: "Rechercher une application…"
                    color: root.theme.onSurfaceVariantColour
                    opacity: 0.7
                    font.family: root.theme.bodyFont
                    font.pointSize: 15
                }
            }

            Row {
                id: hint

                anchors.right: parent.right
                anchors.rightMargin: root.theme.paddingMedium
                anchors.verticalCenter: parent.verticalCenter
                spacing: root.theme.spacingExtraSmall

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    opacity: 0.65
                    text: "Entrée"
                }
            }
        }

        Item {
            width: parent.width
            height: Math.max(0, parent.height - header.height - searchBox.height
                - footer.height - root.theme.spacingMedium * 3)

            ListView {
                id: list

                anchors.fill: parent
                model: Apps.searchModel
                clip: true
                spacing: 2
                currentIndex: Apps.resultCount > 0 ? 0 : -1
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: delegate

                    required property int index
                    required property string appId
                    required property string name
                    required property string description
                    required property string icon
                    required property bool favorite

                    width: list.width
                    height: 60

                    Rectangle {
                        anchors.fill: parent
                        radius: root.theme.roundingMedium
                        color: list.currentIndex === delegate.index
                            ? root.theme.secondaryContainer
                            : (hover.hovered ? root.theme.layer(root.theme.onSurfaceColour, 0.06)
                                : "transparent")

                        Behavior on color {
                            CAnim {}
                        }
                    }

                    MouseArea {
                        id: hover

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            list.currentIndex = delegate.index;
                            root.launchCurrent();
                        }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: root.theme.paddingMedium
                            anchors.rightMargin: root.theme.paddingSmall
                            spacing: root.theme.spacingMedium

                            Item {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 42
                                height: 42

                                Image {
                                    anchors.fill: parent
                                    anchors.margins: 3
                                    source: root.iconUrl(delegate.icon)
                                    visible: delegate.icon !== "" && status === Image.Ready
                                    asynchronous: true
                                    fillMode: Image.PreserveAspectFit
                                    smooth: true
                                }

                                MaterialIcon {
                                    anchors.centerIn: parent
                                    visible: delegate.icon === ""
                                    theme: root.theme
                                    text: "apps"
                                    iconSize: 26
                                    color: root.theme.onSurfaceVariantColour
                                }
                            }

                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 42 - star.width
                                    - root.theme.spacingMedium * 3
                                spacing: 1

                                StyledText {
                                    width: parent.width
                                    theme: root.theme
                                    role: "bodyMedium"
                                    weight: Font.Medium
                                    elide: Text.ElideRight
                                    text: delegate.name
                                }

                                StyledText {
                                    width: parent.width
                                    theme: root.theme
                                    role: "labelSmall"
                                    color: root.theme.onSurfaceVariantColour
                                    opacity: 0.72
                                    elide: Text.ElideRight
                                    text: delegate.description
                                }
                            }

                            MaterialIcon {
                                id: star

                                anchors.verticalCenter: parent.verticalCenter
                                theme: root.theme
                                text: delegate.favorite ? "star" : "star_border"
                                fill: delegate.favorite ? 1 : 0
                                iconSize: 20
                                color: delegate.favorite
                                    ? root.theme.primary : root.theme.onSurfaceVariantColour

                                MouseArea {
                                    anchors.fill: parent
                                    anchors.margins: -8
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: Apps.toggleFavorite(delegate.index)
                                }
                            }
                        }
                    }
                }

                add: Transition {
                    ParallelAnimation {
                        NumberAnimation {
                            property: "opacity"
                            from: 0
                            to: 1
                            duration: root.theme.durationDefaultEffects
                        }
                        NumberAnimation {
                            property: "scale"
                            from: 0.98
                            to: 1
                            duration: root.theme.durationDefaultSpatial
                            easing.type: Easing.BezierSpline
                            easing.bezierCurve: [0.38, 1.21, 0.22, 1, 1, 1]
                        }
                    }
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: root.theme.spacingSmall
                visible: Apps.resultCount === 0

                MaterialIcon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    text: Apps.scanning ? "hourglass_top" : "search_off"
                    iconSize: 34
                    color: root.theme.onSurfaceVariantColour
                    opacity: 0.75
                }

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "bodyMedium"
                    color: root.theme.onSurfaceVariantColour
                    text: Apps.scanning ? "Chargement des applications…"
                        : "Aucune application trouvée"
                }
            }
        }

        Item {
            id: footer

            width: parent.width
            height: 18

            StyledText {
                anchors.left: parent.left
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                opacity: 0.6
                text: "↑ ↓ naviguer  ·  Entrée lancer  ·  Échap fermer"
            }

            StyledText {
                anchors.right: parent.right
                theme: root.theme
                role: "labelSmall"
                color: root.theme.error
                opacity: root.errorText !== "" ? 1 : 0
                text: root.errorText

                Behavior on opacity {
                    Anim {
                        type: Anim.DefaultEffects
                    }
                }
            }
        }
    }
}
