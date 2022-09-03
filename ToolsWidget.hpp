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
    double globalRotationAngle() const { return globalRotationAngle_->value(); }
    int pointCount() const { return pointCount_->value(); }
    int arcPointCount() const { return arcPointCount_->value(); }
    double curvatureRadius() const { return curvatureRadius_->value(); }
    int sampleCount() const { return sampleCount_->value(); }

signals:
    void settingChanged();

private:
    Manipulator* exposure_=nullptr;
    Manipulator* scale_=nullptr;
    Manipulator* globalRotationAngle_=nullptr;
    Manipulator* pointCount_=nullptr;
    Manipulator* arcPointCount_=nullptr;
    Manipulator* curvatureRadius_=nullptr;
    Manipulator* sampleCount_=nullptr;
};
