import QtQuick 1.1
import "."

Rectangle {
    id: rootRect

    width: parent.width
    property real trayItemHeight: parent.width
    height: trayView.count * trayItemHeight

    color: "transparent"

    ListView {
        id: trayView
        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds

        model: SB2_trayModel

        delegate: ActionButton {
            height: rootRect.trayItemHeight
            width: rootRect.width

            isHighlight: actionObject.checked
            actionIconURL: actionIcon

            onTriggered: actionObject.trigger()
        }
    }
}
