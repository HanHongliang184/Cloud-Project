#include "friend.h"
#include "protocol.h"
#include "tcpclient.h"
#include <QInputDialog>
#include <QDebug>
#include "privatechat.h"
#include <QMessageBox>

Friend::Friend(QWidget *parent)
    : QWidget{parent}
{
    m_pShowMsgTE = new QTextEdit;
    m_pFrienfListWidget = new QListWidget;
    m_pInputMsgLE = new QLineEdit;

    m_pDelFriendPB = new QPushButton("删除好友");
    m_pFlushFriendPB = new QPushButton("刷新好友");
    m_pShowOnlineUserPB = new QPushButton("显示在线用户");
    m_pSearchUserPB = new QPushButton("查找用户");
    m_pMsgSendPB = new QPushButton("发送信息");
    m_pPrivateChatPB = new QPushButton("私聊");

    QVBoxLayout *pRightPBVBL = new QVBoxLayout;
    pRightPBVBL->addWidget(m_pDelFriendPB);
    pRightPBVBL->addWidget(m_pFlushFriendPB);
    pRightPBVBL->addWidget(m_pShowOnlineUserPB);
    pRightPBVBL->addWidget(m_pSearchUserPB);
    pRightPBVBL->addWidget(m_pPrivateChatPB);

    QHBoxLayout *pTopHBL = new QHBoxLayout;
    pTopHBL->addWidget(m_pShowMsgTE);
    pTopHBL->addWidget(m_pFrienfListWidget);
    pTopHBL->addLayout(pRightPBVBL);

    QHBoxLayout *pMsgHBL = new QHBoxLayout;
    pMsgHBL->addWidget(m_pInputMsgLE);
    pMsgHBL->addWidget(m_pMsgSendPB);

    m_pOnline = new Online;

    QVBoxLayout *pMain = new QVBoxLayout;
    pMain->addLayout(pTopHBL);
    pMain->addLayout(pMsgHBL);
    pMain->addWidget(m_pOnline);
    m_pOnline->hide();

    setLayout(pMain);

    connect(m_pShowOnlineUserPB, SIGNAL(clicked(bool)), this, SLOT(showOnline()));
    connect(m_pSearchUserPB, SIGNAL(clicked(bool)), this, SLOT(searchUser()));
    connect(m_pFlushFriendPB, SIGNAL(clicked(bool)), this, SLOT(flushFriend()));
    connect(m_pDelFriendPB, SIGNAL(clicked(bool)), this, SLOT(delFriend()));
    connect(m_pPrivateChatPB, SIGNAL(clicked(bool)), this, SLOT(privateChat()));
    connect(m_pMsgSendPB, SIGNAL(clicked(bool)), this, SLOT(groupChat()));
}

void Friend::showAllOnlineUser(PDU *pdu)
{
    if (NULL == pdu) {
        return;
    }
    m_pOnline->showUser(pdu);
}

void Friend::updateFriendList(PDU *pdu)
{
    if (NULL == pdu) {
        return;
    }
    uint uiSize = pdu->uiMsgLen/32;
    char caName[32] = {'\0'};
    for (uint i = 0 ; i < uiSize ; i ++) {
        memcpy(caName, (char*)(pdu->caMsg)+i*32, 32);
        m_pFrienfListWidget->addItem(caName);
    }
}

void Friend::updateGroupMsg(PDU *pdu)
{
    QString strMsg = QString("%1 says: %2").arg(pdu->caData).arg((char*)(pdu->caMsg));
    m_pShowMsgTE->append(strMsg);
}

QListWidget *Friend::getFriendList()
{
    return m_pFrienfListWidget;
}

void Friend::showOnline()
{
    if (m_pOnline->isHidden()){
        m_pOnline->show();

        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_REQUEST;
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    } else {
        m_pOnline->hide();
    }
}

void Friend::searchUser()
{
    m_strSearchName = QInputDialog::getText(this, "搜索", "用户名");
    if (!m_strSearchName.isEmpty()) {
        qDebug() << m_strSearchName;
        PDU *pdu = mkPDU(0);
        memcpy(pdu->caData, m_strSearchName.toStdString().c_str(), m_strSearchName.size());
        pdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USER_REQUEST;
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
}

void Friend::flushFriend()
{
    QString strName = TcpClient::getInstance().loginName();
    PDU *pdu = mkPDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST;
    memcpy(pdu->caData, strName.toStdString().c_str(), strName.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;

}

void Friend::delFriend()
{
    if (NULL != m_pFrienfListWidget->currentItem()) {
        QString strFriendName = m_pFrienfListWidget->currentItem()->text();
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_DEL_FRIEND_REQUEST;
        QString strSelfName = TcpClient::getInstance().loginName();
        memcpy(pdu->caData, strSelfName.toStdString().c_str(), strSelfName.size());
        memcpy(pdu->caData+32, strFriendName.toStdString().c_str(), strFriendName.size());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }

}

void Friend::privateChat()
{
    if (NULL != m_pFrienfListWidget->currentItem()) {
        QString strChatName = m_pFrienfListWidget->currentItem()->text();
        PrivateChat::getInstance().setChatName(strChatName);
        if (PrivateChat::getInstance().isHidden()) {
            PrivateChat::getInstance().show();

        }
    } else {
        QMessageBox::warning(this, "私聊", "请选择私聊对象");
    }
}

void Friend::groupChat()
{
    QString strMsg = m_pInputMsgLE->text();
    if (!strMsg.isEmpty()) {
        PDU *pdu = mkPDU(strMsg.size()+1);
        pdu->uiMsgType = ENUM_MSG_TYPE_GROUP_CHAT_REQUEST;
        QString strName = TcpClient::getInstance().loginName();
        strncpy(pdu->caData, strName.toStdString().c_str(), strName.size());
        strncpy((char*)pdu->caMsg, strMsg.toStdString().c_str(), strMsg.size());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    } else {
        QMessageBox::warning(this, "群聊", "信息不能为空");
    }
}
