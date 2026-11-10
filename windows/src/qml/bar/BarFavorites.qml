import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

// Vertical application dock in the left bar. Favourites take precedence;
// when none exist, recently used applications fill the strip. Icons come from
// the same cached shell icons as the launcher.
Item {
    id: root

    required property var theme
    required property var panelController
    required property string screenName

    readonly property var items: {
        const result = [];
        const seen = ({});
        const all = Apps.favoriteApps.concat(Apps.recentApps);
        for (let i = 0; i < all.length && result.length < 8; ++i) {
            const app = all[i];
            if (!app || seen[app.id])
                continue;
            seen[app.id] = true;
            result.push(app);
        }
        return result;
    }
    readonly property int shown: Math.min(items.length, 8)

    implicitWidth: 36
    implicitHeight: 76

    function iconUrl(path) {
        return path ? "file:///" + path.replace(/\\/g, "/") : "";
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: column.implicitHeight
        clip: true
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: column

            width: parent.width
            spacing: 4

            Repeater {
                model: root.items.slice(0, 8)

                Item {
                    required property var modelData

                    width: 36
                    height: 36

                    StateLayer {
                        id: dockButton

                        anchors.fill: parent
                        theme: root.theme
                        radius: root.theme.roundingMedium
                        onClicked: Apps.launchById(modelData.id)
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 26
                        height: 26
                        source: root.iconUrl(modelData.icon)
                        visible: modelData.icon !== "" && status === Image.Ready
                        asynchronous: true
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        scale: dockButton.containsMouse ? 1.1 : 1

                        Behavior on scale {
                            Anim { type: Anim.FastSpatial }
                        }
                    }

                    MaterialIcon {
                        anchors.centerIn: parent
                        visible: modelData.icon === ""
                        theme: root.theme
                        text: "apps"
                        iconSize: 22
                        color: root.theme.onSurfaceVariantColour
                    }
                }
            }

            Item {
                width: 36
                height: 36
                visible: root.items.length === 0

                StateLayer {
                    anchors.fill: parent
                    theme: root.theme
                    radius: root.theme.roundingMedium
                    onClicked: root.panelController.togglePanel("launcher", root.screenName, 0)
                }

                MaterialIcon {
                    anchors.centerIn: parent
                    theme: root.theme
                    text: "apps"
                    iconSize: 22
                    color: root.theme.primary
                }
            }
        }
    }
}
