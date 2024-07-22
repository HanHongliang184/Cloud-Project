#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
typedef unsigned int uint;

#define REGIST_OK "regist ok"
#define REGIST_FAILED "regist failed : name existed"
#define LOGIN_OK "login ok"
#define LOGIN_FAILED "login failed : name error or pwd error or relogin"

#define SEARCH_USER_NO "nosuch people"
#define SEARCH_USER_ONLINE "online"
#define SEARCH_USER_OFFLINE "offline"

#define UNKNOW_ERROR "unknow error"
#define EXISTED_FRIEND "friend exist"
#define ADD_FRIEND_OFFLINE "user offline"
#define ADD_FRIEND_NO_EXIST "user not exist"
#define ADD_FRIEND_OK "add friend ok"
#define DEL_FRIEND_OK  "delete friend ok"

#define DIR_NO_EXIST "dir not exist"
#define FILE_NAME_EXIST "file name exist"
#define CREATE_DIR_OK "create dir ok"

#define DEL_DIR_OK "delete dir ok"
#define DEL_DIR_FAILURED "delete dir failured: is file"


#define RENAME_FILE_OK "rename file ok"
#define RENAME_FILE_FAILURED "rename file failured"

#define ENTER_DIR_FAILURED "enter dir failured: is regular file"

#define UPLOAD_FILE_OK "upload file ok"
#define UPLOAD_FILE_FAILURED "upload file failured"

#define DEL_FILE_OK "delete file ok"
#define DEL_FILE_FAILURED "delete file failured: is diretory"

#define MOVE_FILE_OK "move file ok"
#define MOVE_FILE_FAILURED "move file failured: is regular file"

#define COMMON_ERR "operate failed: system is busy"
enum ENUM_MSG_TYPE {
    ENUM_MSG_TYPE_MIN = 0,
    ENUM_MSG_TYPE_REGIST_REQUEST,  //注册请求
    ENUM_MSG_TYPE_REGIST_RESPOND,  //注册回复

    ENUM_MSG_TYPE_LOGIN_REQUEST,  //登录请求
    ENUM_MSG_TYPE_LOGIN_RESPOND,  //登录回复

    ENUM_MSG_TYPE_ALL_ONLINE_REQUEST,  //在线用户请求
    ENUM_MSG_TYPE_ALL_ONLINE_RESPOND,  //在线用户回复

    ENUM_MSG_TYPE_SEARCH_USER_REQUEST,  //搜索用户请求
    ENUM_MSG_TYPE_SEARCH_USER_RESPOND,  //搜索用户回复

    ENUM_MSG_TYPE_ADD_FRIEND_REQUEST,  //添加好友请求
    ENUM_MSG_TYPE_ADD_FRIEND_RESPOND,  //添加好友回复

    ENUM_MSG_TYPE_ADD_FRIEND_AGREE,  //同意好友请求
    ENUM_MSG_TYPE_ADD_FRIEND_REFUSE,  //拒绝好友请求

    ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST,  //刷新好友请求
    ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND,  //刷新好友回复

    ENUM_MSG_TYPE_DEL_FRIEND_REQUEST,  //删除好友请求
    ENUM_MSG_TYPE_DEL_FRIEND_RESPOND,  //删除好友回复

    ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST,  //私聊请求
    ENUM_MSG_TYPE_PRIVATE_CHAT_RESPOND,  //私聊回复

    ENUM_MSG_TYPE_GROUP_CHAT_REQUEST,  //群聊请求
    ENUM_MSG_TYPE_GROUP_CHAT_RESPOND,  //群聊回复

    ENUM_MSG_TYPE_CREATE_DIR_REQUEST,  //创建文件夹请求
    ENUM_MSG_TYPE_CREATE_DIR_RESPOND,  //创建文件夹回复

    ENUM_MSG_TYPE_FLUSH_FILE_REQUEST,  //刷新文件请求
    ENUM_MSG_TYPE_FLUSH_FILE_RESPOND,  //刷新文件夹回复

    ENUM_MSG_TYPE_DEL_DIR_REQUEST,  //删除文件夹请求
    ENUM_MSG_TYPE_DEL_DIR_RESPOND,  //删除文件夹回复

    ENUM_MSG_TYPE_RENAME_FILE_REQUEST,  //重命名文件请求
    ENUM_MSG_TYPE_RENAME_FILE_RESPOND,  //重命名文件夹回复

    ENUM_MSG_TYPE_ENTER_DIR_REQUEST,  //进入文件夹请求
    ENUM_MSG_TYPE_ENTER_DIR_RESPOND,  //进入文件夹回复

    ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST,  //上传文件请求
    ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND,  //上传文件回复

    ENUM_MSG_TYPE_DEL_FILE_REQUEST,  //删除文件请求
    ENUM_MSG_TYPE_DEL_FILE_RESPOND,  //删除文件回复

    ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST,  //下载文件请求
    ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND,  //下载文件回复

    ENUM_MSG_TYPE_SHARE_FILE_REQUEST,  //分享文件请求
    ENUM_MSG_TYPE_SHARE_FILE_RESPOND,  //分享文件回复

    ENUM_MSG_TYPE_SHARE_FILE_NOTE_REQUEST,//分享文件通知
    ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND,//分享文件通知回复

    ENUM_MSG_TYPE_MOVE_FILE_REQUEST,//移动文件通知
    ENUM_MSG_TYPE_MOVE_FILE_RESPOND,//移动文件通知回复

    ENUM_MSG_TYPE_MAX = 0x00ffffff
};

struct FileInfo{
    char caFileName[32];    //文件名
    int iFileType;      //文件类型

};

struct PDU{
    uint uiPDULen;//总的协议数据单元大小
    uint uiMsgType;//总的消息类型
    char caData[64];
    uint uiMsgLen;//实际消息长度
    int caMsg[];//实际消息
};

PDU *mkPDU(uint uiMsgLen);

#endif // PROTOCOL_H
