import QtQuick
import QtQuick.Controls
Item {
 id: root
 property url icon
 property bool selected: false
 property string label
 property color selectedColor: "#b58ac6"
 signal clicked()
 width: 32; height: 32
 activeFocusOnTab: true
 Accessible.role: Accessible.Button
 Accessible.name: label
 Accessible.onPressAction: clicked()
 Rectangle {
  anchors.fill: parent; radius: 4
  color: root.selected ? (mouse.pressed ? "#a37cb3" : root.selectedColor) : mouse.pressed ? "#3c403d" : mouse.containsMouse ? "#2b302c" : "transparent"
  border.width: root.activeFocus ? 1 : 0
  border.color: "#d6d8d7"
 }
 ModeIcon {
  anchors.centerIn: parent
  name: root.icon.toString().split("/").pop().replace(".svg", "")
  foreground: root.selected ? "#261c2b" : "#d6d8d7"
 }
 MouseArea {
  id: mouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.ArrowCursor
  onClicked: root.clicked()
 }
 ToolTip.visible: mouse.containsMouse
 ToolTip.delay: 650
 ToolTip.text: label
 Keys.onReturnPressed: clicked()
 Keys.onSpacePressed: clicked()
}
