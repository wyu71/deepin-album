// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4

import org.deepin.album 1.0 as Album

Item {
    id: switchViewAnimation

    property int viewType: -1

    property bool show: false
    property real showOpacity: 1
    property real showX: 0

    property string idName: ""
    property real hideOpacity: 0
    property real hideX: -width - 20

    property int switchType: GStatus.currentSwitchType
    property string switchPropertys: "x,opacity"

    onSwitchTypeChanged: {
        if (switchType === Album.Types.FlipScroll) {
            x = width + 20
            switchPropertys = "x,opacity"
        } else if (switchType === Album.Types.FadeInOut) {
            x = 0
            switchPropertys = "opacity"
        }
    }

    onOpacityChanged: {
        // 当前视图渐隐后，应将视图移到视图区域外，否则当前视图的鼠标事件依然生效
        if (opacity === 0 && switchType === Album.Types.FadeInOut) {
            if (!show) {
                if (x === 0)
                    x = -width - 20
            }
        }
    }

    state: "hide"
    states: [
        State {
            name: "show"
            PropertyChanges {
                target: switchViewAnimation
                opacity: showOpacity
                x: showX
            }
            when: show
        },
        State {
            name: "hide"
            PropertyChanges {
                target: switchViewAnimation
                opacity: hideOpacity
                x: hideX
            }
            when: !show
        }
    ]

    transitions:
        Transition {
        enabled: switchType !== Album.Types.HardCut
        NumberAnimation{properties: switchPropertys; easing.type: Easing.OutExpo; duration: GStatus.animationDuration
        }
    }
}
