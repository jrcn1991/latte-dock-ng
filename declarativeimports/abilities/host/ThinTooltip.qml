/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Window

import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.core as PlasmaCore

import org.kde.latte.core as LatteCore
import org.kde.latte.abilities.definition as AbilityDefinition

AbilityDefinition.ThinTooltip {
    id: _thinTooltip
    property Item debug: null

    readonly property int maxCharacters: 80

    property Item lastHidingVisualParent: null

    onIsEnabledChanged: {
        if (!isEnabled) {
            clearTooltip();
        }
    }

    //! Public API
    readonly property Item publicApi: Item {
        readonly property alias isEnabled: _thinTooltip.isEnabled
        readonly property alias currentText: _thinTooltip.currentText
        readonly property alias currentVisualParent: _thinTooltip.currentVisualParent

        function show(visualParent, text) {
            _thinTooltip.show(visualParent, text);
        }

        function hide(visualParent) {
            _thinTooltip.hide(visualParent);
        }
    }

    //! Private Implementation

    onShowIsBlockedChanged: {       
        if (isEnabled && !showIsBlocked && !_tooltipDialog.visible && _thinTooltip.currentVisualParent) {
            _hideTimer.stop();
            _tooltipDialog.visible = true;
        } else if (showIsBlocked && _tooltipDialog.visible) {
            _tooltipDialog.visible = false;
        }
    }

    function show(visualParent, text) {
        const displayText = text === undefined || text === null ? "" : String(text);

        //! A show() request carries the hovered visual parent even when the
        //! text is not resolved yet. Clearing the whole tooltip here would
        //! detach it from the pointer, so empty text is simply ignored.
        if (!isEnabled || !visualParent || displayText.length === 0) {
            return;
        }

        if (showIsBlocked) {
            _hideTimer.stop();
            _showTimer.stop();
            _tooltipDialog.visible = false;
        }

        const anchorChanged = _thinTooltip.currentVisualParent
                              && _thinTooltip.currentVisualParent !== visualParent;

        _hideTimer.stop();
        if (anchorChanged && _tooltipDialog.visible) {
            // A mapped Wayland tooltip cannot be moved without remapping it.
            // Hide it now and let the existing show timer remap it after the
            // parabolic hand-off settles, avoiding geometry work per frame.
            _tooltipDialog.visible = false;
        }
        _thinTooltip.currentVisualParent = visualParent;
        _tooltipDialog.visualParent = visualParent;

        var fixedDisplayText = displayText.length > maxCharacters ? displayText.substring(0, maxCharacters - 1) + "..." : displayText;
        _thinTooltip.currentText = fixedDisplayText;

        if (!_tooltipDialog.visible && !showIsBlocked) {
            _showTimer.restart();
        }
    }

    function clearTooltip() {
        _hideTimer.stop();
        _showTimer.stop();
        _tooltipDialog.visible = false;
        _thinTooltip.lastHidingVisualParent = null;
        _thinTooltip.currentVisualParent = null;
        _thinTooltip.currentText = "";
    }

    function hide(visualParent) {
        if (_thinTooltip.currentVisualParent === visualParent) {
            _thinTooltip.lastHidingVisualParent = visualParent;
            _showTimer.stop();
            _hideTimer.start();
        }
    }

    //! Show Delayer Timer
    Timer {
        id: _showTimer
        interval: 60
        onTriggered: {
            if (_thinTooltip.currentVisualParent) {
                _tooltipDialog.visible = true;
            }

            if (debug && debug.timersEnabled) {
                console.log("ThinTooltip host timer: show() called...");
            }
        }
    }

    //! Hide Delayer Timer
    Timer {
        id: _hideTimer
        interval: 100
        onTriggered: {
            if (_thinTooltip.lastHidingVisualParent === _thinTooltip.currentVisualParent) {
                _tooltipDialog.visible = false;
                _thinTooltip.lastHidingVisualParent = null;
                _thinTooltip.currentVisualParent = null;
                _thinTooltip.currentText = "";
            }

            if (debug && debug.timersEnabled) {
                console.log("ThinTooltip host timer: hide() called...");
            }
        }
    }

    LatteCore.Dialog{
        id: _tooltipDialog
        type: PlasmaCore.Dialog.Tooltip
        flags: Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus | Qt.ToolTip

        location: plasmoid.location
        edge: plasmoid.location
        mainItem: RowLayout{
            Layout.fillWidth: true
            Layout.fillHeight: true
            PlasmaComponents.Label{
                Layout.leftMargin: 4
                Layout.rightMargin: 4
                Layout.topMargin: 2
                Layout.bottomMargin: 2
                text: _thinTooltip.currentText
            }
        }

        visible: false
    }
}
