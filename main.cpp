#include <iostream>
#include <QScreen>
#include <QFileInfo>
#include <QCheckBox>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QApplication>
#include "Canvas.hpp"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QMainWindow mainWin;

    Canvas canvas;
    canvas.resize(app.primaryScreen()->size()/1.6);
    canvas.show();

    return app.exec();
}
