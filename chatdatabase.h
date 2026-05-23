#ifndef CHATDATABASE_H
#define CHATDATABASE_H

#include <QString>
#include <QSqlDatabase>

class ChatDatabase
{
public:
    ChatDatabase();
    ~ChatDatabase();

    bool open(QString *errorMessage = nullptr);
    void close();
    bool isOpen() const;
    QSqlDatabase database() const;

    bool insertMessage(const QString &direction, const QString &peer,
                       const QString &message, QString *errorMessage = nullptr);

private:
    bool createSchema(QString *errorMessage = nullptr);
    QSqlDatabase m_database;
};

#endif