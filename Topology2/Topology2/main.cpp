#include "MainWindow.hpp"

#include <QApplication>
#include <iostream>
#include <chrono>

int main(int args, char** argv)
{
    QApplication app(args, argv);

    MainWindow w;
    w.show();

    return app.exec();
}


