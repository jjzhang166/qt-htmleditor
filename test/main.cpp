#include "test.h"
#include <QtWidgets/QApplication>

#pragma comment(lib, "d:/mine/htmleditor/Win32/Debug/htmleditor.lib")

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    test w;
    w.show();
    return a.exec();
}
