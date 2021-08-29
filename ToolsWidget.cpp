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

    exposureCompensation_ = addManipulator(layout, this, tr(u8"\u0394e&xposure"), -5, 5, 0, 2);

    layout->addStretch();
}
