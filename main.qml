import QtQuick 2.6
import QtQuick.Controls 1.5
import VideoRendering 1.0

Item {
    width: 400;
    height: 400;
    activeFocusOnTab: true;

    ShaderEffect {
        id: tileBackground;
        anchors.fill: parent;

        property real tileSize: 16;
        property color color1: Qt.rgba(0.9, 0.9, 0.9, 1);
        property color color2: Qt.rgba(0.85, 0.85, 0.85, 1);

        property size pixelSize: Qt.size(width / tileSize, height / tileSize);

        fragmentShader:
            "
                uniform lowp vec4 color1;
                uniform lowp vec4 color2;
                uniform highp vec2 pixelSize;
                varying highp vec2 qt_TexCoord0;
                void main() {
                    highp vec2 tc = sign(sin(3.14152 * qt_TexCoord0 * pixelSize));
                    if (tc.x != tc.y)
                        gl_FragColor = color1;
                    else
                        gl_FragColor = color2;
                }
                "
    }

    VideoManager {
        id: renderer;
        anchors.fill: parent;
        anchors.margins: 100;
        videoSource: VideoManager.PAL;
        runningState: VideoManager.Running;
    }

    Text {
        id: texthello;
        anchors.fill: parent;
        text: qsTr("Hello");
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 50;
        font.bold: true;
        color: "white";
        z: 2;
    }

    Keys.enabled: true;
    Keys.onUpPressed: {
        renderer.runningState += 1;
        console.log("up, renderer.runningState:", renderer.runningState);
    }
    Keys.onDownPressed: {
        renderer.runningState -= 1;
        console.log("down, renderer.runningState:", renderer.runningState);
    }
    Keys.onLeftPressed: {
        renderer.videoSource -= 1;
        console.log("left, renderer.videoSource:", renderer.videoSource);
    }
    Keys.onRightPressed: {
        renderer.videoSource += 1;
        console.log("right, renderer.videoSource:", renderer.videoSource);
    }
}
