#pragma once

#include <cmath>
#include <QDockWidget>
#include "Manipulator.hpp"

class QPushButton;
class ToolsWidget : public QDockWidget
{
    Q_OBJECT

public:
    ToolsWidget(QWidget* parent=nullptr);

    double exposure() const { return exposure_->value(); }
    double screenWidth() const { return screenWidth_->value(); }
    double globalRotationAngle() const { return globalRotationAngle_->value()*-std::acos(-1.)/180; }
    int pointCount() const { return pointCount_->value(); }
    int arcPointCount() const { return arcPointCount_->value(); }
    double apertureRadius() const { return apertureRadius_->value(); }
    double curvatureRadius() const { return curvatureRadius_->value(); }
    int sampleCount() const { return sampleCount_->value(); }
    int wavelengthCount() const { return wavelengthCount_->value(); }

signals:
    void settingChanged();
    void imageSavingRequest();

private:
    Manipulator* exposure_=nullptr;
    Manipulator* screenWidth_=nullptr;
    Manipulator* globalRotationAngle_=nullptr;
    Manipulator* pointCount_=nullptr;
    Manipulator* arcPointCount_=nullptr;
    Manipulator* apertureRadius_=nullptr;
    Manipulator* curvatureRadius_=nullptr;
    Manipulator* sampleCount_=nullptr;
    Manipulator* wavelengthCount_=nullptr;
    QPushButton* saveBtn_=nullptr;
};
