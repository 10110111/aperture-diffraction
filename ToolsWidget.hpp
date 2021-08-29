#pragma once

#include <QDockWidget>
#include "Manipulator.hpp"

class ToolsWidget : public QDockWidget
{
    Q_OBJECT

public:
    ToolsWidget(QWidget* parent=nullptr);

    double exposureCompensation() const { return exposureCompensation_->value(); }

signals:
    void settingChanged();

private:
    Manipulator* exposureCompensation_=nullptr;
};
