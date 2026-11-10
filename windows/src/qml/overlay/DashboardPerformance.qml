import QtQuick
import Caelestia.Platform 1.0
import ".."
import "../components"

Item {
    id: root

    required property var theme

    Column {
        anchors.fill: parent
        spacing: root.theme.spacingMedium

        Row {
            width: parent.width
            height: 172
            spacing: root.theme.spacingMedium

            DashCard {
                width: (parent.width - root.theme.spacingMedium) * 0.54
                height: parent.height
                contentHeight: 110
                theme: root.theme
                title: "Processeur"
                icon: "memory"

                Column {
                    anchors.fill: parent
                    spacing: root.theme.spacingSmall

                    Row {
                        width: parent.width

                        StyledText {
                            theme: root.theme
                            role: "bodyLarge"
                            pointSize: 32
                            weight: Font.DemiBold
                            color: root.theme.primary
                            text: Math.round(System.cpuUsage) + " %"
                        }

                        Item { width: parent.width - 120; height: 1 }

                        StyledText {
                            anchors.verticalCenter: parent.verticalCenter
                            theme: root.theme
                            role: "labelSmall"
                            color: root.theme.onSurfaceVariantColour
                            text: "60 derniÃ¨res secondes"
                        }
                    }

                    Sparkline {
                        width: parent.width
                        height: 44
                        values: System.cpuHistory
                        lineColor: root.theme.primary
                        maximum: 100
                    }

                    StyledText {
                        width: parent.width
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        elide: Text.ElideRight
                        text: System.cpuName
                    }
                }
            }

            DashCard {
                width: parent.width - (parent.width - root.theme.spacingMedium) * 0.54
                    - root.theme.spacingMedium
                height: parent.height
                contentHeight: 110
                theme: root.theme
                title: "Carte graphique"
                icon: "desktop_windows"

                Column {
                    anchors.fill: parent
                    spacing: root.theme.spacingSmall

                    StyledText {
                        theme: root.theme
                        role: "bodyLarge"
                        pointSize: 32
                        weight: Font.DemiBold
                        color: root.theme.secondary
                        text: System.gpuUsageAvailable
                            ? Math.round(System.gpuUsage) + " %" : "N/D"
                    }

                    Sparkline {
                        width: parent.width
                        height: 44
                        values: System.gpuHistory
                        lineColor: root.theme.secondary
                        maximum: 100
                        visible: System.gpuUsageAvailable
                    }

                    StyledText {
                        width: parent.width
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        elide: Text.ElideRight
                        text: System.gpuAvailable
                            ? System.gpuName + " Â· " + Math.round(System.gpuMemoryUsedMb)
                                + " / " + Math.round(System.gpuMemoryTotalMb) + " Mo"
                            : "Aucun GPU dÃ©tectÃ©"
                    }
                }
            }
        }

        Row {
            width: parent.width
            height: 172
            spacing: root.theme.spacingMedium

            DashCard {
                width: (parent.width - root.theme.spacingMedium * 2) / 3
                height: parent.height
                contentHeight: 110
                theme: root.theme
                title: "MÃ©moire"
                icon: "developer_board"

                Column {
                    anchors.fill: parent
                    spacing: root.theme.spacingSmall

                    StyledText {
                        theme: root.theme
                        role: "bodyLarge"
                        pointSize: 26
                        weight: Font.DemiBold
                        text: System.memoryUsedGb.toFixed(1) + " Go"
                    }

                    StyledText {
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        text: "sur " + System.memoryTotalGb.toFixed(1) + " Go Â· "
                            + Math.round(System.memoryPercent) + " %"
                    }

                    Sparkline {
                        width: parent.width
                        height: 38
                        values: System.memoryHistory
                        lineColor: root.theme.tertiary
                        maximum: 100
                    }

                    Meter {
                        width: parent.width
                        theme: root.theme
                        value: System.memoryPercent
                        colour: root.theme.tertiary
                    }
                }
            }

            DashCard {
                width: (parent.width - root.theme.spacingMedium * 2) / 3
                height: parent.height
                contentHeight: 110
                theme: root.theme
                title: "RÃ©seau"
                icon: "swap_vert"

                Column {
                    anchors.fill: parent
                    spacing: root.theme.spacingSmall

                    StyledText {
                        theme: root.theme
                        role: "bodyLarge"
                        pointSize: 22
                        weight: Font.DemiBold
                        text: root.formatRate(System.networkDownKbps)
                    }

                    StyledText {
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        text: "â†“ descendant Â· â†‘ " + root.formatRate(System.networkUpKbps)
                    }

                    Sparkline {
                        width: parent.width
                        height: 60
                        values: System.networkHistory
                        lineColor: root.theme.secondary
                    }
                }
            }

            DashCard {
                width: (parent.width - root.theme.spacingMedium * 2) / 3
                height: parent.height
                contentHeight: 110
                theme: root.theme
                title: "Stockage C:"
                icon: "hard_drive"

                Column {
                    anchors.fill: parent
                    spacing: root.theme.spacingSmall

                    StyledText {
                        theme: root.theme
                        role: "bodyLarge"
                        pointSize: 26
                        weight: Font.DemiBold
                        text: System.diskUsedGb.toFixed(1) + " Go"
                    }

                    StyledText {
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        text: "sur " + System.diskTotalGb.toFixed(1) + " Go Â· "
                            + Math.round(System.diskPercent) + " %"
                    }

                    Item { width: 1; height: 14 }

                    Meter {
                        width: parent.width
                        theme: root.theme
                        value: System.diskPercent
                        colour: root.theme.primary
                    }

                    StyledText {
                        width: parent.width
                        theme: root.theme
                        role: "labelSmall"
                        color: root.theme.onSurfaceVariantColour
                        opacity: 0.7
                        wrapMode: Text.WordWrap
                        text: "Espace utilisÃ© sur le volume systÃ¨me."
                    }
                }
            }
        }
    }

    function formatRate(kbps) {
        if (kbps >= 1000)
            return (kbps / 1000).toFixed(1) + " Mb/s";
        return Math.round(kbps) + " kb/s";
    }
}

