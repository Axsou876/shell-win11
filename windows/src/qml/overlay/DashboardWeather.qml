import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme

    property bool editing: !Weather.configured
    property string cityInput: Weather.configured ? Weather.city : ""

    function saveCity() {
        if (cityInput.trim().length === 0)
            return;
        Weather.setCity(cityInput);
        root.editing = false;
    }

    Column {
        anchors.fill: parent
        spacing: root.theme.spacingMedium
        visible: root.editing || !Weather.configured

        Item { width: 1; height: 30 }

        MaterialIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            text: "cloud"
            iconSize: 58
            color: root.theme.primary
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            role: "bodyLarge"
            pointSize: 24
            weight: Font.DemiBold
            text: "Configurer la mÃ©tÃ©o"
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(520, parent.width)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            theme: root.theme
            role: "bodyMedium"
            color: root.theme.onSurfaceVariantColour
            text: "Indiquez une ville. Les donnÃ©es proviennent d'Open-Meteo, sans clÃ© API. "
                + "La ville est la seule information enregistrÃ©e localement."
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(440, parent.width)
            height: 52
            radius: root.theme.roundingLarge
            color: root.theme.surfaceContainerHigh
            border.width: cityField.activeFocus ? 2 : 1
            border.color: cityField.activeFocus ? root.theme.primary : root.theme.outlineVariant

            TextInput {
                id: cityField

                anchors.fill: parent
                anchors.leftMargin: root.theme.paddingLarge
                anchors.rightMargin: root.theme.paddingLarge
                verticalAlignment: TextInput.AlignVCenter
                color: root.theme.onSurfaceColour
                selectionColor: root.theme.primary
                selectedTextColor: root.theme.onPrimaryColour
                font.family: root.theme.bodyFont
                font.pointSize: 14
                text: root.cityInput
                selectByMouse: true
                focus: root.editing

                onTextChanged: root.cityInput = text
                onAccepted: root.saveCity()

                Text {
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    visible: cityField.text.length === 0
                    text: "Ex. Paris, MontrÃ©al, Tokyoâ€¦"
                    color: root.theme.onSurfaceVariantColour
                    opacity: 0.65
                    font.family: root.theme.bodyFont
                    font.pointSize: 14
                }
            }
        }

        M3Button {
            anchors.horizontalCenter: parent.horizontalCenter
            theme: root.theme
            text: Weather.loading ? "Rechercheâ€¦" : "Enregistrer"
            icon: "search"
            enabled: !Weather.loading && cityInput.trim().length > 0
            onClicked: root.saveCity()
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(520, parent.width)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: Weather.error !== ""
            theme: root.theme
            role: "labelSmall"
            color: root.theme.error
            text: Weather.error
        }

        M3Button {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: Weather.configured
            theme: root.theme
            text: "Annuler"
            variant: "text"
            onClicked: root.editing = false
        }
    }

    Column {
        anchors.fill: parent
        spacing: root.theme.spacingMedium
        visible: !root.editing && Weather.configured

        Row {
            width: parent.width
            height: 128
            spacing: root.theme.spacingLarge

            Column {
                width: parent.width * 0.55
                spacing: 2

                Row {
                    spacing: root.theme.spacingSmall

                    StyledText {
                        theme: root.theme
                        role: "bodyLarge"
                        pointSize: 26
                        weight: Font.DemiBold
                        text: Weather.city
                    }

                    M3Button {
                        anchors.verticalCenter: parent.verticalCenter
                        theme: root.theme
                        icon: "edit"
                        variant: "text"
                        implicitWidth: 34
                        implicitHeight: 30
                        onClicked: root.editing = true
                    }
                }

                StyledText {
                    theme: root.theme
                    role: "labelSmall"
                    color: root.theme.onSurfaceVariantColour
                    text: Qt.formatDateTime(new Date(), "dddd d MMMM")
                        + (Weather.updatedAt !== "" ? " Â· actualisÃ© Ã  " + Weather.updatedAt : "")
                }

                Row {
                    spacing: root.theme.spacingMedium

                    MaterialIcon {
                        anchors.verticalCenter: parent.verticalCenter
                        theme: root.theme
                        text: Weather.icon
                        iconSize: 58
                        color: root.theme.primary
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter

                        StyledText {
                            theme: root.theme
                            role: "bodyLarge"
                            pointSize: 40
                            weight: Font.DemiBold
                            color: root.theme.primary
                            text: Weather.temperature
                        }

                        StyledText {
                            theme: root.theme
                            role: "bodyMedium"
                            color: root.theme.onSurfaceVariantColour
                            text: Weather.loading ? "Mise Ã  jourâ€¦" : Weather.description
                        }
                    }
                }
            }

            Row {
                width: parent.width * 0.45
                height: parent.height
                spacing: root.theme.spacingMedium

                Repeater {
                    model: [
                        { icon: "water_drop", label: "HumiditÃ©", value: Weather.humidity },
                        { icon: "air", label: "Vent", value: Weather.wind },
                        { icon: "wb_twilight", label: "Lever", value: Weather.sunrise },
                        { icon: "bedtime", label: "Coucher", value: Weather.sunset }
                    ]

                    Column {
                        required property var modelData

                        width: (parent.width - root.theme.spacingMedium * 3) / 4
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2

                        MaterialIcon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            text: modelData.icon
                            iconSize: 22
                            color: root.theme.secondary
                        }

                        StyledText {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            role: "labelSmall"
                            color: root.theme.onSurfaceVariantColour
                            text: modelData.label
                        }

                        StyledText {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            role: "bodyMedium"
                            weight: Font.Medium
                            text: modelData.value
                        }
                    }
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: root.theme.outlineVariant
            opacity: 0.5
        }

        Row {
            width: parent.width
            spacing: root.theme.spacingMedium

            Repeater {
                model: Weather.forecast

                Rectangle {
                    required property var modelData

                    width: (parent.width - root.theme.spacingMedium * 4) / 5
                    height: 164
                    radius: root.theme.roundingLarge
                    color: root.theme.surfaceContainer
                    border.width: 1
                    border.color: Qt.alpha(root.theme.outlineVariant, 0.25)

                    Column {
                        anchors.centerIn: parent
                        spacing: root.theme.spacingSmall

                        StyledText {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            role: "labelSmall"
                            weight: Font.DemiBold
                            color: root.theme.onSurfaceVariantColour
                            text: modelData.day
                        }

                        MaterialIcon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            text: modelData.icon
                            iconSize: 34
                            color: root.theme.primary
                        }

                        StyledText {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            role: "bodyMedium"
                            weight: Font.Medium
                            text: modelData.max + " / " + modelData.min
                        }

                        StyledText {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width - 8
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            theme: root.theme
                            role: "labelSmall"
                            color: root.theme.onSurfaceVariantColour
                            text: modelData.description
                        }
                    }
                }
            }
        }

        StyledText {
            width: parent.width
            visible: Weather.error !== ""
            theme: root.theme
            role: "labelSmall"
            color: root.theme.error
            wrapMode: Text.WordWrap
            text: Weather.error
        }

        M3Button {
            theme: root.theme
            text: "Actualiser"
            icon: "refresh"
            variant: "text"
            enabled: !Weather.loading
            onClicked: Weather.refresh()
        }
    }
}


