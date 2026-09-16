/* SPDX-FileCopyrightText: 2026 Latte Dock Contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */
import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami
Item {
    id: root
    property var windows: []
    signal activate(string uuid)
    // Capture readiness never changes card geometry or starts a layout loop.
    width: Math.max(1, Math.min(windows.length, 3)) * 248
    height: Math.max(1, Math.ceil(windows.length / 3)) * 178
    Grid {
        columns: 3
        Repeater {
            model: root.windows
            delegate: Item {
                id: card
                required property var modelData
                width: 248
                height: 178
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 4
                    color: hover.hovered ? Kirigami.Theme.hoverColor : Kirigami.Theme.backgroundColor
                    radius: 6
                }
                Loader {
                    id: capture
                    x: 8; y: 8
                    width: 232; height: 130
                    active: !card.modelData.minimized
                    asynchronous: true
                    property string uuid: card.modelData.uuid
                    source: "Capture.qml"
                }
                Label {
                    anchors.centerIn: capture
                    visible: !capture.item || !capture.item.ready
                    text: card.modelData.minimized ? qsTr("Minimized") : qsTr("Loading preview…")
                }
                Label {
                    x: 8; y: 142
                    width: 232; height: 28
                    text: card.modelData.title
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                }
                HoverHandler { id: hover }
                TapHandler { onTapped: root.activate(card.modelData.uuid) }
            }
        }
    }
}
