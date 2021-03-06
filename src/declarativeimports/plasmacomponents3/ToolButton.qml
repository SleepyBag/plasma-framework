/*
 *   Copyright 2016 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.6
import QtQuick.Layouts 1.2
import QtQuick.Templates @QQC2_VERSION@ as T
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kirigami 2.5 as Kirigami
import "private" as Private


T.ToolButton {
    id: control

    implicitWidth: Math.max(background.implicitWidth,
                            contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(background.implicitHeight, contentItem.implicitHeight + topPadding + bottomPadding)

    leftPadding: surfaceNormal.margins.left
    topPadding: surfaceNormal.margins.top
    rightPadding: surfaceNormal.margins.right
    bottomPadding: surfaceNormal.margins.bottom

    hoverEnabled: !Kirigami.Settings.tabletMode

    Kirigami.MnemonicData.enabled: control.enabled && control.visible
    Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.SecondaryControl
    Kirigami.MnemonicData.label: control.text

    flat: true

    PlasmaCore.ColorScope.inherit: flat
    PlasmaCore.ColorScope.colorGroup: flat ? parent.PlasmaCore.ColorScope.colorGroup : PlasmaCore.Theme.ButtonColorGroup
    
    contentItem: GridLayout {
        columns: control.display == T.AbstractButton.TextBesideIcon ? 2 : 1

        PlasmaCore.IconItem {
            id: icon

            Layout.alignment: Qt.AlignCenter

            Layout.fillWidth: control.display === T.AbstractButton.TextUnderIcon
            Layout.fillHeight: control.display !== T.AbstractButton.TextUnderIcon

            Layout.minimumWidth: units.iconSizes.tiny
            Layout.minimumHeight: units.iconSizes.tiny
            Layout.maximumWidth: control.icon.width > 0 ? control.icon.width : units.iconSizes.smallMedium
            Layout.maximumHeight: control.icon.height > 0 ? control.icon.height : units.iconSizes.smallMedium

            implicitWidth: Layout.maximumWidth
            implicitHeight: Layout.maximumHeight

            colorGroup: control.PlasmaCore.ColorScope.colorGroup
            visible: source.length > 0 && control.display !== T.AbstractButton.TextOnly
            source: control.icon ? (control.icon.name || control.icon.source) : ""
            status: !control.flat && control.activeFocus && !control.pressed && !control.checked ? PlasmaCore.Svg.Selected : PlasmaCore.Svg.Normal
        }
        
        Label {
            id: label
            Layout.fillWidth: true
            visible: text.length > 0 && control.display !== T.AbstractButton.IconOnly
            text: control.Kirigami.MnemonicData.richTextLabel
            font: control.font
            opacity: width > 0 ? (enabled || control.highlighted || control.checked ? 1 : 0.4) : 0
            color: (control.hovered || !control.flat) && buttonsurfaceChecker.usedPrefix != "toolbutton-hover" ? theme.buttonTextColor : PlasmaCore.ColorScope.textColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight

            PlasmaCore.FrameSvgItem {
                id: buttonsurfaceChecker
                visible: false
                imagePath: "widgets/button"
                prefix: control.flat ? ["toolbutton-hover", "normal"] : "normal"
            }
        }
    }

    background: Item {
        //retrocompatibility with old controls
        implicitWidth: Math.floor(units.gridUnit * 1.6) + Math.floor(units.gridUnit * 1.6) % 2
        implicitHeight: implicitWidth
        Private.ButtonShadow {
            anchors.fill: parent
            visible: (!control.flat || control.hovered) && (!control.pressed || !control.checked)
            state: {
                if (control.pressed) {
                    return "hidden"
                } else if (control.hovered) {
                    return "hover"
                } else if (control.activeFocus) {
                    return "focus"
                } else {
                    return "shadow"
                }
            }
        }
        PlasmaCore.FrameSvgItem {
            id: surfaceNormal
            anchors.fill: parent
            imagePath: "widgets/button"
            prefix: "normal"
            opacity: !control.flat && (!control.pressed || !control.checked) ? 1 : 0
            Behavior on opacity {
                OpacityAnimator {
                    duration: units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }
        }
        PlasmaCore.FrameSvgItem {
            anchors.fill: parent
            imagePath: "widgets/button"
            prefix: "pressed"
            opacity: control.checked || control.pressed ? 1 : 0
            Behavior on opacity {
                OpacityAnimator {
                    duration: units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }
}
