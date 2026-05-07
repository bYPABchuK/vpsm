#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ControlPlaneService.hpp"
#include "DataPlaneService.hpp"
#include "MainViewModel.hpp"

#include <QMainWindow>
#include <QDialog>
#include <QLineEdit>
#include <QMap>
#include <QSpinBox>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void bindUi();
    void applyVmState();
    void openServerWindow();
    void openUserWindow();

    Ui::MainWindow *ui;
    QDialog *m_serverDialog {nullptr};
    QDialog *m_userDialog {nullptr};
    QLineEdit *m_serverControlUrl {nullptr};
    QLineEdit *m_serverRouterHost {nullptr};
    QLineEdit *m_userNickname {nullptr};
    QLineEdit *m_userPassword {nullptr};
    QSpinBox *m_serverRouterPort {nullptr};
    QSpinBox *m_serverListenPort {nullptr};
    QSpinBox *m_serverTxThreads {nullptr};
    QSpinBox *m_serverRxThreads {nullptr};
    QMap<qint64, QString> m_networkNames;
    QString m_pendingCreateNetworkPassword;
    DataPlaneService m_dataPlane;
    ControlPlaneService m_controlPlane;
    MainViewModel m_viewModel;
};
#endif // MAINWINDOW_H
