#pragma once

#include <QDockWidget>
#include "Manipulator.hpp"

class ToolsWidget : public QDockWidget
{
    Q_OBJECT

public:
    ToolsWidget(QWidget* parent=nullptr);

    double exposure() const { return exposure_->value(); }
    double scale() const { return scale_->value(); }

signals:
    void settingChanged();

private:
    Manipulator* exposure_=nullptr;
    Manipulator* scale_=nullptr;
};
