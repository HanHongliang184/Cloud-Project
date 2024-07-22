#include "tcpserver.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //数据库初始化
    opeDB::getInstance().init();

    TcpServer w;
    w.show();

    return a.exec();
}
