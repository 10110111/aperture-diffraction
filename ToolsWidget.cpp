#include "ToolsWidget.hpp"
#include "Manipulator.hpp"

static Manipulator* addManipulator(QVBoxLayout*const layout, ToolsWidget*const tools,
                                   QString const& label, const double min, const double max, const double defaultValue,
                                   const int decimalPlaces, QString const& unit="")
{
    const auto manipulator=new Manipulator(label, min, max, defaultValue, decimalPlaces);
    layout->addWidget(manipulator);
    tools->connect(manipulator, &Manipulator::valueChanged, tools, &ToolsWidget::settingChanged);
    if(!unit.isEmpty())
        manipulator->setUnit(unit);
    return manipulator;
}

ToolsWidget::ToolsWidget(QWidget* parent)
    : QDockWidget(tr("Tools"),parent)
{
    const auto mainWidget=new QWidget(this);
    const auto layout=new QVBoxLayout;
    mainWidget->setLayout(layout);
    setWidget(mainWidget);

    exposure_ = addManipulator(layout, this, tr(u8"log<sub>10</sub>e&xposure"), -8, 7, -2, 2);
    scale_    = addManipulator(layout, this, tr(u8"log<sub>10</sub>&scale"), -5, 1, -3, 2);
    globalRotationAngle_ = addManipulator(layout, this, tr(u8"&Rotation angle"), -90, 90, 14, 4, u8"°");
    pointCount_ = addManipulator(layout, this, tr(u8"Aperture ed&ge count"), 3, 99, 6, 0);
    arcPointCount_ = addManipulator(layout, this, tr(u8"&Points per arc (side)"), 0, 99, 25, 0);
    curvatureRadius_ = addManipulator(layout, this, tr(u8"Radius of &curvature of side"), 1, 50, 3, 2);
    sampleCount_ = addManipulator(layout, this, tr(u8"Sa&mples per pixel side"), 1, 19, 1, 0);
    wavelengthCount_ = addManipulator(layout, this, tr(u8"Number of &wavelengths"), 1, 9999, 256, 0);

    layout->addStretch();
}
