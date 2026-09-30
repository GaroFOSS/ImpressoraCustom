import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    visible: true
    width: 560
    height: 480
    minimumWidth: 400
    minimumHeight: 360
    title: "Elgin i8 — Text Printer"

    ButtonGroup { id: sizeGroup }
    ButtonGroup { id: alignmentGroup }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: textInput
                objectName: "textInput"
                placeholderText: "Enter text to print…"
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                Accessible.name: "Text to print"
            }
        }

        RowLayout {
            Label { text: "Size"; Layout.preferredWidth: 76 }
            RadioButton { id: small; text: "Pequeno"; checked: true; ButtonGroup.group: sizeGroup }
            RadioButton { id: medium; text: "Medio"; ButtonGroup.group: sizeGroup }
            RadioButton { text: "Grande"; ButtonGroup.group: sizeGroup }
        }

        RowLayout {
            Label { text: "Alignment"; Layout.preferredWidth: 76 }
            RadioButton { id: left; text: "Esquerda"; checked: true; ButtonGroup.group: alignmentGroup }
            RadioButton { id: center; text: "Centro"; ButtonGroup.group: alignmentGroup }
            RadioButton { text: "Direita"; ButtonGroup.group: alignmentGroup }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: printerController.status
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                Accessible.name: text
            }
            Button {
                objectName: "printButton"
                text: printerController.busy ? "Sending…" : "Print"
                enabled: !printerController.busy && textInput.text.trim().length > 0
                onClicked: printerController.printText(textInput.text,
                    small.checked ? 0 : medium.checked ? 1 : 2,
                    left.checked ? 0 : center.checked ? 1 : 2)
            }
        }
    }
}
