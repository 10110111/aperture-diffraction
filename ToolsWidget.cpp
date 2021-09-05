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

    exposure_ = addManipulator(layout, this, tr(u8"log<sub>10</sub>e&xposure"), -8, 7, 0, 2);
    scale_    = addManipulator(layout, this, tr(u8"log<sub>10</sub>&scale"), -5, 1, -2.5, 2);

    layout->addStretch();
}
