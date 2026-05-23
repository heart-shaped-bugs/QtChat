#include "chatdatabase.h"
#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

ChatDatabase::ChatDatabase() {}

ChatDatabase::~ChatDatabase()
{
    close();
}

bool ChatDatabase::open(QString *errorMessage)
{
    if (m_database.isValid())
        close();

    QString connName = QStringLiteral("mysql_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    m_database = QSqlDatabase::addDatabase("QMYSQL", connName);

    // ВАЖНО: используем 127.0.0.1 вместо localhost, чтобы принудительно задействовать TCP/IP
    m_database.setHostName("127.0.0.1");
    m_database.setPort(3306);
    m_database.setDatabaseName("chat_db");
    m_database.setUserName("chat_user");
    m_database.setPassword("chat_password");

    if (!m_database.open()) {
        if (errorMessage)
            *errorMessage = m_database.lastError().text();
        return false;
    }

    return createSchema(errorMessage);
}

void ChatDatabase::close()
{
    if (m_database.isValid()) {
        QString connName = m_database.connectionName();
        m_database.close();
        m_database = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
    }
}

bool ChatDatabase::isOpen() const
{
    return m_database.isValid() && m_database.isOpen();
}

QSqlDatabase ChatDatabase::database() const
{
    return m_database;
}

bool ChatDatabase::createSchema(QString *errorMessage)
{
    QSqlQuery query(m_database);
    QString sql = "CREATE TABLE IF NOT EXISTS messages ("
                  "id INT AUTO_INCREMENT PRIMARY KEY,"
                  "direction VARCHAR(10) NOT NULL,"
                  "peer VARCHAR(100) NOT NULL,"
                  "message TEXT NOT NULL,"
                  "timestamp VARCHAR(30) NOT NULL)";
    if (!query.exec(sql)) {
        if (errorMessage)
            *errorMessage = query.lastError().text();
        return false;
    }
    return true;
}

bool ChatDatabase::insertMessage(const QString &direction, const QString &peer,
                                 const QString &message, QString *errorMessage)
{
    if (!isOpen()) {
        if (errorMessage)
            *errorMessage = "Database not open";
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare("INSERT INTO messages(direction, peer, message, timestamp) VALUES (?, ?, ?, ?)");
    query.addBindValue(direction);
    query.addBindValue(peer);
    query.addBindValue(message);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODateWithMs));

    if (!query.exec()) {
        if (errorMessage)
            *errorMessage = query.lastError().text();
        return false;
    }
    return true;
}