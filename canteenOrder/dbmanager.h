#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

class DBManager : public QObject
{
    Q_OBJECT
public:
    static DBManager& instance(); // 单例访问点
    bool openDb();
    bool registerUser(const QString &username, const QString &password);
    bool loginUser(const QString &username, const QString &password);

private:
    explicit DBManager(QObject *parent = 0);
    QSqlDatabase m_db;
    void initTable();
};

#endif // DBMANAGER_H
