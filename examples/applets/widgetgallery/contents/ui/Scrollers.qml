/*
*   Copyright (C) 2011 by Daker Fernandes Pinheiro <dakerfp@gmail.com>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU Library General Public License as
*   published by the Free Software Foundation; either version 2, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details
*
*   You should have received a copy of the GNU Library General Public
*   License along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

import QtQuick 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

PlasmaComponents.Page {
    height: childrenRect.height
    property int implicitHeight: childrenRect.height

    tools: PlasmaComponents.ToolBarLayout {
        spacing: 5
        PlasmaComponents.ToolButton {
            visible: pageStack.depth > 1
            iconSource: "go-previous"
            onClicked: pageStack.pop()
        }
        PlasmaComponents.ScrollBar {
            orientation: Qt.Horizontal
            interactive: true
            flickableItem: scrollArea
            width: 200
        }
        PlasmaComponents.TextField {
            clearButtonShown: true
            text: "hello"
        }
    }

    PlasmaExtras.ScrollArea {
        anchors.fill: parent
        Flickable {
            id: flickable
            contentWidth: column.width
            contentHeight: column.height
            clip: true
            anchors.fill: parent

            Item {
                width: Math.max(flickable.width, column.width)
                height: column.height
                Column {
                    id: column
                    spacing: 20
                    anchors.horizontalCenter: parent.horizontalCenter

                    PlasmaComponents.Label {
                        font.pixelSize: 20
                        text: "Scroll Bar"
                    }

                    PlasmaExtras.ScrollArea {
                        width: 200
                        height: 200
                        ListView {
                            id: scrollList

                            width: 200
                            height: 200
                            clip: true
                            model: 20
                            delegate: PlasmaComponents.Label {
                                width: 200
                                height: 30
                                text: index
                                font.pixelSize: 18
                            }

                            Rectangle {
                                anchors.fill: parent
                                color: "grey"
                                opacity: 0.3
                            }
                        }
                    }

                    PlasmaComponents.Label {
                        font.pixelSize: 20
                        text: "Scroll Decorator"
                    }

                    Item {
                        width: 200
                        height: 200
                        PlasmaComponents.Highlight { anchors.fill: parent }
                        Flickable {
                            id: scrollArea
                            anchors.fill: parent
                            clip: true
                            contentWidth: 400
                            contentHeight: 400

                            // Flickable Contents
                            Rectangle {
                                color: "green"
                                width: 100
                                height: 100
                            }
                            Rectangle {
                                x: 80
                                y: 80
                                color: "blue"
                                width: 200
                                height: 200
                            }
                            Rectangle {
                                x: 200
                                y: 200
                                color: "red"
                                width: 150
                                height: 150
                            }
                        }

                        // Scroll Decorators
                        PlasmaComponents.ScrollBar {
                            orientation: Qt.Vertical
                            flickableItem: scrollArea
                            inverted: true
                            anchors {
                                top: scrollArea.top
                                right: scrollArea.right
                                bottom: scrollArea.bottom
                            }
                            PlasmaComponents.Label {
                                y: parent.height / 2
                                x: 13
                                rotation: -90
                                text: "inverted"
                            }
                        }
                        PlasmaComponents.ScrollBar {
                            orientation: Qt.Horizontal
                            flickableItem: scrollArea
                            anchors {
                                left: scrollArea.left
                                right: scrollArea.right
                                bottom: scrollArea.bottom
                            }
                        }
                    }
                }
            }
        }
    }
}
