import QtQuick
import QtQuick.Layouts
import Caelestia.Platform 1.0
import ".."
import "../components"

// Weather tab reconstructed from WeatherTab.qml, driven by the real Open-Meteo
// service (city, current conditions, sunrise/sunset and 5-day forecast).
Item {
    id: root

    required property var theme

    property string cityInput: Weather.city

    // --- Configuration -----------------------------------------------------
    Column {
        anchors.centerIn: parent
        width: Math.min(520, parent.width)
        spacing: root.theme.spacingMedium
        visible: !Weather.configured

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
            text: "Configurer la météo"
        }

        StyledText {
            width: parent.width
            theme: root.theme
            role: "bodyMedium"
            color: root.theme.onSurfaceVariantColour
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: "Indiquez une ville. Les données proviennent d’Open-Meteo, sans clé API."
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(440, parent.width)
            height: 52
            radius: root.theme.roundingLarge
            color: root.theme.surfaceContainerHigh
            border.width: cityField.activeFocus ? 2 : 1
            border.color: cityField.activeFocus
                ? root.theme.primary : root.theme.outlineVariant

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
                focus: true
                onTextChanged: root.cityInput = text
                onAccepted: {
                    if (cityInput.trim().length > 0)
                        Weather.setCity(cityInput);
                }

                Text {
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    visible: cityField.text.length === 0
                    text: "Ex. Paris, Montréal, Tokyo…"
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
            text: Weather.loading ? "Recherche…" : "Enregistrer"
            icon: "search"
            enabled: !Weather.loading && cityInput.trim().length > 0
            onClicked: Weather.setCity(cityInput)
        }

        StyledText {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            theme: root.theme
            role: "labelSmall"
            color: root.theme.error
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: Weather.error !== ""
            text: Weather.error
        }
    }

    // --- Weather -----------------------------------------------------------
    ColumnLayout {
        anchors.fill: parent
        spacing: root.theme.spacingMedium
        visible: Weather.configured

        // Header: same 16 px inner margins as WeatherTab.qml.
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: root.theme.paddingLarge
            Layout.rightMargin: root.theme.paddingLarge
            spacing: root.theme.spacingLargeIncreased

            Column {
                Layout.alignment: Qt.AlignVCenter
                spacing: 2

                StyledText {
                    theme: root.theme
                    role: "bodyLarge"
                    pointSize: 28
                    weight: Font.DemiBold
                    color: root.theme.onSurfaceColour
                    text: Weather.city !== "" ? Weather.city : "Chargement…"
                }

                StyledText {
                    theme: root.theme
                    role: "bodySmall"
                    color: root.theme.onSurfaceVariantColour
                    text: Qt.formatDate(new Date(), "dddd d MMMM")
                }
            }

            Item { Layout.fillWidth: true }

            Row {
                Layout.alignment: Qt.AlignVCenter
                spacing: root.theme.spacingLargeIncreased

                Row {
                    spacing: root.theme.spacingSmall

                    MaterialIcon {
                        anchors.verticalCenter: parent.verticalCenter
                        theme: root.theme
                        text: "wb_twilight"
                        iconSize: 26
                        color: root.theme.tertiary
                    }

                    Column {
                        StyledText {
                            theme: root.theme
                            role: "bodySmall"
                            color: root.theme.onSurfaceVariantColour
                            text: "Lever"
                        }

                        StyledText {
                            theme: root.theme
                            role: "labelSmall"
                            weight: Font.DemiBold
                            text: Weather.sunrise !== "" ? Weather.sunrise : "--:--"
                        }
                    }
                }

                Row {
                    spacing: root.theme.spacingSmall

                    MaterialIcon {
                        anchors.verticalCenter: parent.verticalCenter
                        theme: root.theme
                        text: "bedtime"
                        iconSize: 26
                        color: root.theme.tertiary
                    }

                    Column {
                        StyledText {
                            theme: root.theme
                            role: "bodySmall"
                            color: root.theme.onSurfaceVariantColour
                            text: "Coucher"
                        }

                        StyledText {
                            theme: root.theme
                            role: "labelSmall"
                            weight: Font.DemiBold
                            text: Weather.sunset !== "" ? Weather.sunset : "--:--"
                        }
                    }
                }
            }
        }

        // Big current conditions card.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            radius: root.theme.roundingExtraLarge * 2
            color: root.theme.surfaceContainer

            Row {
                anchors.centerIn: parent
                spacing: root.theme.spacingLargeIncreased

                MaterialIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    theme: root.theme
                    text: Weather.available ? Weather.icon : "cloud_off"
                    iconSize: 84
                    color: root.theme.secondary
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: -4

                    StyledText {
                        theme: root.theme
                        role: "bodyLarge"
                        pointSize: 56
                        weight: Font.Medium
                        color: root.theme.primary
                        text: Weather.available ? Weather.temperature : "--"
                    }

                    StyledText {
                        theme: root.theme
                        role: "bodyMedium"
                        color: root.theme.onSurfaceVariantColour
                        text: Weather.available ? Weather.description : "Données indisponibles"
                    }
                }
            }
        }

        // Detail cards.
        RowLayout {
            Layout.fillWidth: true
            spacing: root.theme.spacingMedium

            Repeater {
                model: [
                    { icon: "water_drop", label: "Humidité",
                        value: Weather.humidity !== "" ? Weather.humidity : "N/D",
                        colour: root.theme.secondary },
                    { icon: "thermostat", label: "Ressenti",
                        value: Weather.feelsLike !== "" ? Weather.feelsLike : "N/D",
                        colour: root.theme.primary },
                    { icon: "air", label: "Vent",
                        value: Weather.wind !== "" ? Weather.wind : "N/D",
                        colour: root.theme.tertiary }
                ]

                Rectangle {
                    id: detailCard

                    required property var modelData

                    Layout.fillWidth: true
                    Layout.preferredHeight: 66
                    radius: root.theme.roundingMedium
                    color: root.theme.surfaceContainer

                    Row {
                        anchors.centerIn: parent
                        spacing: root.theme.spacingMedium

                        MaterialIcon {
                            anchors.verticalCenter: parent.verticalCenter
                            theme: root.theme
                            text: detailCard.modelData.icon
                            iconSize: 26
                            color: detailCard.modelData.colour
                        }

                        Column {
                            spacing: 0

                            StyledText {
                                theme: root.theme
                                role: "bodySmall"
                                color: root.theme.onSurfaceVariantColour
                                opacity: 0.8
                                text: detailCard.modelData.label
                            }

                            StyledText {
                                theme: root.theme
                                role: "labelSmall"
                                weight: Font.DemiBold
                                text: detailCard.modelData.value
                            }
                        }
                    }
                }
            }
        }

        // Forecast title: 12 px left inset and 12 px top margin, as Linux.
        StyledText {
            Layout.leftMargin: root.theme.paddingMedium
            Layout.topMargin: root.theme.spacingMedium
            theme: root.theme
            role: "bodyMedium"
            weight: Font.DemiBold
            color: root.theme.onSurfaceColour
            visible: Weather.forecast.length > 0
            text: "Prévisions " + Weather.forecast.length + " jours"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: root.theme.spacingMedium
            visible: Weather.forecast.length > 0

            Repeater {
                model: Weather.forecast

                Rectangle {
                    id: forecastItem

                    required property int index
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    radius: root.theme.roundingLarge
                    color: root.theme.surfaceContainer

                    Column {
                        anchors.centerIn: parent
                        spacing: root.theme.spacingSmall

                        StyledText {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            role: "bodySmall"
                            weight: Font.DemiBold
                            color: root.theme.primary
                            text: forecastItem.index === 0 ? "Aujourd’hui" : forecastItem.modelData.day
                        }

                        StyledText {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            role: "labelSmall"
                            color: root.theme.onSurfaceVariantColour
                            opacity: 0.75
                            text: Qt.formatDate(new Date(forecastItem.modelData.date), "d MMM")
                        }

                        MaterialIcon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            text: forecastItem.modelData.icon
                            iconSize: 28
                            color: root.theme.secondary
                        }

                        StyledText {
                            anchors.horizontalCenter: parent.horizontalCenter
                            theme: root.theme
                            role: "labelSmall"
                            weight: Font.DemiBold
                            color: root.theme.tertiary
                            text: forecastItem.modelData.min + " / " + forecastItem.modelData.max
                        }
                    }
                }
            }
        }
    }
}

