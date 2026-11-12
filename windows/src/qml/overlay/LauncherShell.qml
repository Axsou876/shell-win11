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

    readonly property string query: searchInput.text.trim().toLowerCase()
    // One search field, several modes, selected by a ">" prefix — the same
    // model as the Linux shell (AppList.qml::stateForText). ">calc" and
    // ">wallpaper" are specific modes, a bare ">" lists actions.
    readonly property string mode: {
        if (query.startsWith(">calc"))
            return "calc";
        if (query.startsWith(">wal"))
            return "wallpaper";
        if (query.startsWith(">"))
            return "actions";
        return "apps";
    }

    readonly property string expression: mode === "calc" ? query.substring(5).trim() : ""
    readonly property var calcResult: mode === "calc"
        ? Calculator.evaluate(root.expression)
        : ({ ok: false, text: "", error: "" })

    // The Linux action list is user-configured shell commands; on Windows the
    // useful ones are programs and session calls, so the list is fixed. The
    // session entries are the same calls PowerMenu.qml makes.
    readonly property var actions: [
        {
            id: "calc",
            name: "Calculatrice",
            description: "Ouvrir la calculatrice Windows",
            icon: "calculate",
            target: "calc.exe"
        },
        {
            id: "settings",
            name: "Paramètres",
            description: "Ouvrir les réglages du shell et de Windows",
            icon: "settings",
            target: "ms-settings:"
        },
        {
            id: "terminal",
            name: "Terminal",
            description: "Ouvrir Windows Terminal",
            icon: "terminal",
            target: "wt.exe"
        },
        {
            id: "files",
            name: "Explorateur de fichiers",
            description: "Parcourir les fichiers",
            icon: "folder",
            target: "explorer.exe"
        },
        {
            id: "taskmanager",
            name: "Gestionnaire des tâches",
            description: "Voir et gérer les processus",
            icon: "monitoring",
            target: "taskmgr.exe"
        },
        {
            id: "lock",
            name: "Verrouiller",
            description: "Verrouiller la session",
            icon: "lock"
        },
        {
            id: "sleep",
            name: "Veille",
            description: "Mettre l’ordinateur en veille",
            icon: "bedtime"
        }
    ]

    readonly property var actionItems: {
        const needle = mode === "actions" ? query.substring(1).trim() : "";
        const all = actions;
        if (!needle)
            return all;
        // The id doubles as an English alias so ">terminal" works even though
        // the labels are French.
        return all.filter(action => action.name.toLowerCase().includes(needle)
            || action.id.includes(needle)
            || action.description.toLowerCase().includes(needle));
    }

    readonly property var wallpaperItems: {
        const needle = mode === "wallpaper" ? query.substring(4).trim() : "";
        const all = Wallpaper.wallpapers;
        if (!needle)
            return all;
        return all.filter(item => item.name.toLowerCase().includes(needle)
            || item.folder.toLowerCase().includes(needle));
    }

    function iconUrl(path) {
        if (!path)
            return "";
        return "file:///" + path.replace(/\\/g, "/");
    }

    function activeList() {
        if (mode === "wallpaper")
            return wallpaperList;
        if (mode === "actions")
            return actionsList;
        if (mode === "calc")
            return null;
        return list;
    }

    function runAction(action) {
        if (!action)
            return;

        let ok = true;
        if (action.id === "lock") {
            // Shell lock screen; its password check and the Windows handover
            // live behind it, so this action cannot fail.
            root.controller.closePanel();
            Lock.requestLock();
            return;
        } else if (action.id === "sleep")
            ok = Power.sleep();
        else if (action.id === "settings") {
            // The shell's own settings panel; everything it cannot change itself
            // is one click away as a ms-settings: link.
            root.controller.openPanel("nexus", root.controller.currentScreenName,
                Screen.virtualY + Screen.height / 2, "explicit");
            return;
        } else
            ok = Platform.openExternal(action.target);

        if (!ok) {
            root.errorText = "Action impossible : " + action.name;
            errorTimer.restart();
            root.focusSearch();
            return;
        }
        root.controller.closePanel();
    }

    function launchCurrent() {
        if (mode === "calc") {
            if (!calcResult.ok)
                return;
            Platform.copyText(calcResult.text);
            root.controller.closePanel();
            return;
        }

        if (mode === "actions") {
            const rows = actionItems;
            if (rows.length === 0)
                return;
            const row = Math.max(0, Math.min(actionsList.currentIndex, rows.length - 1));
            root.runAction(rows[row]);
            return;
        }

        if (mode === "wallpaper") {
            const rows = wallpaperItems;
            if (rows.length === 0)
                return;
            const row = Math.max(0, Math.min(wallpaperList.currentIndex, rows.length - 1));
            if (Wallpaper.apply(rows[row].path))
                root.controller.closePanel();
            return;
        }

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
            height: 40

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
                    text: root.mode === "wallpaper"
                        ? root.wallpaperItems.length + " fonds d’écran"
                        : root.mode === "actions"
                            ? root.actionItems.length + " actions"
                            : root.mode === "calc"
                                ? "Calculatrice"
                                : (Apps.scanning ? "Recherche…"
                                    : Apps.totalCount + " applications")
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
                text: root.mode === "calc" ? "calculate" : "search"
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

                onTextChanged: {
                    Apps.setQuery(text);
                    list.currentIndex = 0;
                    wallpaperList.currentIndex = 0;
                    actionsList.currentIndex = 0;
                }
                onAccepted: root.launchCurrent()

                Keys.onUpPressed: {
                    const target = root.activeList();
                    if (target && target.count > 0)
                        target.decrementCurrentIndex();
                    event.accepted = true;
                }

                Keys.onDownPressed: {
                    const target = root.activeList();
                    if (target && target.count > 0)
                        target.incrementCurrentIndex();
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
                    text: "Rechercher une application… ou > pour les actions"
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
                visible: root.mode === "apps"
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
                            : (hover.containsMouse ? root.theme.layer(root.theme.onSurfaceColour, 0.06)
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

            ListView {
                id: wallpaperList

                anchors.fill: parent
                visible: root.mode === "wallpaper"
                model: root.wallpaperItems
                clip: true
                spacing: 2
                currentIndex: root.wallpaperItems.length > 0 ? 0 : -1
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: wallpaperDelegate

                    required property int index
                    required property var modelData

                    width: wallpaperList.width
                    height: 64

                    Rectangle {
                        anchors.fill: parent
                        radius: root.theme.roundingMedium
                        color: wallpaperList.currentIndex === wallpaperDelegate.index
                            ? root.theme.secondaryContainer
                            : (wallpaperHover.containsMouse
                                ? root.theme.layer(root.theme.onSurfaceColour, 0.06)
                                : "transparent")
                    }

                    MouseArea {
                        id: wallpaperHover

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            wallpaperList.currentIndex = wallpaperDelegate.index;
                            root.launchCurrent();
                        }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: root.theme.paddingMedium
                            anchors.rightMargin: root.theme.paddingSmall
                            spacing: root.theme.spacingMedium

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 58
                                height: 38
                                radius: root.theme.roundingSmall
                                color: root.theme.surfaceContainerHigh
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    source: "file:///" + wallpaperDelegate.modelData.path.replace(/\\/g, "/")
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                }
                            }

                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 58 - currentMark.width
                                    - root.theme.spacingMedium * 3
                                spacing: 1

                                StyledText {
                                    width: parent.width
                                    theme: root.theme
                                    role: "bodyMedium"
                                    weight: Font.Medium
                                    elide: Text.ElideRight
                                    text: wallpaperDelegate.modelData.name
                                }

                                StyledText {
                                    width: parent.width
                                    theme: root.theme
                                    role: "labelSmall"
                                    color: root.theme.onSurfaceVariantColour
                                    elide: Text.ElideRight
                                    text: wallpaperDelegate.modelData.folder
                                }
                            }

                            MaterialIcon {
                                id: currentMark

                                anchors.verticalCenter: parent.verticalCenter
                                theme: root.theme
                                text: Wallpaper.currentPath
                                    === wallpaperDelegate.modelData.path ? "check_circle" : ""
                                iconSize: 18
                                color: root.theme.primary
                            }
                        }
                    }
                }
            }

            ListView {
                id: actionsList

                anchors.fill: parent
                visible: root.mode === "actions"
                model: root.actionItems
                clip: true
                spacing: 2
                currentIndex: root.actionItems.length > 0 ? 0 : -1
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: actionDelegate

                    required property int index
                    required property var modelData

                    width: actionsList.width
                    height: 60

                    Rectangle {
                        anchors.fill: parent
                        radius: root.theme.roundingMedium
                        color: actionsList.currentIndex === actionDelegate.index
                            ? root.theme.secondaryContainer
                            : (actionHover.containsMouse
                                ? root.theme.layer(root.theme.onSurfaceColour, 0.06)
                                : "transparent")

                        Behavior on color {
                            CAnim {}
                        }
                    }

                    MouseArea {
                        id: actionHover

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            actionsList.currentIndex = actionDelegate.index;
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

                                MaterialIcon {
                                    anchors.centerIn: parent
                                    theme: root.theme
                                    text: actionDelegate.modelData.icon
                                    iconSize: 26
                                    color: root.theme.onSurfaceVariantColour
                                }
                            }

                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 42 - root.theme.spacingMedium * 2
                                spacing: 1

                                StyledText {
                                    width: parent.width
                                    theme: root.theme
                                    role: "bodyMedium"
                                    weight: Font.Medium
                                    elide: Text.ElideRight
                                    text: actionDelegate.modelData.name
                                }

                                StyledText {
                                    width: parent.width
                                    theme: root.theme
                                    role: "labelSmall"
                                    color: root.theme.onSurfaceVariantColour
                                    opacity: 0.72
                                    elide: Text.ElideRight
                                    text: actionDelegate.modelData.description
                                }
                            }
                        }
                    }
                }
            }

            Item {
                id: calcPanel

                anchors.fill: parent
                visible: root.mode === "calc"

                Column {
                    anchors.centerIn: parent
                    width: parent.width
                    spacing: root.theme.spacingMedium

                    MaterialIcon {
                        anchors.horizontalCenter: parent.horizontalCenter
                        theme: root.theme
                        text: "calculate"
                        iconSize: 30
                        color: root.theme.onSurfaceVariantColour
                        opacity: 0.75
                    }

                    StyledText {
                        width: parent.width
                        theme: root.theme
                        role: "bodyLarge"
                        pointSize: 32
                        weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideMiddle
                        text: root.calcResult.ok ? root.calcResult.text : "—"
                    }

                    StyledText {
                        width: parent.width
                        theme: root.theme
                        role: "labelSmall"
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        opacity: 0.8
                        color: root.expression.length === 0 || root.calcResult.ok
                            ? root.theme.onSurfaceVariantColour : root.theme.error
                        text: root.expression.length === 0
                            ? "Exemple : 2^10 * pi"
                            : root.calcResult.ok
                                ? "Entrée copie le résultat"
                                : root.calcResult.error
                    }
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: root.theme.spacingSmall
                visible: root.mode === "wallpaper"
                    ? root.wallpaperItems.length === 0
                    : root.mode === "actions"
                        ? root.actionItems.length === 0
                        : root.mode === "apps" && Apps.resultCount === 0

                MaterialIcon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    text: root.mode === "wallpaper" ? "wallpaper"
                        : root.mode === "actions" ? "search_off"
                        : (Apps.scanning ? "hourglass_top" : "search_off")
                    iconSize: 34
                    color: root.theme.onSurfaceVariantColour
                    opacity: 0.75
                }

                StyledText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    theme: root.theme
                    role: "bodyMedium"
                    color: root.theme.onSurfaceVariantColour
                    text: root.mode === "wallpaper" ? "Aucun fond d’écran trouvé"
                        : root.mode === "actions" ? "Aucune action trouvée"
                        : (Apps.scanning ? "Chargement des applications…"
                            : "Aucune application trouvée")
                }
            }
        }

        Item {
            id: footer

            width: parent.width
            height: 18

            StyledText {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                theme: root.theme
                role: "labelSmall"
                color: root.theme.onSurfaceVariantColour
                opacity: 0.6
                text: root.mode === "calc"
                    ? "Entrée copier le résultat  ·  Échap fermer"
                    : "↑ ↓ naviguer  ·  Entrée "
                        + (root.mode === "actions" ? "exécuter" : "lancer")
                        + "  ·  Échap fermer"
            }

            StyledText {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
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
