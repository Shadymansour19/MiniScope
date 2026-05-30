#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QString port = (argc > 1) ? QString::fromLocal8Bit(argv[1]) : QString();
    MainWindow w(port);
    w.setMinimumSize(1000, 500);
    w.resize(1600, 800);
    w.move(100, 100);
    w.show();
    return a.exec();
}
