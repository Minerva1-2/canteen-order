#include "dbmanager.h"

DBManager::DBManager(QObject *parent) : QObject(parent)
{
    // 初始化数据库连接，使用SQLite
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("restaurant.db"); // 数据库文件将生成在运行目录

    if(openDb()){
        initTable();
    }
}

DBManager& DBManager::instance()
{
    static DBManager instance;
    return instance;
}

bool DBManager::openDb()
{
    if (!m_db.isOpen()) {
        if (!m_db.open()) {
            qDebug() << "Error: connection with database failed";
            return false;
        }
    }
    return true;
}

void DBManager::initTable()
{
    // 创建用户表
    QSqlQuery query;
    QString sql = "CREATE TABLE IF NOT EXISTS users ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "username TEXT UNIQUE, "
                  "password TEXT)";
    if (!query.exec(sql)) {
        qDebug() << "Create table error:" << query.lastError();
    }
}

// 实现注册功能：保存用户到数据库
bool DBManager::registerUser(const QString &username, const QString &password)
{
    if(username.isEmpty() || password.isEmpty()) return false;

    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password) VALUES (:name, :pass)");
    query.bindValue(":name", username);
    query.bindValue(":pass", password); // 实际项目中建议加密存储

    if(query.exec()){
        return true;
    } else {
        qDebug() << "Register error:" << query.lastError();
        return false;
    }
}

// 登录验证功能
bool DBManager::loginUser(const QString &username, const QString &password)
{
    QSqlQuery query;
    query.prepare("SELECT username FROM users WHERE username = :name AND password = :pass");
    query.bindValue(":name", username);
    query.bindValue(":pass", password);

    if(query.exec()){
        if(query.next()){
            return true; // 找到用户
        }
    }
    return false;
}
