#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDateTime>
#include <QSqlError>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_udp(new QUdpSocket(this))
    , m_listening(false)
    , m_remotePort(0)
    , m_logModel(nullptr)
{
    ui->setupUi(this);
    setupDatabase();
    setupLogModel();
    updateStatus();
    setStatusMessage("Готов");

    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::onStartListening);
    connect(ui->setRemoteButton, &QPushButton::clicked, this, &MainWindow::onSetRemote);
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onSendMessage);
    connect(ui->messageEdit, &QLineEdit::returnPressed, this, &MainWindow::onSendMessage);
    connect(m_udp, &QUdpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(m_udp, &QUdpSocket::errorOccurred, this, &MainWindow::onUdpError);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupDatabase()
{
    QString err;
    if (!m_db.open(&err)) {
        QMessageBox::critical(this, "Ошибка БД", err);
        setStatusMessage("Ошибка БД: " + err);
    } else {
        setStatusMessage("Подключено к MySQL");
    }
}

void MainWindow::setupLogModel()
{
    if (!m_db.isOpen()) return;
    if (m_logModel) delete m_logModel;
    m_logModel = new QSqlTableModel(this, m_db.database());
    m_logModel->setTable("messages");
    m_logModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_logModel->setSort(0, Qt::AscendingOrder);
    m_logModel->select();
    ui->logView->setModel(m_logModel);
    ui->logView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->logView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->logView->resizeColumnsToContents();
}

void MainWindow::updateStatus()
{
    QString text;
    if (m_listening) {
        text = QString("Слушаю порт %1").arg(m_udp->localPort());
        if (m_remotePort != 0)
            text += QString(" -> отправка на %1:%2").arg(m_remoteHost.toString()).arg(m_remotePort);
    } else {
        text = "Не активен";
    }
    ui->startButton->setText(m_listening ? "Остановить" : "Начать прослушивание");
    setWindowTitle("UDP Чат - " + text);
}

void MainWindow::setStatusMessage(const QString &msg)
{
    statusBar()->showMessage(msg, 3000);
}

void MainWindow::appendChat(const QString &text)
{
    ui->chatHistoryEdit->appendPlainText(QString("[%1] %2")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"), text));
}

void MainWindow::closeUdp()
{
    if (m_udp->state() == QUdpSocket::BoundState)
        m_udp->close();
    m_listening = false;
    m_remotePort = 0;
    m_remoteHost.clear();
    updateStatus();
}

void MainWindow::onStartListening()
{
    if (m_listening) {
        closeUdp();
        appendChat("Прослушивание остановлено");
        setStatusMessage("Прослушивание остановлено");
        return;
    }

    bool ok;
    quint16 port = ui->localPortEdit->text().toUShort(&ok);
    if (!ok || port == 0) {
        QMessageBox::warning(this, "Ошибка", "Неверный локальный порт");
        return;
    }

    if (!m_udp->bind(port)) {
        QMessageBox::critical(this, "Ошибка", m_udp->errorString());
        setStatusMessage("Не удалось начать прослушивание");
        return;
    }
    m_listening = true;
    appendChat(QString("Прослушивание UDP на порту %1").arg(port));
    setStatusMessage("Прослушивание начато");
    updateStatus();
}

void MainWindow::onSetRemote()
{
    QString host = ui->remoteHostEdit->text().trimmed();
    bool ok;
    quint16 port = ui->remotePortEdit->text().toUShort(&ok);
    if (host.isEmpty() || !ok || port == 0) {
        QMessageBox::warning(this, "Ошибка", "Неверный удалённый адрес");
        return;
    }
    m_remoteHost = QHostAddress(host);
    if (m_remoteHost.isNull()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось разрешить хост");
        return;
    }
    m_remotePort = port;
    appendChat(QString("Удалённый адрес установлен: %1:%2").arg(host).arg(port));
    if (!m_listening)
        QMessageBox::information(this, "Напоминание", "Не забудьте начать прослушивание локального порта");
    updateStatus();
}

void MainWindow::onSendMessage()
{
    if (!m_listening) {
        QMessageBox::warning(this, "Ошибка", "Нет активного прослушивания");
        return;
    }
    if (m_remotePort == 0 || m_remoteHost.isNull()) {
        QMessageBox::warning(this, "Ошибка", "Не указан удалённый адрес");
        return;
    }

    QString msg = ui->messageEdit->text().trimmed();
    if (msg.isEmpty()) return;

    QByteArray datagram = msg.toUtf8();
    qint64 sent = m_udp->writeDatagram(datagram, m_remoteHost, m_remotePort);
    if (sent == -1) {
        setStatusMessage("Ошибка отправки: " + m_udp->errorString());
        return;
    }

    appendChat("Я: " + msg);
    QString peer = QString("%1:%2").arg(m_remoteHost.toString()).arg(m_remotePort);
    QString err;
    if (!m_db.insertMessage("out", peer, msg, &err))
        setStatusMessage("Ошибка БД: " + err);
    ui->messageEdit->clear();

    if (m_logModel) m_logModel->select();
}

void MainWindow::onReadyRead()
{
    while (m_udp->hasPendingDatagrams()) {
        QByteArray buf;
        buf.resize(m_udp->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        m_udp->readDatagram(buf.data(), buf.size(), &sender, &senderPort);
        QString msg = QString::fromUtf8(buf).trimmed();
        if (msg.isEmpty()) continue;

        QString peer = QString("%1:%2").arg(sender.toString()).arg(senderPort);
        appendChat("Партнёр: " + msg);
        QString err;
        if (!m_db.insertMessage("in", peer, msg, &err))
            setStatusMessage("Ошибка БД: " + err);
        if (m_logModel) m_logModel->select();
    }
}

void MainWindow::onUdpError(QUdpSocket::SocketError)
{
    setStatusMessage("Ошибка UDP: " + m_udp->errorString());
}