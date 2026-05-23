#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QSqlTableModel>
#include "chatdatabase.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartListening();
    void onSetRemote();
    void onSendMessage();
    void onReadyRead();
    void onUdpError(QUdpSocket::SocketError);

private:
    void setupDatabase();
    void setupLogModel();
    void updateStatus();
    void appendChat(const QString &text);
    void setStatusMessage(const QString &msg);
    void closeUdp();

    Ui::MainWindow *ui;
    QUdpSocket *m_udp;
    bool m_listening;
    QHostAddress m_remoteHost;
    quint16 m_remotePort;
    ChatDatabase m_db;
    QSqlTableModel *m_logModel;
};

#endif