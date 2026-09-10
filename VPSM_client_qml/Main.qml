pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window

Window {
    id: root

    width: 980
    height: 720
    minimumWidth: 760
    minimumHeight: 600
    visible: true
    title: "VPSM Client"
    color: "white"

    property var viewModel: null
    readonly property var tunnel: viewModel ? viewModel.tunnel : null
    readonly property bool connected: tunnel ? tunnel.connected : false
    readonly property bool working: viewModel ? (viewModel.busy || (tunnel && tunnel.busy)) : false

    property bool serverDialogVisible: false
    property bool userDialogVisible: false
    property bool networkDialogVisible: false
    property bool createNetworkMode: true
    property bool confirmationVisible: false
    property string confirmationText: ""
    property string confirmationAction: ""
    property string confirmationPeerId: ""
    property bool errorDismissed: false

    property string serverAddress: "127.0.0.1"
    property string controlPort: "8080"
    property string routerPort: "4000"
    property string localPort: "4001"
    property string interfaceName: "vpsm0"
    property string nickname: ""
    property string password: ""
    property int selectedNetworkId: 0
    readonly property var selectedNetwork: {
        if (!viewModel)
            return null
        for (let i = 0; i < viewModel.networks.length; ++i) {
            if (Number(viewModel.networks[i].id) === selectedNetworkId)
                return viewModel.networks[i]
        }
        return null
    }

    function confirmAction() {
        confirmationVisible = false
        if (!viewModel || selectedNetworkId === 0)
            return
        if (confirmationAction === "delete")
            viewModel.deleteNetwork(selectedNetworkId)
        else if (confirmationAction === "leave")
            viewModel.leaveNetwork(selectedNetworkId)
        else if (confirmationAction === "removePeer")
            viewModel.removePeer(selectedNetworkId, confirmationPeerId)
    }

    function serverUrl() {
        return "http://" + serverAddress.trim() + ":" + controlPort
    }

    function connectToNetwork() {
        errorDismissed = false
        if (!viewModel)
            return

        if (connected) {
            viewModel.disconnectTunnel()
            return
        }

        if (!viewModel.serverConnected) {
            serverDialogVisible = true
            return
        }
        if (!viewModel.authenticated) {
            userDialogVisible = true
            return
        }
        if (selectedNetworkId === 0)
            return
        viewModel.connectAvailableNetwork(selectedNetworkId,serverAddress,Number(routerPort),Number(localPort),interfaceName)
    }

    component PlainButton: Rectangle {
        id: button

        property string text: ""
        signal clicked()

        implicitWidth: 140
        implicitHeight: 40
        color: enabled ? "#eeeeee" : "#f7f7f7"
        border.width: 1
        border.color: "#777777"
        opacity: enabled ? 1 : 0.5

        Text {
            anchors.centerIn: parent
            text: button.text
            color: "#111111"
            font.pixelSize: 13
        }

        MouseArea {
            anchors.fill: parent
            enabled: button.enabled
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: button.clicked()
        }
    }

    component PlainField: Rectangle {
        id: field

        property alias text: editor.text
        property string placeholderText: ""
        property bool passwordMode: false
        property int inputMethodHints: Qt.ImhNone
        function forceInputFocus() { editor.forceActiveFocus() }

        implicitHeight: 40
        color: "white"
        border.width: 1
        border.color: editor.activeFocus ? "#333333" : "#999999"

        TextInput {
            id: editor
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            verticalAlignment: TextInput.AlignVCenter
            color: "#111111"
            selectionColor: "#cccccc"
            selectedTextColor: "#111111"
            font.pixelSize: 13
            clip: true
            echoMode: field.passwordMode ? TextInput.Password : TextInput.Normal
            inputMethodHints: field.inputMethodHints
        }

        Text {
            anchors.fill: parent
            anchors.leftMargin: 8
            verticalAlignment: Text.AlignVCenter
            visible: editor.text.length === 0 && !editor.activeFocus
            text: field.placeholderText
            color: "#777777"
            font.pixelSize: 13
        }
    }

    component FieldLabel: Text {
        color: "#333333"
        font.pixelSize: 12
    }

    component ValueBlock: Rectangle {
        property string label: ""
        property string value: "0"

        implicitHeight: 72
        color: "white"
        border.width: 1
        border.color: "#bbbbbb"

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
            text: parent.label
            color: "#555555"
            font.pixelSize: 11
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10
            text: parent.value
            color: "#111111"
            font.pixelSize: 18
        }
    }

    Connections {
        target: root.viewModel
        enabled: root.viewModel !== null

        function onAuthenticatedChanged() {
            if (root.viewModel.authenticated) {
                root.userDialogVisible = false
                root.viewModel.refreshNetworkPeers()
            } else {
                root.selectedNetworkId = 0
            }
        }

        function onServerConnectedChanged() {
            if (root.viewModel.serverConnected)
                root.serverDialogVisible = false
            else
                root.selectedNetworkId = 0
        }

        function onErrorTextChanged() {
            root.errorDismissed = false
        }
    }

    Rectangle {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 64
        color: "#f3f3f3"
        border.width: 1
        border.color: "#bbbbbb"

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: "VPSM Client"
            color: "#111111"
            font.pixelSize: 20
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 160
            anchors.verticalCenter: parent.verticalCenter
            text: root.viewModel ? root.viewModel.statusText : "UI preview"
            color: "#333333"
            font.pixelSize: 13
        }

        PlainButton {
            id: connectButton
            anchors.right: serverButton.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: root.connected ? "Отключить" : root.working ? "Подключение..." : "Подключить"
            enabled: root.viewModel !== null && !root.working
                    && (root.connected || (root.viewModel.serverConnected
                    && root.viewModel.authenticated && root.selectedNetworkId !== 0))
            onClicked: root.connectToNetwork()
        }

        PlainButton {
            id: serverButton
            anchors.right: userButton.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 110
            text: "Сервер"
            onClicked: root.serverDialogVisible = true
        }

        PlainButton {
            id: userButton
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            width: 120
            text: root.viewModel && root.viewModel.authenticated ? "Профиль" : "Войти"
            onClicked: root.userDialogVisible = true
        }
    }

    Rectangle {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: footer.top
        anchors.margins: 16
        color: "white"

        Rectangle {
            id: networkBlock
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 300
            color: "#fafafa"
            border.width: 1
            border.color: "#aaaaaa"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.top: parent.top
                anchors.topMargin: 14
                text: "Доступные сети"
                color: "#111111"
                font.pixelSize: 16
            }

            Row {
                id: networkActions
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 46
                spacing: 6

                PlainButton {
                    width: (networkActions.width - 12) / 3
                    height: 34
                    text: "Создать"
                    enabled: root.viewModel !== null && root.viewModel.authenticated && !root.working
                    onClicked: {
                        root.createNetworkMode = true
                        root.networkDialogVisible = true
                    }
                }
                PlainButton {
                    width: (networkActions.width - 12) / 3
                    height: 34
                    text: "Вступить"
                    enabled: root.viewModel !== null && root.viewModel.authenticated && !root.working
                    onClicked: {
                        root.createNetworkMode = false
                        root.networkDialogVisible = true
                    }
                }
                PlainButton {
                    width: (networkActions.width - 12) / 3
                    height: 34
                    text: "Обновить"
                    enabled: root.viewModel !== null && root.viewModel.authenticated && !root.working
                    onClicked: root.viewModel.refreshNetworkPeers()
                }
            }

            Rectangle {
                id: networksListBlock
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: activeNetworkBlock.top
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 90
                anchors.bottomMargin: 14
                color: "white"
                border.width: 1
                border.color: "#bbbbbb"

                ListView {
                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true
                    model: root.viewModel ? root.viewModel.networks : []

                    delegate: Rectangle {
                        id: networkDelegate
                        required property var modelData
                        width: ListView.view.width
                        height: root.selectedNetworkId === Number(modelData.id)
                                ? 58 + modelData.peers.length * 34 : 58
                        color: root.selectedNetworkId === Number(modelData.id) ? "#dddddd" : "white"
                        border.width: 1
                        border.color: "#cccccc"

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.selectedNetworkId = Number(networkDelegate.modelData.id)
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.top: parent.top
                            anchors.topMargin: 8
                            text: networkDelegate.modelData.name + " (" + networkDelegate.modelData.id + ")"
                            color: "#111111"
                            font.pixelSize: 13
                        }
                        Column {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            anchors.topMargin: 58
                            visible: root.selectedNetworkId === Number(networkDelegate.modelData.id)

                            Repeater {
                                model: networkDelegate.modelData.peers
                                delegate: Rectangle {
                                    id: peerDelegate
                                    required property var modelData
                                    width: parent.width
                                    height: 34
                                    color: "#f4f4f4"
                                    Text {
                                        anchors.left: parent.left
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: peerDelegate.modelData.nickname + " — " + peerDelegate.modelData.address
                                        color: "#333333"
                                        font.pixelSize: 11
                                    }
                                    PlainButton {
                                        anchors.right: parent.right
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 28
                                        height: 26
                                        text: "×"
                                        visible: networkDelegate.modelData.isOwner
                                                 && peerDelegate.modelData.peerId !== root.viewModel.peerId
                                        onClicked: {
                                            root.confirmationAction = "removePeer"
                                            root.confirmationPeerId = peerDelegate.modelData.peerId
                                            root.confirmationText = "Удалить участника " + peerDelegate.modelData.nickname + " из сети?"
                                            root.confirmationVisible = true
                                        }
                                    }
                                }
                            }
                        }
                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.top: parent.top
                            anchors.topMargin: 32
                            text: "Ваш VIP: " + networkDelegate.modelData.address
                                  + "   Сеть: " + networkDelegate.modelData.networkAddress
                            color: "#555555"
                            font.pixelSize: 11
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: !root.viewModel || root.viewModel.networks.length === 0
                    text: root.viewModel && root.viewModel.authenticated
                          ? "Сети не найдены" : "Сначала войдите на сервер"
                    color: "#666666"
                    font.pixelSize: 12
                }
            }

            Rectangle {
                id: activeNetworkBlock
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 14
                height: 132
                color: "white"
                border.width: 1
                border.color: "#bbbbbb"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.top: parent.top
                    anchors.topMargin: 12
                    text: root.connected ? "TUN подключён" : root.selectedNetwork !== null
                          ? "Сеть выбрана — нажмите «Подключить» сверху"
                          : "Выберите сеть для подключения TUN"
                    color: "#111111"
                    font.pixelSize: 13
                }

                PlainButton {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10
                    height: 34
                    visible: root.selectedNetwork !== null
                    text: root.selectedNetwork && root.selectedNetwork.isOwner
                          ? "Удалить сеть" : "Покинуть сеть"
                    enabled: !root.working
                    onClicked: {
                        root.confirmationAction = root.selectedNetwork.isOwner ? "delete" : "leave"
                        root.confirmationText = root.selectedNetwork.isOwner
                            ? "Удалить сеть и все её memberships?"
                            : "Покинуть выбранную сеть?"
                        root.confirmationVisible = true
                    }
                }

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 12
                    text: root.connected && root.tunnel
                          ? root.tunnel.localAddress + " / " + root.tunnel.interfaceName
                          : "VIP и интерфейс не назначены"
                    color: "#555555"
                    font.pixelSize: 12
                }
            }
        }

        Rectangle {
            id: tunnelBlock
            anchors.left: networkBlock.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 16
            color: "#fafafa"
            border.width: 1
            border.color: "#aaaaaa"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.top: parent.top
                anchors.topMargin: 14
                text: "Туннель"
                color: "#111111"
                font.pixelSize: 16
            }

            Rectangle {
                id: stateBlock
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 48
                height: 104
                color: "white"
                border.width: 1
                border.color: "#bbbbbb"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.top: parent.top
                    anchors.topMargin: 12
                    text: root.tunnel ? root.tunnel.stateText : "Отключен"
                    color: "#111111"
                    font.pixelSize: 18
                }

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 12
                    text: root.connected && root.tunnel
                          ? "Адрес: " + root.tunnel.localAddress
                            + "    Интерфейс: " + root.tunnel.interfaceName
                          : "TUN-интерфейс не активен"
                    color: "#555555"
                    font.pixelSize: 13
                }
            }

            Row {
                id: packetStatistics
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: stateBlock.bottom
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 16
                spacing: 10

                ValueBlock {
                    width: (packetStatistics.width - 20) / 3
                    label: "TX packets"
                    value: root.tunnel ? String(root.tunnel.txPackets) : "0"
                }
                ValueBlock {
                    width: (packetStatistics.width - 20) / 3
                    label: "RX packets"
                    value: root.tunnel ? String(root.tunnel.rxPackets) : "0"
                }
                ValueBlock {
                    width: (packetStatistics.width - 20) / 3
                    label: "Dropped"
                    value: root.tunnel ? String(root.tunnel.droppedPackets) : "0"
                }
            }

            Row {
                id: byteStatistics
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: packetStatistics.bottom
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 10
                spacing: 10

                ValueBlock {
                    width: (byteStatistics.width - 10) / 2
                    label: "TX bytes"
                    value: root.tunnel ? String(root.tunnel.txBytes) : "0"
                }
                ValueBlock {
                    width: (byteStatistics.width - 10) / 2
                    label: "RX bytes"
                    value: root.tunnel ? String(root.tunnel.rxBytes) : "0"
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 14
                height: 70
                visible: root.viewModel !== null
                         && root.viewModel.errorText.length > 0
                         && !root.errorDismissed
                color: "white"
                border.width: 1
                border.color: "#777777"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.right: dismissError.left
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.viewModel ? root.viewModel.errorText : ""
                    color: "#111111"
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                PlainButton {
                    id: dismissError
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: 36
                    height: 32
                    text: "X"
                    onClicked: root.errorDismissed = true
                }
            }
        }
    }

    Rectangle {
        id: footer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 44
        color: "#f3f3f3"
        border.width: 1
        border.color: "#bbbbbb"

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: root.viewModel
                  ? root.viewModel.statusText
                  : "ViewModel не подключена — режим предпросмотра"
            color: "#333333"
            font.pixelSize: 12
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.serverDialogVisible
        color: "#88000000"
        z: 10

        MouseArea {
            anchors.fill: parent
            onClicked: root.serverDialogVisible = false
        }

        Rectangle {
            anchors.centerIn: parent
            width: 460
            height: 480
            color: "white"
            border.width: 1
            border.color: "#777777"

            MouseArea { anchors.fill: parent }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.top: parent.top
                anchors.topMargin: 18
                text: "Настройки сервера"
                color: "#111111"
                font.pixelSize: 18
            }

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 20
                anchors.topMargin: 58
                spacing: 7

                FieldLabel { text: "IP сервера" }
                PlainField {
                    width: parent.width
                    text: root.serverAddress
                    placeholderText: "203.0.113.10"
                    onTextChanged: root.serverAddress = text.trim()
                }
                Item { width: 1; height: 6 }
                FieldLabel { text: "Control Plane port" }
                PlainField {
                    width: parent.width
                    text: root.controlPort
                    inputMethodHints: Qt.ImhDigitsOnly
                    onTextChanged: root.controlPort = text
                }
                Item { width: 1; height: 6 }
                FieldLabel { text: "Data Plane UDP port" }
                PlainField {
                    width: parent.width
                    text: root.routerPort
                    inputMethodHints: Qt.ImhDigitsOnly
                    onTextChanged: root.routerPort = text
                }
                Item { width: 1; height: 6 }
                FieldLabel { text: "Local UDP port" }
                PlainField {
                    width: parent.width
                    text: root.localPort
                    inputMethodHints: Qt.ImhDigitsOnly
                    onTextChanged: root.localPort = text
                }
                Item { width: 1; height: 6 }
                FieldLabel { text: "TUN interface" }
                PlainField {
                    width: parent.width
                    text: root.interfaceName
                    onTextChanged: root.interfaceName = text
                }
                Item { width: 1; height: 8 }
                PlainButton {
                    width: parent.width
                    text: root.viewModel && root.viewModel.serverConnected
                          ? "Подключено к серверу" : "Подключиться к серверу"
                    enabled: root.viewModel !== null && !root.working
                             && root.serverAddress.length > 0
                    onClicked: root.viewModel.connectServer(root.serverUrl())
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.userDialogVisible
        color: "#88000000"
        z: 11

        MouseArea {
            anchors.fill: parent
            onClicked: root.userDialogVisible = false
        }

        Rectangle {
            anchors.centerIn: parent
            width: 400
            height: root.viewModel && root.viewModel.authenticated ? 240 : 330
            color: "white"
            border.width: 1
            border.color: "#777777"

            MouseArea { anchors.fill: parent }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.top: parent.top
                anchors.topMargin: 18
                text: root.viewModel && root.viewModel.authenticated ? "Профиль" : "Вход"
                color: "#111111"
                font.pixelSize: 18
            }

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 20
                anchors.topMargin: 60
                spacing: 8

                FieldLabel { text: "Никнейм" }
                PlainField {
                    id: nicknameField
                    width: parent.width
                    text: root.nickname
                    enabled: !(root.viewModel && root.viewModel.authenticated)
                    onTextChanged: root.nickname = text.trim()
                }
                Item { width: 1; height: 6 }
                FieldLabel {
                    visible: !(root.viewModel && root.viewModel.authenticated)
                    text: "Пароль"
                }
                PlainField {
                    id: passwordField
                    width: parent.width
                    visible: !(root.viewModel && root.viewModel.authenticated)
                    text: root.password
                    passwordMode: true
                    onTextChanged: root.password = text
                }
                Item { width: 1; height: 8 }
                PlainButton {
                    width: parent.width
                    text: root.viewModel && root.viewModel.authenticated ? "Выйти" : "Войти"
                    enabled: root.viewModel !== null && !root.working
                             && (root.viewModel.authenticated || root.viewModel.serverConnected)
                    onClicked: {
                        root.errorDismissed = false
                        if (root.viewModel.authenticated) {
                            root.viewModel.logout()
                            root.userDialogVisible = false
                        } else if (nicknameField.text.trim().length > 0
                                   && passwordField.text.length > 0) {
                            root.viewModel.login(root.serverUrl(),
                                                 nicknameField.text.trim(),
                                                 passwordField.text)
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.networkDialogVisible
        color: "#88000000"
        z: 12

        MouseArea { anchors.fill: parent; onClicked: root.networkDialogVisible = false }
        Rectangle {
            anchors.centerIn: parent
            width: 420
            height: root.createNetworkMode ? 300 : 330
            color: "white"
            border.width: 1
            border.color: "#777777"
            MouseArea { anchors.fill: parent }
            Column {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 8
                Text {
                    text: root.createNetworkMode ? "Создать сеть" : "Вступить в сеть"
                    color: "#111111"
                    font.pixelSize: 18
                }
                Item { width: 1; height: 8 }
                FieldLabel { visible: root.createNetworkMode; text: "Название сети" }
                PlainField {
                    id: newNetworkName
                    width: parent.width
                    visible: root.createNetworkMode
                    placeholderText: "team-network"
                }
                FieldLabel { visible: !root.createNetworkMode; text: "Название сети" }
                PlainField {
                    id: joinNetworkName
                    width: parent.width
                    visible: !root.createNetworkMode
                    placeholderText: "team-network"
                }
                FieldLabel { text: "Пароль сети" }
                PlainField {
                    id: networkPassword
                    width: parent.width
                    passwordMode: true
                }
                Item { width: 1; height: 8 }
                PlainButton {
                    width: parent.width
                    text: root.createNetworkMode ? "Создать" : "Вступить"
                    enabled: !root.working && (root.createNetworkMode
                             ? newNetworkName.text.trim().length > 0
                             : joinNetworkName.text.trim().length > 0)
                    onClicked: {
                        if (root.createNetworkMode)
                            root.viewModel.createNetwork(newNetworkName.text.trim(), networkPassword.text)
                        else
                            root.viewModel.joinNetwork(joinNetworkName.text.trim(), networkPassword.text)
                        networkPassword.text = ""
                        root.networkDialogVisible = false
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.confirmationVisible
        color: "#88000000"
        z: 13

        MouseArea { anchors.fill: parent; onClicked: root.confirmationVisible = false }
        Rectangle {
            anchors.centerIn: parent
            width: 440
            height: 190
            color: "white"
            border.width: 1
            border.color: "#777777"
            MouseArea { anchors.fill: parent }
            Text {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 20
                text: root.confirmationText
                color: "#111111"
                font.pixelSize: 16
                wrapMode: Text.WordWrap
            }
            Row {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 20
                spacing: 8
                PlainButton {
                    width: 120
                    text: "Отмена"
                    onClicked: root.confirmationVisible = false
                }
                PlainButton {
                    width: 120
                    text: "Подтвердить"
                    onClicked: root.confirmAction()
                }
            }
        }
    }
}