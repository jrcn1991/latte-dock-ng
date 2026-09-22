/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import org.kde.plasma.core as PlasmaCore

Item {
    id: shadowsContainer
    opacity: 0.4

    readonly property int gradientLength: appletAbilities.metrics.iconSize / 3
    readonly property int thickness: appletAbilities.metrics.backgroundThickness
    readonly property color appliedColor: appletAbilities.myView.itemShadow.shadowSolidColor

    property Item flickable

    Rectangle {
        id: firstGradient
        width: !root.vertical ? gradientLength : shadowsContainer.thickness
        height: !root.vertical ? shadowsContainer.thickness : gradientLength

        gradient: Gradient {
            orientation: !root.vertical ? Gradient.Horizontal : Gradient.Vertical
            GradientStop { position: 0.0; color: (scrollableList.currentPos > scrollableList.scrollFirstPos ? appliedColor : "transparent") }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    Rectangle {
        id: lastGradient
        width: firstGradient.width
        height: firstGradient.height

        gradient: Gradient {
            orientation: !root.vertical ? Gradient.Horizontal : Gradient.Vertical
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: (scrollableList.currentPos < scrollableList.scrollLastPos ? appliedColor : "transparent") }
        }
    }

    states: [
        State {
            name: "bottom"
            when: root.location === PlasmaCore.Types.BottomEdge

            AnchorChanges {
                target: firstGradient
                anchors{ top:undefined; bottom:parent.bottom; left:parent.left; right:undefined;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
            AnchorChanges {
                target: lastGradient
                anchors{ top:undefined; bottom:parent.bottom; left:undefined; right:parent.right;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
            AnchorChanges {
                target: shadowsContainer
                anchors{ top:undefined; bottom:flickable.bottom; left:undefined; right:undefined;
                    horizontalCenter:flickable.horizontalCenter; verticalCenter:undefined}
            }
            PropertyChanges {
                shadowsContainer.anchors.leftMargin: 0;    shadowsContainer.anchors.rightMargin: 0;     shadowsContainer.anchors.topMargin: 0;    shadowsContainer.anchors.bottomMargin: appletAbilities.metrics.margin.screenEdge;
                shadowsContainer.anchors.horizontalCenterOffset: 0; shadowsContainer.anchors.verticalCenterOffset: 0;
            }
        },
        State {
            name: "top"
            when: root.location === PlasmaCore.Types.TopEdge

            AnchorChanges {
                target: firstGradient
                anchors{ top:parent.top; bottom:undefined; left:parent.left; right:undefined;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
            AnchorChanges {
                target: lastGradient
                anchors{ top:parent.top; bottom:undefined; left:undefined; right:parent.right;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
            AnchorChanges {
                target: shadowsContainer
                anchors{ top:flickable.top; bottom:undefined; left:undefined; right:undefined;
                    horizontalCenter:flickable.horizontalCenter; verticalCenter:undefined}
            }
            PropertyChanges {
                shadowsContainer.anchors.leftMargin: 0;    shadowsContainer.anchors.rightMargin: 0;     shadowsContainer.anchors.topMargin: appletAbilities.metrics.margin.screenEdge;    shadowsContainer.anchors.bottomMargin: 0;
                shadowsContainer.anchors.horizontalCenterOffset: 0; shadowsContainer.anchors.verticalCenterOffset: 0;
            }
        },
        State {
            name: "left"
            when: root.location === PlasmaCore.Types.LeftEdge

            AnchorChanges {
                target: firstGradient
                anchors{ top:parent.top; bottom:undefined; left:parent.left; right:undefined;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
            AnchorChanges {
                target: lastGradient
                anchors{ top:undefined; bottom:parent.bottom; left:parent.left; right:undefined;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
            AnchorChanges {
                target: shadowsContainer
                anchors{ top:undefined; bottom:undefined; left:flickable.left; right:undefined;
                    horizontalCenter:undefined; verticalCenter:flickable.verticalCenter}
            }
            PropertyChanges {
                shadowsContainer.anchors.leftMargin: appletAbilities.metrics.margin.screenEdge;    shadowsContainer.anchors.rightMargin: 0;     shadowsContainer.anchors.topMargin: 0;    shadowsContainer.anchors.bottomMargin: 0;
                shadowsContainer.anchors.horizontalCenterOffset: 0; shadowsContainer.anchors.verticalCenterOffset: 0;
            }
        },
        State {
            name: "right"
            when: root.location === PlasmaCore.Types.RightEdge

            AnchorChanges {
                target: firstGradient
                anchors{ top:parent.top; bottom:undefined; left:undefined; right:parent.right;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
            AnchorChanges {
                target: lastGradient
                anchors{ top:undefined; bottom:parent.bottom; left:undefined; right:parent.right;
                    horizontalCenter:undefined; verticalCenter:undefined}
            }
            AnchorChanges {
                target: shadowsContainer
                anchors{ top:undefined; bottom:undefined; left:undefined; right:flickable.right;
                    horizontalCenter:undefined; verticalCenter:flickable.verticalCenter}
            }
            PropertyChanges {
                shadowsContainer.anchors.leftMargin: 0;    shadowsContainer.anchors.rightMargin: appletAbilities.metrics.margin.screenEdge;     shadowsContainer.anchors.topMargin: 0;    shadowsContainer.anchors.bottomMargin: 0;
                shadowsContainer.anchors.horizontalCenterOffset: 0; shadowsContainer.anchors.verticalCenterOffset: 0;
            }
        }
    ]
}
