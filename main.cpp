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
#include "ApertureOutline.hpp"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":icon.png"));

    QMainWindow mainWin;
    mainWin.setWindowTitle("Aperture diffraction");

    const auto tools = new ToolsWidget;
    const auto canvas = new Canvas(tools);
    const auto widget=QWidget::createWindowContainer(canvas);
    QObject::connect(tools, &ToolsWidget::settingChanged, canvas, qOverload<>(&Canvas::update));
    mainWin.setCentralWidget(widget);
    mainWin.addDockWidget(Qt::TopDockWidgetArea, tools);

    const auto apertureOutline = new ApertureOutline(tools);
    QObject::connect(tools, &ToolsWidget::settingChanged, apertureOutline, qOverload<>(&ApertureOutline::update));
    const auto apOutDock = new QDockWidget("Aperture outline");
    apOutDock->setWidget(apertureOutline);
    mainWin.addDockWidget(Qt::TopDockWidgetArea, apOutDock);

    const auto size = app.primaryScreen()->size().height()/1.4;
    mainWin.resize(size,size);
    mainWin.show();

    return app.exec();
}
