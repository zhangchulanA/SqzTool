import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

Window {
    width: 400
    height: 300
    visible: true
    title: "登录窗口"

    Column {
        anchors.centerIn: parent
        spacing: 10

        TextField {
            id: usernameInput
            placeholderText: "用户名"
            width: 200
        }

        TextField {
            id: passwordInput
            placeholderText: "密码"
            echoMode: TextInput.Password
            width: 200
        }

        Button {
            text: "登录"
            width: 200
            onClicked: {
                console.log("用户名:", usernameInput.text)
                console.log("密码:", passwordInput.text)
            }
        }
    }
}
