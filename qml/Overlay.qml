pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import RazeSnip 1.0
Item {
 id: root
 required property int screenX
 required property int screenY
 required property bool toolbarVisible
 focus: true
 property real dimLevel: 0.6
 function begin() { forceActiveFocus() }

 FreezeCanvas { anchors.fill: parent; controller: snip; screenX: root.screenX; screenY: root.screenY; dim: root.dimLevel }
 MouseArea {
  anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.LeftButton | Qt.RightButton
  cursorShape: snip.mode === 2 ? Qt.ArrowCursor : Qt.BlankCursor
  onPressed: function(m) { if (!snip.testing) snip.down(root.screenX + m.x, root.screenY + m.y, m.button, m.modifiers) }
  onPositionChanged: function(m) { if (!snip.testing) snip.move(root.screenX + m.x, root.screenY + m.y) }
  onReleased: function(m) { if (!snip.testing) snip.up(root.screenX + m.x, root.screenY + m.y, m.button, m.modifiers) }
  onCanceled: snip.cancel()
 }
 Item {
  x: snip.cursorPosition.x - root.screenX - 10
  y: snip.cursorPosition.y - root.screenY - 10
  width: 21; height: 21
  visible: snip.mode !== 2 && !(root.toolbarVisible && x + 10 >= toolbar.x && x + 10 <= toolbar.x + toolbar.width && y + 10 >= toolbar.y && y + 10 <= toolbar.y + toolbar.height)
  Rectangle { x: 9; y: 0; width: 3; height: 9; color: "#dd000000"; Rectangle { x: 1; y: 1; width: 1; height: 7; color: "white" } }
  Rectangle { x: 9; y: 12; width: 3; height: 9; color: "#dd000000"; Rectangle { x: 1; y: 1; width: 1; height: 7; color: "white" } }
  Rectangle { x: 0; y: 9; width: 9; height: 3; color: "#dd000000"; Rectangle { x: 1; y: 1; width: 7; height: 1; color: "white" } }
  Rectangle { x: 12; y: 9; width: 9; height: 3; color: "#dd000000"; Rectangle { x: 1; y: 1; width: 7; height: 1; color: "white" } }
  Rectangle { x: 10; y: 10; width: 1; height: 1; color: "white" }
 }
 Rectangle {
  id: toolbar
  visible: root.toolbarVisible
  width: 228; height: 48; x: Math.round((parent.width-width)/2); y: 12
  radius: 8; color: "#f2111613"; border.width: 1; border.color: "#1affffff"
  MouseArea { anchors.fill: parent; acceptedButtons: Qt.LeftButton | Qt.RightButton; cursorShape: Qt.ArrowCursor; onClicked: function(m) { if (m.button === Qt.RightButton) snip.cancel() } }
  Repeater {
   model: [ { icon: "rectangle", label: "Rectangular snip" }, { icon: "freeform", label: "Freeform snip" }, { icon: "window", label: "Window snip" }, { icon: "fullscreen", label: "Fullscreen snip" } ]
   ModeButton { required property var modelData; required property int index; x: 12 + index * 42; y: 8; icon: "qrc:/data/icons/" + modelData.icon + ".svg"; label: modelData.label; selected: snip.mode === index; onClicked: snip.mode = index }
  }
  Rectangle { x: 180; y: 12; width: 1; height: 24; color: "#383d39" }
  ModeButton { x: 188; y: 8; icon: "qrc:/data/icons/close.svg"; label: "Close (Escape)"; onClicked: snip.cancel() }
 }
 Keys.onPressed: function(event) {
  if (event.key === Qt.Key_Escape || event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Left || event.key === Qt.Key_Right || event.key === Qt.Key_Up || event.key === Qt.Key_Down) { snip.key(event.key, event.modifiers); event.accepted = true }
 }
}
