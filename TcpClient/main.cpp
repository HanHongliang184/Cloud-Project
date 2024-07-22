#include "tcpclient.h"
#include <QApplication>
#include <QMessageBox>
//#include "sharefile.h"
// #include "book.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFont font("Times", 24, QFont::Bold);
    a.setFont(font);

    // TcpClient w;
    // w.show();
    TcpClient::getInstance().show();

    // ShareFile w;
    // w.test();
    // w.show();

    // Book w;
    // w.show();

    return a.exec();
}
