#include <iostream>
#include <QScreen>
#include <QFileInfo>
#include <QCheckBox>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QApplication>
#include "Canvas.hpp"
#include "ToolsWidget.hpp"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QMainWindow mainWin;

    const auto tools = new ToolsWidget;
    const auto canvas = new Canvas(tools);
    const auto widget=QWidget::createWindowContainer(canvas);
    QObject::connect(tools, &ToolsWidget::settingChanged, canvas, qOverload<>(&Canvas::update));
    mainWin.setCentralWidget(widget);
    mainWin.addDockWidget(Qt::TopDockWidgetArea, tools);
    const auto size = app.primaryScreen()->size().height()/1.4;
    mainWin.resize(size,size);
    mainWin.show();

    return app.exec();
}
