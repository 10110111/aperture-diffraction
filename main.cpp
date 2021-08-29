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
    const auto size = app.primaryScreen()->size()/1.4;
    const auto side = std::min(size.width(), size.height());
    canvas.resize(side, side);
    canvas.show();

    return app.exec();
}
