#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDateTime>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QInputDialog>

namespace {
constexpr int kNetworkIdRole = Qt::UserRole;
constexpr int kExpandedRole = Qt::UserRole + 1;
constexpr int kIsPeerRole = Qt::UserRole + 2;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_dataPlane(this)
    , m_controlPlane(nullptr)
    , m_viewModel(&m_dataPlane, &m_controlPlane, this)
{
    ui->setupUi(this);

    bindUi();
    applyVmState();
}

MainWindow::~MainWindow()
{
    m_viewModel.stopSession();
    delete ui;
}

void MainWindow::bindUi()
{
    connect(ui->buttonServer, &QPushButton::clicked, this, &MainWindow::openServerWindow);
    connect(ui->buttonUser, &QPushButton::clicked, this, &MainWindow::openUserWindow);

    connect(ui->buttonConnect, &QPushButton::clicked, this, [this]() {
        if (m_viewModel.connected()) {
            m_viewModel.stopSession();
            return;
        }

        if (!m_serverRouterHost || !m_serverRouterPort || !m_serverListenPort || !m_serverTxThreads || !m_serverRxThreads) {
            QMessageBox::information(this, tr("Сервер"), tr("Сначала откройте окно 'Сервер' и задайте параметры подключения."));
            return;
        }

        const QString nick = m_userNickname ? m_userNickname->text().trimmed() : QString();
        const QString password = m_userPassword ? m_userPassword->text() : QString();
        if (nick.isEmpty() || password.isEmpty()) {
            QMessageBox::information(this, tr("Пользователь"), tr("Укажите ник и пароль в окне 'Пользователь'."));
            return;
        }

        m_controlPlane.login(
            m_serverControlUrl ? m_serverControlUrl->text().trimmed() : QStringLiteral("http://127.0.0.1:8080"),
            nick,
            password);

        setWindowTitle(tr("VPSM Qt Client — %1").arg(nick));
    });

    connect(ui->buttonSendPacket, &QPushButton::clicked, this, [this]() {
        m_viewModel.sendPacket(
            ui->editSrcVip->text().trimmed(),
            ui->editDstVip->text().trimmed(),
            ui->editNetworkIp->text().trimmed(),
            ui->editPayload->text());
    });

    connect(ui->buttonAddNetwork, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        const QString networkName = QInputDialog::getText(this,
                                                           tr("Добавить сеть"),
                                                           tr("Название сети:"),
                                                           QLineEdit::Normal,
                                                           QString(),
                                                           &ok).trimmed();
        if (!ok || networkName.isEmpty()) {
            return;
        }

        const QString networkPassword = QInputDialog::getText(this,
                                                               tr("Добавить сеть"),
                                                               tr("Пароль сети:"),
                                                               QLineEdit::Password,
                                                               QString(),
                                                               &ok);
        if (!ok || networkPassword.isEmpty()) {
            return;
        }

        m_pendingCreateNetworkPassword = networkPassword;
        m_controlPlane.createNetwork(networkName, networkPassword);
    });

    connect(ui->buttonJoinNetwork, &QPushButton::clicked, this, [this]() {
        bool idOk = false;
        const qint64 networkId = QInputDialog::getInt(this,
                                                       tr("Подключиться к сети"),
                                                       tr("ID сети:"),
                                                       1,
                                                       1,
                                                       std::numeric_limits<int>::max(),
                                                       1,
                                                       &idOk);
        if (!idOk) {
            return;
        }

        bool ok = false;
        const QString password = QInputDialog::getText(this,
                                                        tr("Подключиться к сети"),
                                                        tr("Пароль сети ID=%1:").arg(networkId),
                                                        QLineEdit::Password,
                                                        QString(),
                                                        &ok);
        if (!ok || password.isEmpty()) {
            return;
        }

        m_controlPlane.joinNetwork(networkId, password);
    });

    connect(ui->listNetworks, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (!item) return;
        if (item->data(kIsPeerRole).toBool()) {
            return; // peer row
        }

        const qint64 networkId = item->data(kNetworkIdRole).toLongLong();
        const QString networkName = m_networkNames.value(networkId, item->text().mid(2));
        const bool expanded = item->data(kExpandedRole).toBool();
        const int row = ui->listNetworks->row(item);

        if (!expanded) {
            item->setText(QStringLiteral("▾ %1").arg(networkName));
            item->setData(kExpandedRole, true);
            m_controlPlane.listUserNetworkPeers();
        } else {
            item->setText(QStringLiteral("▸ %1").arg(networkName));
            item->setData(kExpandedRole, false);
            int next = row + 1;
            while (next < ui->listNetworks->count()) {
                QListWidgetItem *candidate = ui->listNetworks->item(next);
                if (!candidate->data(kIsPeerRole).toBool()) {
                    break;
                }
                delete ui->listNetworks->takeItem(next);
            }
        }
    });

    connect(&m_viewModel, &MainViewModel::timerDisplayChanged, this, &MainWindow::applyVmState);
    connect(&m_viewModel, &MainViewModel::connectedChanged, this, &MainWindow::applyVmState);
    connect(&m_viewModel, &MainViewModel::statusTextChanged, this, &MainWindow::applyVmState);
    connect(&m_viewModel, &MainViewModel::packetLineAdded, this, [this](const QString &line) {
        ui->textLog->appendPlainText(line);
    });

    connect(&m_controlPlane, &ControlPlaneService::loginFinished, this, [this](bool ok, const QString &message) {
        if (!ok) {
            if (m_viewModel.connected()) {
                m_viewModel.stopSession();
            }
            QMessageBox::warning(this, tr("Логин"), message);
            return;
        }

        if (m_serverRouterHost && m_serverRouterPort && m_serverListenPort && m_serverTxThreads && m_serverRxThreads) {
            const QString logPath = QStringLiteral("vpsm_packet_log_%1.jsonl")
                                        .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_hhmmss")));
            m_viewModel.startSession(
                m_serverRouterHost->text().trimmed(),
                static_cast<quint16>(m_serverRouterPort->value()),
                static_cast<quint16>(m_serverListenPort->value()),
                m_serverTxThreads->value(),
                m_serverRxThreads->value(),
                logPath);
        }

        m_controlPlane.listUserNetworks();
    });

    connect(&m_controlPlane, &ControlPlaneService::networkCreated, this, [this](bool ok, qint64 networkId, const QString &message) {
        if (!ok) {
            m_pendingCreateNetworkPassword.clear();
            QMessageBox::warning(this, tr("Создание сети"), message);
            return;
        }

        if (!m_pendingCreateNetworkPassword.isEmpty()) {
            const QString password = m_pendingCreateNetworkPassword;
            m_pendingCreateNetworkPassword.clear();
            m_controlPlane.joinNetwork(networkId, password);
            return;
        }

        m_controlPlane.listUserNetworks();
    });

    connect(&m_controlPlane, &ControlPlaneService::networkJoined, this, [this](bool ok, qint64 networkId, const QString &message) {
        if (!ok) {
            QMessageBox::warning(this, tr("Подключение к сети"), message);
            return;
        }
        QMessageBox::information(this, tr("Подключение к сети"), tr("Подключение к сети %1 выполнено.").arg(networkId));
        m_controlPlane.listUserNetworks();
    });

    connect(&m_controlPlane, &ControlPlaneService::networksListed, this, [this](bool ok, const QList<qint64> &networkIds, const QMap<qint64, QString> &networkNames, const QString &message) {
        if (!ok) {
            QMessageBox::warning(this, tr("Список сетей"), message);
            return;
        }
        ui->listNetworks->clear();
        m_networkNames.clear();
        for (qint64 id : networkIds) {
            const QString name = networkNames.value(id, QStringLiteral("network-%1").arg(id));
            m_networkNames.insert(id, name);
            auto *item = new QListWidgetItem(QStringLiteral("▸ %1").arg(name));
            item->setData(kNetworkIdRole, id);
            item->setData(kExpandedRole, false);
            item->setData(kIsPeerRole, false);
            ui->listNetworks->addItem(item);
        }
    });

    connect(&m_controlPlane, &ControlPlaneService::networkPeersListed, this, [this](bool ok, const QMap<qint64, QList<NetworkPeerInfo>> &peersByNetwork, const QString &message) {
        if (!ok) {
            QMessageBox::warning(this, tr("Список участников"), message);
            return;
        }

        for (int i = 0; i < ui->listNetworks->count(); ++i) {
            QListWidgetItem *netItem = ui->listNetworks->item(i);
            if (!netItem || netItem->data(kIsPeerRole).toBool()) continue;
            if (!netItem->data(kExpandedRole).toBool()) continue;

            int next = i + 1;
            while (next < ui->listNetworks->count() && ui->listNetworks->item(next)->data(kIsPeerRole).toBool()) {
                delete ui->listNetworks->takeItem(next);
            }

            const qint64 nid = netItem->data(kNetworkIdRole).toLongLong();
            const auto peers = peersByNetwork.value(nid);
            int insertAt = i + 1;
            for (const auto &peer : peers) {
                const QString displayNick = peer.nickname.isEmpty() ? QStringLiteral("peer") : peer.nickname;
                const QString displayVip = peer.vip.isEmpty() ? QStringLiteral("-") : peer.vip;
                auto *peerItem = new QListWidgetItem(
                    QStringLiteral("    • id=%1 vip=%2 %3")
                        .arg(peer.peerId)
                        .arg(displayVip)
                        .arg(displayNick));
                peerItem->setData(kIsPeerRole, true);
                ui->listNetworks->insertItem(insertAt++, peerItem);
            }
        }
    });

}

void MainWindow::openServerWindow()
{
    if (!m_serverDialog) {
        m_serverDialog = new QDialog(this);
        m_serverDialog->setWindowTitle(tr("Сервер"));
        m_serverDialog->resize(520, 320);

        auto *layout = new QVBoxLayout(m_serverDialog);
        auto *form = new QFormLayout();

        m_serverControlUrl = new QLineEdit(QStringLiteral("http://127.0.0.1:8080"), m_serverDialog);
        m_serverRouterHost = new QLineEdit(QStringLiteral("127.0.0.1"), m_serverDialog);
        m_serverRouterPort = new QSpinBox(m_serverDialog); m_serverRouterPort->setMaximum(65535); m_serverRouterPort->setValue(9000);
        m_serverListenPort = new QSpinBox(m_serverDialog); m_serverListenPort->setMaximum(65535); m_serverListenPort->setValue(9001);
        m_serverTxThreads = new QSpinBox(m_serverDialog); m_serverTxThreads->setRange(1, 32); m_serverTxThreads->setValue(2);
        m_serverRxThreads = new QSpinBox(m_serverDialog); m_serverRxThreads->setRange(1, 32); m_serverRxThreads->setValue(2);

        form->addRow(tr("Control URL"), m_serverControlUrl);
        form->addRow(tr("Router host"), m_serverRouterHost);
        form->addRow(tr("Router port"), m_serverRouterPort);
        form->addRow(tr("Listen port"), m_serverListenPort);
        form->addRow(tr("TX threads (n)"), m_serverTxThreads);
        form->addRow(tr("RX threads (m)"), m_serverRxThreads);
        layout->addLayout(form);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, m_serverDialog);
        connect(buttons, &QDialogButtonBox::rejected, m_serverDialog, &QDialog::close);
        layout->addWidget(buttons);
    }
    m_serverDialog->show();
    m_serverDialog->raise();
    m_serverDialog->activateWindow();
}

void MainWindow::openUserWindow()
{
    if (!m_userDialog) {
        m_userDialog = new QDialog(this);
        m_userDialog->setWindowTitle(tr("Пользователь"));
        m_userDialog->resize(460, 240);

        auto *layout = new QVBoxLayout(m_userDialog);
        auto *form = new QFormLayout();
        m_userNickname = new QLineEdit(m_userDialog);
        m_userNickname->setPlaceholderText(tr("nick"));
        m_userPassword = new QLineEdit(m_userDialog);
        m_userPassword->setPlaceholderText(tr("password"));
        m_userPassword->setEchoMode(QLineEdit::Password);
        form->addRow(tr("Ник"), m_userNickname);
        form->addRow(tr("Пароль"), m_userPassword);
        layout->addLayout(form);

        auto *hint = new QLabel(tr("Эти данные будут использованы при авторизации на сервере (/user/login)."), m_userDialog);
        hint->setWordWrap(true);
        layout->addWidget(hint);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, m_userDialog);
        connect(buttons, &QDialogButtonBox::rejected, m_userDialog, &QDialog::close);
        layout->addWidget(buttons);
    }
    m_userDialog->show();
    m_userDialog->raise();
    m_userDialog->activateWindow();
}

void MainWindow::applyVmState()
{
    ui->labelTimer->setText(m_viewModel.timerDisplay());
    ui->labelStatus->setText(m_viewModel.statusText());
    ui->buttonConnect->setText(m_viewModel.connected() ? tr("ОТКЛЮЧИТЬ") : tr("ПОДКЛЮЧИТЬ"));

    ui->buttonConnect->setEnabled(true);
}
