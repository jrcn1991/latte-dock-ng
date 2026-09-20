/* SPDX-FileCopyrightText: 2026 Latte Dock Contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Effects
import org.kde.kirigami as Kirigami
import org.kde.ksvg as KSvg
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.private.mpris as Mpris

Item {
    id: root

    property var windows: []
    property int edge: PlasmaCore.Types.BottomEdge
    // Set by the helper from Plasma::Theme. A standalone QQuickView does not
    // inherit plasmashell's Kirigami palette even though its FrameSvg does use
    // the active Plasma artwork.
    property color popupTextColor: "white"

    signal activate(string uuid)
    signal closeRequested(string uuid)

    readonly property bool vertical: edge === PlasmaCore.Types.LeftEdge
                                     || edge === PlasmaCore.Types.RightEdge
    readonly property real cardWidth: Kirigami.Units.gridUnit * 16
    readonly property real previewHeight: Kirigami.Units.gridUnit * 8
    readonly property real headerHeight: appNameMetrics.implicitHeight
                                         + titleMetrics.implicitHeight * 2
    readonly property var playerData: windows.length > 0
        ? mpris2Source.playerForLauncherUrl(String(windows[0].launcherUrl || ""),
                                            Number(windows[0].appPid || 0))
        : null
    readonly property bool hasPlayer: playerData !== null && playerData.canControl
    readonly property real mediaHeight: hasPlayer ? Kirigami.Units.gridUnit * 2 : 0
    readonly property real cardHeight: headerHeight + Kirigami.Units.smallSpacing + previewHeight
                                       + (hasPlayer ? Kirigami.Units.smallSpacing + mediaHeight : 0)
    readonly property real contentLength: windows.length > 0
        ? windows.length * (vertical ? cardHeight : cardWidth)
          + (windows.length - 1) * Kirigami.Units.gridUnit
        : (vertical ? cardHeight : cardWidth)

    width: background.margins.left + background.margins.right
           + (vertical ? cardWidth
                       : Math.min(contentLength,
                                  Screen.desktopAvailableWidth
                                  - background.margins.left - background.margins.right
                                  - Kirigami.Units.smallSpacing * 2))
    height: background.margins.top + background.margins.bottom
            + (vertical
               ? Math.min(contentLength,
                          Screen.desktopAvailableHeight
                          - background.margins.top - background.margins.bottom
                          - Kirigami.Units.smallSpacing * 2)
               : cardHeight)

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    Mpris.Mpris2Model {
        id: mpris2Source
    }

    // The Plasma task manager puts its delegates inside a Plasma dialog. The
    // helper is a layer-shell QQuickView instead, so it must provide the same
    // themed dialog frame explicitly or the transparent surface has no shared
    // popup background.
    KSvg.FrameSvgItem {
        id: background
        anchors.fill: parent
        imagePath: "widgets/background"
    }

    Kirigami.Heading {
        id: appNameMetrics
        visible: false
        level: 3
        text: "M"
    }

    PlasmaComponents.Label {
        id: titleMetrics
        visible: false
        text: "M"
    }

    ListView {
        id: previewList

        x: background.margins.left
        y: background.margins.top
        width: parent.width - background.margins.left - background.margins.right
        height: parent.height - background.margins.top - background.margins.bottom
        model: root.windows
        orientation: root.vertical ? ListView.Vertical : ListView.Horizontal
        spacing: Kirigami.Units.gridUnit
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        delegate: ColumnLayout {
            id: card

            required property var modelData
            required property int index

            width: root.cardWidth
            height: root.cardHeight
            spacing: Kirigami.Units.smallSpacing

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: root.headerHeight

                RowLayout {
                    anchors.fill: parent
                    spacing: Kirigami.Units.smallSpacing

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 0

                        Kirigami.Heading {
                            level: 3
                            Layout.fillWidth: true
                            maximumLineCount: 1
                            elide: Text.ElideRight
                            text: String(card.modelData.appName || "")
                            color: root.popupTextColor
                            opacity: card.index === 0 ? 1 : 0
                            textFormat: Text.PlainText
                        }

                        PlasmaComponents.Label {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            maximumLineCount: 2
                            wrapMode: Text.Wrap
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                            text: root.windowTitle(card.modelData)
                            color: root.popupTextColor
                            opacity: 0.75
                            textFormat: Text.PlainText
                        }
                    }

                    PlasmaComponents.ToolButton {
                        Layout.alignment: Qt.AlignTop
                        icon.name: "window-close"
                        icon.color: root.popupTextColor
                        onClicked: root.closeRequested(String(card.modelData.uuid))

                        PlasmaComponents.ToolTip.text: qsTr("Close window")
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: root.previewHeight
                clip: true

                PlasmaExtras.Highlight {
                    anchors.fill: parent
                    visible: hover.hovered
                    hovered: true
                }

                Loader {
                    id: capture
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.smallSpacing * 2
                    active: !card.modelData.minimized
                    asynchronous: true
                    property string uuid: String(card.modelData.uuid)
                    readonly property bool captureReady: (item as Capture)?.ready ?? false
                    sourceComponent: Capture {
                        uuid: capture.uuid
                    }

                    // Plasma's task preview adds a soft shadow around a ready
                    // thumbnail. Keep the capture item visible: hiding a
                    // PipeWireSourceItem pauses frame delivery.
                    layer.enabled: captureReady
                    layer.effect: MultiEffect {
                        shadowEnabled: true
                        shadowColor: "black"
                        shadowBlur: 0.5
                        shadowVerticalOffset: 3
                    }
                }

                PlasmaComponents.Label {
                    anchors.centerIn: parent
                    visible: !capture.captureReady
                    text: card.modelData.minimized ? qsTr("Minimized") : qsTr("Loading preview…")
                    color: root.popupTextColor
                }

                HoverHandler {
                    id: hover
                }

                TapHandler {
                    onTapped: root.activate(String(card.modelData.uuid))
                }
            }

            // Match Plasma's ordering: metadata and transport controls sit
            // below the thumbnail, and only the first member owns group media.
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: root.mediaHeight
                spacing: Kirigami.Units.smallSpacing
                visible: card.index === 0 && root.hasPlayer
                enabled: root.playerData?.canControl ?? false

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    PlasmaComponents.Label {
                        Layout.fillWidth: true
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        text: root.playerData?.track ?? ""
                        color: root.popupTextColor
                        textFormat: Text.PlainText
                    }

                    PlasmaExtras.DescriptiveLabel {
                        Layout.fillWidth: true
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        text: root.playerData?.artist ?? ""
                        color: root.popupTextColor
                        visible: text.length > 0
                        textFormat: Text.PlainText
                    }
                }

                PlasmaComponents.ToolButton {
                    enabled: root.playerData?.canGoPrevious ?? false
                    icon.name: mirrored ? "media-skip-forward" : "media-skip-backward"
                    icon.color: root.popupTextColor
                    onClicked: root.playerData.Previous()
                }

                PlasmaComponents.ToolButton {
                    readonly property bool playing: root.playerData?.playbackStatus === Mpris.PlaybackStatus.Playing
                    enabled: (playing ? root.playerData?.canPause : root.playerData?.canPlay) ?? false
                    icon.name: playing ? "media-playback-pause" : "media-playback-start"
                    icon.color: root.popupTextColor
                    onClicked: playing ? root.playerData.Pause() : root.playerData.Play()
                }

                PlasmaComponents.ToolButton {
                    enabled: root.playerData?.canGoNext ?? false
                    icon.name: mirrored ? "media-skip-backward" : "media-skip-forward"
                    icon.color: root.popupTextColor
                    onClicked: root.playerData.Next()
                }
            }
        }
    }

    function windowTitle(windowData) {
        const title = String(windowData.title || "");
        const appName = String(windowData.appName || "");
        const separator = title.match(/\s+(—|-|–)\s+/);
        if (!separator || appName.length === 0) {
            return title;
        }

        const suffix = title.slice(separator.index + separator[0].length);
        return suffix === appName ? (title.slice(0, separator.index) || "—") : title;
    }
}
