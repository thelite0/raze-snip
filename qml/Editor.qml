pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import RazeSnip 1.0
Rectangle {
 id: root
 color: "#1b1b1b"; focus: true
 Connections { target: snip; function onEditorImage(image) { canvas.load(image) } }
 EditorCanvas {
  id: canvas; anchors.fill: parent; anchors.topMargin: 110
  MouseArea {
   anchors.fill: parent; cursorShape: Qt.CrossCursor; acceptedButtons: Qt.LeftButton
   onPressed: function(m) { canvas.down(m.x,m.y) }
   onPositionChanged: function(m) { canvas.move(m.x,m.y) }
   onReleased: function(m) { canvas.up(m.x,m.y) }
  }
 }
 ColumnLayout {
  anchors.top: parent.top; anchors.horizontalCenter: parent.horizontalCenter; anchors.topMargin: 12; spacing: 6
  RowLayout {
   Repeater {
    model: ["Arrow", "Rectangle", "Pen", "Text", "Pixelate", "Number", "Blur", "Pick color"]
    Button { required property int index; required property string modelData; text: modelData; checkable: true; checked: canvas.tool === index; onClicked: canvas.tool = index }
   }
  }
  RowLayout {
   Repeater { model: ["#ff453a", "#ffcc00", "#30d158", "#0a84ff", "#bf5af2", "#ffffff", "#000000"]
    Rectangle { required property string modelData; width: 24; height: 24; radius: 12; color: modelData; border.color: canvas.ink === modelData ? "#b58ac6" : "#555555"; border.width: 2; MouseArea { anchors.fill: parent; onClicked: canvas.ink = parent.modelData } }
   }
   Text { text: "Width"; color: "#dddddd" }
   SpinBox { from: 1; to: 30; value: 3; onValueModified: canvas.stroke = value }
   TextField { visible: canvas.tool === 3; text: "Text"; placeholderText: "Annotation"; onTextChanged: canvas.text = text; Layout.preferredWidth: 120 }
   Button { text: "Undo"; enabled: canvas.canUndo; onClicked: canvas.undo() }
   Button { text: "Redo"; enabled: canvas.canRedo; onClicked: canvas.redo() }
   Button { text: "Copy"; onClicked: snip.editorDone(canvas.image(), false) }
   Button { text: "Save + copy"; onClicked: snip.editorDone(canvas.image(), true) }
  }
 }
 Shortcut { sequence: "Ctrl+Z"; onActivated: canvas.undo() }
 Shortcut { sequence: "Ctrl+Shift+Z"; onActivated: canvas.redo() }
 Shortcut { sequence: "Ctrl+C"; onActivated: snip.editorDone(canvas.image(), false) }
 Shortcut { sequence: "Ctrl+S"; onActivated: snip.editorDone(canvas.image(), true) }
 Shortcut { sequence: "Escape"; onActivated: root.Window.window.hide() }
}
