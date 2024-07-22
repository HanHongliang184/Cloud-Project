#ifndef OPEDB_H
#define OPEDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>

class opeDB : public QObject
{
    Q_OBJECT
public:
    explicit opeDB(QObject *parent = nullptr);
    static opeDB& getInstance();
    void init();
    ~opeDB();

    bool handleRegist(const char *name, const char *pwd);
    bool handleLogin(const char *name, const char *pwd);
    void handleOffline(const char *name);
    QStringList handleAllOnline();
    int handleSearchUser(const char *name);
    int handleAddFriend(const char *pername, const char *name);
    bool handleAddFriendAgree(const char *pername, const char *name); // 处理同意好友申请
    int getIdByUserName(const char *name);                // 根据用户名获取用户id
    QStringList handleFlushFriend(const char *name);
    bool handleDelFriend(const char *name, const char *friendName);
signals:

public slots:
private:
    QSqlDatabase m_db;
};

#endif // OPEDB_H
