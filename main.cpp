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

    const auto canvas = new Canvas;
    mainWin.setCentralWidget(canvas);
    mainWin.resize(app.primaryScreen()->size()/1.6);
    mainWin.show();

    canvas->setFocus(Qt::OtherFocusReason);

    return app.exec();
}
