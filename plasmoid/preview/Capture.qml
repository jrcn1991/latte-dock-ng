/* SPDX-FileCopyrightText: 2026 Latte Dock Contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */
import QtQuick
import org.kde.pipewire as PipeWire
import org.kde.taskmanager as TaskManager
PipeWire.PipeWireSourceItem {
    id: source
    // Invisible sources pause capture, so never bind visibility to ready.
    nodeId: request.nodeId
    TaskManager.ScreencastingRequest {
        id: request
        uuid: source.parent.uuid
    }
}
