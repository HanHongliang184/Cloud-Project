#include "mytcpsocket.h"
#include "mytcpserver.h"
#include <QDir>
#include <QFileInfoList>

MyTcpSocket::MyTcpSocket(QObject *parent)
    : QTcpSocket{parent}
{
    connect(this, SIGNAL(readyRead()), this, SLOT(recvMsg()));
    connect(this, SIGNAL(disconnected()), this, SLOT(clientOffline()));
    m_bUpload = false;
    m_pTimer = new QTimer;
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(sendFileToClient()));
}

QString MyTcpSocket::getName()
{
    return m_strName;
}

void MyTcpSocket::copyDir(QString strSrcDir, QString strDestDir)
{
    QDir dir;
    dir.mkdir(strDestDir);

    dir.setPath(strSrcDir);
    QFileInfoList fileInfoList = dir.entryInfoList();
    QString srcTmp;
    QString destTmp;

    for (int i = 0 ; i < fileInfoList.size() ; i ++) {
        qDebug() << "路径：" << fileInfoList[i].fileName();
        if (fileInfoList[i].isFile()) {

            srcTmp = strSrcDir+'/'+fileInfoList[i].fileName();
            destTmp = strDestDir+'/'+fileInfoList[i].fileName();
            QFile::copy(srcTmp, destTmp);
        }
        else if (fileInfoList[i].isDir()) {
            if (QString(".") == fileInfoList[i].fileName() || QString("..") == fileInfoList[i].fileName()) {
                continue;
            }
            srcTmp = strSrcDir+'/'+fileInfoList[i].fileName();
            destTmp = strDestDir+'/'+fileInfoList[i].fileName();
            copyDir(srcTmp, destTmp);
        }
    }
}

// 同意加好友
void handleAddFriendAgree(PDU* pdu)
{
    char addedName[32] = {'\0'};
    char sourceName[32] = {'\0'};
    // 拷贝读取的信息
    strncpy(addedName, pdu -> caData, 32);
    strncpy(sourceName, pdu -> caData + 32, 32);

    // 将新的好友关系信息写入数据库
    opeDB::getInstance().handleAddFriendAgree(addedName, sourceName);

    // 服务器需要转发给发送好友请求方其被同意的消息
    MyTcpServer::getInstance().resend(sourceName, pdu);
}

// 拒绝加好友
void handleAddFriendRefuse(PDU* pdu)
{
    char sourceName[32] = {'\0'};
    // 拷贝读取的信息
    strncpy(sourceName, pdu -> caData + 32, 32);
    // 服务器需要转发给发送好友请求方其被拒绝的消息
    MyTcpServer::getInstance().resend(sourceName, pdu);
}



void MyTcpSocket::recvMsg()
{
    if (!m_bUpload) {
    qDebug() << this->bytesAvailable();
    uint uiPDULen = 0;
    this->read((char*)&uiPDULen, sizeof(uint));
    uint uiMsgLen = uiPDULen - sizeof(PDU);
    PDU *pdu = mkPDU(uiMsgLen);
    this->read((char*)pdu+sizeof(uint), uiPDULen - sizeof(uint));
    switch(pdu->uiMsgType) {
    case ENUM_MSG_TYPE_REGIST_REQUEST: {
        char caName[32] = {'\0'};
        char caPwd[32] = {'\0'};
        strncpy(caName, pdu->caData, 32);
        strncpy(caPwd, pdu->caData+32, 32);
        bool ret = opeDB::getInstance().handleRegist(caName, caPwd);
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_REGIST_RESPOND;
        if (ret){
            strcpy(respdu->caData, REGIST_OK);
            QDir dir;
            qDebug() << "create dir" << dir.mkdir(QString("./%1").arg(caName));

        } else{
            strcpy(respdu->caData, REGIST_FAILED);
        }
        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_LOGIN_REQUEST:{
        char caName[32] = {'\0'};
        char caPwd[32] = {'\0'};
        strncpy(caName, pdu->caData, 32);
        strncpy(caPwd, pdu->caData+32, 32);
        bool ret = opeDB::getInstance().handleLogin(caName, caPwd);
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND;
        if (ret){
            strcpy(respdu->caData, LOGIN_OK);
            m_strName = caName;
        } else{
            strcpy(respdu->caData, LOGIN_FAILED);
        }
        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST:{
        QStringList ret = opeDB::getInstance().handleAllOnline();
        uint uiMsgLen = ret.size()*32;
        PDU *respdu = mkPDU(uiMsgLen);
        respdu->uiMsgType= ENUM_MSG_TYPE_ALL_ONLINE_RESPOND;
        for (int i = 0 ; i < ret.size() ; i ++) {
            memcpy((char*)(respdu->caMsg)+i*32
                   , ret.at(i).toStdString().c_str()
                   , ret.at(i).size());
        }
        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_SEARCH_USER_REQUEST:{
        char caName[32] = {'\0'};
        strncpy(caName, pdu -> caData, 32);
        int ret = opeDB::getInstance().handleSearchUser(caName);
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USER_RESPOND;
        if (-1 == ret) {
            strcpy(respdu->caData, SEARCH_USER_NO);
        }
        else if (1 == ret) {
            strcpy(respdu->caData, SEARCH_USER_ONLINE);
        }
        else if (0 == ret){
            strcpy(respdu->caData, SEARCH_USER_OFFLINE);
        }
        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST: {
        char caPerName[32] = {'\0'};
        char caName[32] = {'\0'};
        strncpy(caPerName, pdu->caData, 32);
        strncpy(caName, pdu->caData+32, 32);
        qDebug() << "handleAddFriendRequest  " << caPerName << " " << caName;
        // qDebug() << "handleAddFriend is ready";
        int ret = opeDB::getInstance().handleAddFriend(caPerName, caName);
        PDU *respdu = NULL;

        if (4 == ret) {
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData, UNKNOW_ERROR);
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        else if (3 == ret) {
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData, EXISTED_FRIEND);
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        else if (1 == ret) {
            MyTcpServer::getInstance().resend(caPerName, pdu);

            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData, ADD_FRIEND_OK);
            free(respdu);
            respdu = NULL;
        }
        else if (0 == ret) {
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData, ADD_FRIEND_OFFLINE);
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        else if (2 == ret) {
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strcpy(respdu->caData, ADD_FRIEND_NO_EXIST);
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_AGREE: {
        handleAddFriendAgree(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE: {
        handleAddFriendRefuse(pdu);
        break;
    }
    case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST: {
        char caName[32] = {'\0'};
        strncpy(caName, pdu->caData, 32);
        QStringList ret = opeDB::getInstance().handleFlushFriend(caName);
        uint uiMsgLen = ret.size()*32;
        PDU *respdu = mkPDU(uiMsgLen);
        respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;
        for (int i = 0 ; i < ret.size() ; i ++) {
            memcpy((char*)(respdu->caMsg)+i*32, ret.at(i).toStdString().c_str(), ret.at(i).size());

        }
        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_DEL_FRIEND_REQUEST: {
        char caSelfName[32] = {'\0'};
        char caFriendName[32] = {'\0'};
        strncpy(caSelfName, pdu -> caData, 32);
        strncpy(caFriendName, pdu -> caData + 32, 32);
        opeDB::getInstance().handleDelFriend(caSelfName, caFriendName);

        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FRIEND_RESPOND;
        strcpy(respdu->caData, DEL_FRIEND_OK);
        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;

        MyTcpServer::getInstance().resend(caFriendName, pdu);

        break;
    }
    case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST: {
        char caPerName[32] = {'\0'};
        memcpy(caPerName, pdu->caData+32, 32);
        qDebug() << caPerName;
        MyTcpServer::getInstance().resend(caPerName, pdu);


        break;
    }
    case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST: {
        char caName[32] = {'\0'};
        strncpy(caName, pdu->caData, 32);
        QStringList onlineFriend = opeDB::getInstance().handleFlushFriend(caName);
        QString tmp;
        for (int i = 0 ; i < onlineFriend.size() ; i ++) {
            tmp = onlineFriend.at(i);
            MyTcpServer::getInstance().resend(tmp.toStdString().c_str(), pdu);
        }
        break;
    }
    case ENUM_MSG_TYPE_CREATE_DIR_REQUEST: {
        QDir dir;
        QString strCurPath = QString("%1").arg((char*)(pdu->caMsg));
        qDebug() << strCurPath;
        bool ret = dir.exists(strCurPath);
        PDU *respdu = NULL;
        if (ret) {  //当前目录存在
            char caNewDir[32] = {'\0'};
            memcpy(caNewDir, pdu->caData+32, 32);
            QString strNewPath = strCurPath + "/" + caNewDir;
            qDebug() << strNewPath;
            ret = dir.exists(strNewPath);
            qDebug() << "-->" << ret;
            if (ret) { //创建的文件已存在
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                strcpy(respdu->caData, FILE_NAME_EXIST);
            } else {   //创建的文件不存在
                dir.mkdir(strNewPath);
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                strcpy(respdu->caData, CREATE_DIR_OK);
            }
        } else {    //当前目录不存在
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
            strcpy(respdu->caData, DIR_NO_EXIST);

        }
        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_FLUSH_FILE_REQUEST: {
        char *pCurPath = new char[pdu->uiMsgLen];
        memcpy(pCurPath, pdu->caMsg, pdu->uiMsgLen);
        QDir dir(pCurPath);
        QFileInfoList fileInfoList = dir.entryInfoList();
        int iFileCount = fileInfoList.size();
        PDU *respdu = mkPDU(sizeof(FileInfo)*iFileCount);
        respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
        FileInfo *pFileInfo = NULL;
        QString strFileName;
        for (int i = 0 ; i < iFileCount ; i ++) {
            // if (QString(".") == fileInfoList[i].fileName() || QString("..") == fileInfoList[i].fileName()) {
            //     continue;
            // }
            pFileInfo = (FileInfo*)(respdu->caMsg)+i;
            strFileName = fileInfoList[i].fileName();

            memcpy(pFileInfo->caFileName, strFileName.toStdString().c_str(), strFileName.size());
            if (fileInfoList[i].isDir()) {
                pFileInfo->iFileType = 0;
            }
            else if (fileInfoList[i].isFile()){
                pFileInfo->iFileType = 1;
            }
            // qDebug() << fileInfoList[i].fileName()
            //          << fileInfoList[i].size()
            //          <<"文件夹:" << fileInfoList[i].isDir()
            //          << "常规文件:" << fileInfoList[i].isFile();
        }
        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_DEL_DIR_REQUEST: {
        char caName[32] = {'\0'};
        strcpy(caName, pdu->caData);
        char *pPath = new char[pdu->uiMsgLen];
        memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
        QString strPath = QString("%1/%2").arg(pPath).arg(caName);
        // qDebug() << strPath;

        QFileInfo fileInfo(strPath);
        bool ret = false;
        if (fileInfo.isDir()) {
            QDir dir;
            dir.setPath(strPath);
            ret = dir.removeRecursively();
        } else if (fileInfo.isFile()){
            ret = false;
        }
        PDU *respdu = NULL;
        if (ret) {
            respdu = mkPDU(strlen(DEL_DIR_OK)+1);
            respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
            memcpy(respdu->caData, DEL_DIR_OK, strlen(DEL_DIR_OK));
        } else {
            respdu = mkPDU(strlen(DEL_DIR_FAILURED)+1);
            respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
            memcpy(respdu->caData, DEL_DIR_FAILURED, strlen(DEL_DIR_FAILURED));
        }

        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;

        break;
    }
    case ENUM_MSG_TYPE_RENAME_FILE_REQUEST: {
        char caOldName[32] = {'\0'};
        char caNewName[32] = {'\0'};
        strncpy(caOldName, pdu->caData, 32);
        strncpy(caNewName, pdu->caData+32, 32);

        char *pPath = new char[pdu->uiMsgLen];
        memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);

        QString strOldPath = QString("%1/%2").arg(pPath).arg(caOldName);
        QString strNewPath = QString("%1/%2").arg(pPath).arg(caNewName);

        // qDebug() << strOldPath << strNewPath;

        QDir dir;
        bool ret = dir.rename(strOldPath, strNewPath);
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_RENAME_FILE_RESPOND;
        if (ret) {
            strcpy(respdu->caData, RENAME_FILE_OK);
        } else {
            strcpy(respdu->caData, RENAME_FILE_FAILURED);
        }

        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;

        break;
    }
    case ENUM_MSG_TYPE_ENTER_DIR_REQUEST: {
        char caEnterName[32] = {'\0'};
        strncpy(caEnterName, pdu->caData, 32);
        char *pPath = new char[pdu->uiMsgLen];
        memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);

        QString strPath = QString("%1/%2").arg(pPath).arg(caEnterName);

        qDebug() << strPath;

        QFileInfo fileInfo(strPath);
        PDU *respdu = NULL;
        if (fileInfo.isDir()) {
            QDir dir(strPath);
            QFileInfoList fileInfoList = dir.entryInfoList();
            int iFileCount = fileInfoList.size();
            respdu = mkPDU(sizeof(FileInfo)*iFileCount);
            respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
            FileInfo *pFileInfo = NULL;
            QString strFileName;
            for (int i = 0 ; i < iFileCount ; i ++) {
                pFileInfo = (FileInfo*)(respdu->caMsg)+i;
                strFileName = fileInfoList[i].fileName();
                memcpy(pFileInfo->caFileName, strFileName.toStdString().c_str(), strFileName.size());
                if (fileInfoList[i].isDir()) {
                    pFileInfo->iFileType = 0;
                }
                else if (fileInfoList[i].isFile()){
                    pFileInfo->iFileType = 1;
                }
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        else if (fileInfo.isFile()){
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_RESPOND;
            strcpy(respdu->caData, ENTER_DIR_FAILURED);

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        break;
    }
    case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST: {
        char caFileName[32] = {'\0'};
        qint64 fileSize = 0;
        sscanf(pdu->caData, "%s %lld", caFileName, &fileSize);
        char *pPath = new char[pdu->uiMsgLen];
        memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
        QString strPath = QString("%1/%2").arg(pPath).arg(caFileName);
        qDebug() << strPath;
        delete[]pPath;
        pPath = NULL;
        
        m_file.setFileName(strPath);
        //以只写的方式打开文件，若文件不存在，则会自动创建文件
        if (m_file.open(QIODevice::WriteOnly)) {
            m_bUpload = true;
            m_iTotal = fileSize;
            m_iRecved = 0;
        }
        
        break;
    }
    case ENUM_MSG_TYPE_DEL_FILE_REQUEST: {
        char caName[32] = {'\0'};
        strcpy(caName, pdu->caData);
        char *pPath = new char[pdu->uiMsgLen];
        memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
        QString strPath = QString("%1/%2").arg(pPath).arg(caName);
        // qDebug() << strPath;

        QFileInfo fileInfo(strPath);
        bool ret = false;
        if (fileInfo.isDir()) {
            ret = false;
        } else if (fileInfo.isFile()){
            QDir dir;
            ret = dir.remove(strPath);
        }
        PDU *respdu = NULL;
        if (ret) {
            respdu = mkPDU(strlen(DEL_DIR_OK)+1);
            respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPOND;
            memcpy(respdu->caData, DEL_DIR_OK, strlen(DEL_FILE_OK));
        } else {
            respdu = mkPDU(strlen(DEL_DIR_FAILURED)+1);
            respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPOND;
            memcpy(respdu->caData, DEL_DIR_FAILURED, strlen(DEL_FILE_FAILURED));
        }

        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST: {
        char caFileName[32] = {'\0'};
        memcpy(caFileName, pdu->caData, 32);
        char *pPath = new char[pdu->uiMsgLen];
        memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
        QString strNewPath = QString("%1/%2").arg(pPath).arg(caFileName);

        qDebug() << "当前下载文件的地址：" << strNewPath;
        delete[]pPath;
        pPath = NULL;

        QFileInfo fileInfo(strNewPath);
        qint64 fileSize = fileInfo.size();
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND;
        sprintf(respdu->caData, "%s %lld", caFileName, fileSize);

        write((char*)respdu, respdu->uiPDULen);
        //qDebug() << "respdu->caData" << respdu->caData;
        free(respdu);
        respdu = NULL;

        m_file.setFileName(strNewPath);
        m_file.open(QIODevice::ReadOnly);
        m_pTimer->start(1000);

        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_REQUEST: {
        char caSendName[32] = {'\0'};
        int num = 0;
        sscanf(pdu->caData, "%s %d", caSendName, &num);
        //qDebug() << "分享文件的人：" << caSendName << " 人数：" << num;
        int size = num*32;
        PDU *respdu = mkPDU(pdu->uiMsgLen-size);
        respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_NOTE_REQUEST;

        strcpy(respdu->caData, caSendName);
        // qDebug() << "respdu->caData" << respdu->caData;
        memcpy(respdu->caMsg, (char*)(pdu->caMsg)+size, pdu->uiMsgLen-size);
        // qDebug() << "respdu->caMsg" << respdu->caMsg; // respdu->caMsg 0x27254b5ec2c ???

        char caRecvName[32] = {'\0'};
        for (int i = 0 ; i < num ; i ++) {
            memcpy(caRecvName, (char*)(pdu->caMsg)+i*32, 32);
            MyTcpServer::getInstance().resend(caRecvName, respdu);
        }
        free(respdu);
        respdu = NULL;

        respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_RESPOND;
        strcpy(respdu->caData, "share file ok");
        // qDebug() << "respdu->caData" << respdu->caData;
        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;

        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND: {

        QString strRecvPath = QString("./%1").arg(pdu->caData);
        QString strShareFilePath = QString("%1").arg((char*)(pdu->caMsg));
        int index = strShareFilePath.lastIndexOf('/');
        QString strFileName = strShareFilePath.right(strShareFilePath.size()-index-1);
        strRecvPath = strRecvPath+'/'+strFileName;

        QFileInfo fileInfo(strShareFilePath);
        if (fileInfo.isFile()) {
            QFile::copy(strShareFilePath, strRecvPath);
        }
        else if (fileInfo.isDir()){
            copyDir(strShareFilePath, strRecvPath);
        }
        break;
    }
    case ENUM_MSG_TYPE_MOVE_FILE_REQUEST: {
        char caFileName[32] = {'\0'};
        int srcLen = 0;
        int destLen = 0;
        sscanf(pdu->caData, "%d%d%s", &srcLen, &destLen, caFileName);

        char *pSrcPath = new char[srcLen + 1];
        char *pDestPath = new char[destLen + 1 + 32];
        memset(pSrcPath, '\0', srcLen + 1);
        memset(pDestPath, '\0', destLen + 1 + 32);

        memcpy(pSrcPath, pdu->caMsg, srcLen);
        memcpy(pDestPath, (char*)(pdu->caMsg) + (srcLen + 1), destLen);

        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_MOVE_FILE_RESPOND;
        QFileInfo fileInfo(pDestPath);
        if (fileInfo.isDir()) {
            strcat(pDestPath, "/");
            strcat(pDestPath, caFileName);

            bool ret = QFile::rename(pSrcPath, pDestPath);
            if (ret) {
                strcpy(respdu->caData, MOVE_FILE_OK);
            } else {
                strcpy(respdu->caData, COMMON_ERR);
            }
        }
        else if (fileInfo.isFile()) {
            strcpy(respdu->caData, MOVE_FILE_FAILURED);
        }

        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;

        break;
    }
    default:
        break;
    }

    free(pdu);
    pdu = NULL;
    }
    else {
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
        QByteArray buffer = readAll();
        m_file.write(buffer);
        m_iRecved += buffer.size();
        if (m_iTotal == m_iRecved) {
            m_file.close();
            m_bUpload = false;
            strcpy(respdu->caData, UPLOAD_FILE_OK);
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        else if (m_iTotal < m_iRecved) {
            m_file.close();
            m_bUpload = false;
            strcpy(respdu->caData, UPLOAD_FILE_FAILURED);
        }
    }
}

void MyTcpSocket::clientOffline()
{
    opeDB::getInstance().handleOffline(m_strName.toStdString().c_str());
    emit offline(this);
}

void MyTcpSocket::sendFileToClient()
{
    m_pTimer->stop();
    char *pData = new char[4096];
    quint64 ret = 0;
    while (true) {
        ret = m_file.read(pData, 4096);
        if (ret > 0 && ret <= 4096) {
            write(pData, ret);
        }
        else if (0 == ret) {
            m_file.close();
            break;
        }
        else if (ret < 0) {
            qDebug() << "发送文件给客户端失败";
            m_file.close();
            break;
        }
    }
    delete[] pData;
    pData = NULL;
}
