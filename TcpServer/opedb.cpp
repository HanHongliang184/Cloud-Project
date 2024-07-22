#include "opedb.h"
#include <QDebug>
#include <QMessageBox>

opeDB::opeDB(QObject *parent)
    : QObject{parent}
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
}

opeDB &opeDB::getInstance()
{
    static opeDB instance;
    return instance;
}

void opeDB::init()
{
    m_db.setHostName("localhost");
    m_db.setDatabaseName("C:\\Users\\18430\\Desktop\\Haiyun Cloud\\TcpServer\\cloud.db");
    if (m_db.open()){
        QSqlQuery query;
        query.exec("select * from userInfo");
        while (query.next()) {
            QString data = QString("%1,%2,%3").arg(query.value(0).toString()).arg(query.value(1).toString()).arg(query.value(2).toString());
            qDebug() << data;
        }
    } else {
        QMessageBox::critical(NULL, "打开数据库", "打开数据库失败");
    }
}

opeDB::~opeDB()
{
    m_db.close();
}

bool opeDB::handleRegist(const char *name, const char *pwd)
{
    if (NULL == name || NULL == pwd) {
        qDebug() << "name | pwd is NULL";
        return false;
    }
    QString data = QString("insert into userInfo(name, pwd) values(\'%1\',\'%2\')").arg(name).arg(pwd);
    // qDebug() << data;
    QSqlQuery query;
    return query.exec(data);
}

bool opeDB::handleLogin(const char *name, const char *pwd)
{
    if (NULL == name || NULL == pwd) {
        qDebug() << "name | pwd is NULL";
        return false;
    }
    QString data = QString("select * from userInfo where name=\'%1\' and pwd=\'%2\' and online=0").arg(name).arg(pwd);
    qDebug() << data;
    QSqlQuery query;
    query.exec(data);
    if (query.next()){// 如果有数据 返回true 否则false
        data = QString("update userInfo set online=1 where name=\'%1\' and pwd=\'%2\'").arg(name).arg(pwd);
        //qDebug() << data;
        QSqlQuery query;
        query.exec(data);

        return true;
    } else {
        return false;
    }
}

void opeDB::handleOffline(const char *name)
{
    if (NULL == name) {
        qDebug() << "name is NULL";
        return;
    }
    QString data = QString("update userInfo set online=0 where name=\'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);
}

QStringList opeDB::handleAllOnline()
{
    QString data = QString("select name from userInfo where online=1");

    // qDebug() << "db_____" << data;
    QSqlQuery query;
    query.exec(data);
    QStringList result;
    result.clear();

    while (query.next()) {
        result.append(query.value(0).toString());
    }
    return result;
}

int opeDB::handleSearchUser(const char *name)
{
    if (NULL == name) {
        return 2;
    }
    QString data = QString("select online from userInfo where name=\'%1\'").arg(name);
    // qDebug() << data;
    QSqlQuery query;
    query.exec(data);

    if(query.next()){
        return query.value(0).toInt(); // 存在并在线返回1，存在不在线返回0
    }else{
        return 2; // 不存在该用户
    }
}

int opeDB::handleAddFriend(const char *pername, const char *name)
{
    // qDebug() << "handleAddFriend is ready";
    if (NULL == pername || NULL == name) {
        return 4;
    }
    QString data = QString("select * from friend "
                           "where (id = (select id from userInfo where name = \'%1\') and "
                           "friendId = (select id from userInfo where name = \'%2\')) or "  // 好友是双向的，数据库只存了单向，注意是or关系
                           "(id = (select id from userInfo where name = \'%3\') and "
                           "friendId = (select id from userInfo where name = \'%4\'))").arg(name).arg(pername).arg(pername).arg(name);
    // qDebug() << data;
    QSqlQuery query;
    query.exec(data);
    if (query.next()) {
        return 3; //双方已经是好友
    } else {
        return handleSearchUser(pername); // 查询对方，存在并在线返回1，存在不在线返回0，不存在该用户返回2
    }
}

bool opeDB::handleAddFriendAgree(const char *pername, const char *name)
{
    if (NULL == pername || NULL == name) {
        qDebug() << "handleAddFriendAgree: name is NULL";
        return false;
    }
    //qDebug() << pername << name;

    int nameId = -1, pernameId = -1;
    // 查找用户对应id
    pernameId = getIdByUserName(pername);
    nameId = getIdByUserName(name);
    //qDebug() << pernameId << nameId;
    QString data = QString("insert into friend(id, friendId) values(%1, %2)").arg(nameId).arg(pernameId);
    QSqlQuery query;
    qDebug() << data;

    return query.exec(data);
}

int opeDB::getIdByUserName(const char *name)
{

    if (NULL == name) {
        return -1;
    }
    QString data = QString("select id from userInfo where name=\'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);

    if(query.next()){
        return query.value(0).toInt();
    }
    else{
        return -1; // 不存在该用户
    }
}

QStringList opeDB::handleFlushFriend(const char *name)
{
    QStringList strFriendList;
    strFriendList.clear();
    if (NULL == name) {
        return strFriendList;
    }
    QString data = QString("select name from userInfo where online=1 and id in (select id from friend where "
                           "friendId=(select id from userInfo where name = \'%1\'))").arg(name);
    QSqlQuery query;
    // qDebug() << data;
    query.exec(data);
    while (query.next()) {
        strFriendList.append(query.value(0).toString());
        // qDebug() << query.value(0).toString();
    }

    data = QString("select name from userInfo where online=1 and id in (select friendId from friend where "
                   "id=(select id from userInfo where name = \'%1\'))").arg(name);
    // qDebug() << data;
    query.exec(data);
    while (query.next()) {
        strFriendList.append(query.value(0).toString());
        qDebug() << query.value(0).toString();
    }
    return strFriendList;
}

bool opeDB::handleDelFriend(const char *name, const char *friendName)
{
    if (NULL == name || NULL == friendName) {
        return false;
    }
    QString data = QString("delete from friend where id=(select id from userInfo where name=\'%1\') and friendId = (select id from userInfo where name=\'%2\')").arg(name).arg(friendName);
    QSqlQuery query;
    // qDebug() << data;
    query.exec(data);

    data = QString("delete from friend where id=(select id from userInfo where name=\'%1\') and friendId = (select id from userInfo where name=\'%2\')").arg(friendName).arg(name);
    query.exec(data);

    return true;
}
