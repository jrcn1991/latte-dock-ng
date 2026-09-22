/*
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

Timer {
    id: scheduler

    required property Item task
    required property Animation animation

    interval: 0
    repeat: false

    function schedule() {
        // ListView can emit remove while a delegate's size binding is updating.
        // Hold the delegate synchronously, then let its existing removal animation
        // own delayRemove and cleanup after that binding unwinds. Starting it here
        // would write parabolic zoom back into the active size binding. The owned
        // timer is cancelled automatically if the delegate/view is destroyed.
        task.ListView.delayRemove = true;
        restart();
    }

    onTriggered: {
        animation.stop();
        animation.start();
    }
}
