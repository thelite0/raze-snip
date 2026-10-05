import QtQuick
import QtQuick.Shapes
Shape {
 id: root
 required property string name
 property color foreground: "#d6d8d7"
 width: 20; height: 20
 preferredRendererType: Shape.CurveRenderer
 property var paths: ({
  "rectangle": "M3.5 13.5 V4.5 Q3.5 3.5 4.5 3.5 H15.5 Q16.5 3.5 16.5 4.5 V11.5 M3.5 13.5 H12.5 M16.5 12.5 V18.5 M13.5 15.5 H19.5",
  "freeform": "M13.5 4.5 C12.5 1.5 7.5 2.5 5.5 5.5 C3.5 8.5 2.5 12.5 4.5 14.5 C6.5 16.5 9.5 16.5 12.5 15.5 M13.5 4.5 C16.5 4.5 17.5 7.5 16.5 10.5 M16.5 12.5 V18.5 M13.5 15.5 H19.5",
  "window": "M3.5 14.5 V4.5 Q3.5 3.5 4.5 3.5 H15.5 Q16.5 3.5 16.5 4.5 V11.5 M3.5 7.5 H16.5 M3.5 14.5 H12.5 M16.5 12.5 V18.5 M13.5 15.5 H19.5",
  "fullscreen": "M3.5 5.5 H16.5 V15.5 H3.5 Z M5.5 9.5 V7.5 H7.5 M12.5 7.5 H14.5 V9.5 M5.5 11.5 V13.5 H7.5 M12.5 13.5 H14.5 V11.5",
  "close": "M5.5 5.5 L14.5 14.5 M14.5 5.5 L5.5 14.5"
 })
 ShapePath {
  strokeColor: root.foreground
  strokeWidth: 1
  fillColor: "transparent"
  capStyle: ShapePath.RoundCap
  joinStyle: ShapePath.RoundJoin
  PathSvg { path: root.paths[root.name] || "" }
 }
}
