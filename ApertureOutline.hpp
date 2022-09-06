#pragma once

#include <QDockWidget>

class ToolsWidget;
class ApertureOutline : public QWidget
{
    Q_OBJECT

public:
    ApertureOutline(ToolsWidget* tools, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    ToolsWidget* tools_;
};
