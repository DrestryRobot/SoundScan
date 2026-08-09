#include "colorManager.h"
#include "dialog/viewwidget.h"
#include <QtGlobal>

ColorManager* ColorManager::instance()
{
    static ColorManager manager;
    return &manager;
}

ColorManager::ColorManager(QObject* parent)
    : QObject(parent)
    , m_globalBgColor(Qt::black)
    , m_globalLineColor(Qt::green)
{
}

ColorManager::~ColorManager()
{
}

void ColorManager::updateMeasureWidgetColor(QWidget* widget, const QColor &bgColor)
{
    if (!widget) return;

    // 计算亮度，决定字体颜色
    double brightness = 0.2126 * bgColor.red() +
                        0.7152 * bgColor.green() +
                        0.0722 * bgColor.blue();

    // 亮度 > 128 用黑色字，否则用白色字
    QColor textColor = (brightness > 128) ? Qt::black : Qt::white;

    // 创建半透明背景色（使用波形背景色，50%透明度）
    QColor semiTransparentBg = bgColor;
    semiTransparentBg.setAlpha(127);  // 127 = 50% 透明度

    // 设置样式：半透明背景 + 透明边框 + 字体颜色
    widget->setStyleSheet(QString("background-color: rgba(%1, %2, %3, 127); "
                                  "border: none; "
                                  "color: %4;")
                              .arg(semiTransparentBg.red())
                              .arg(semiTransparentBg.green())
                              .arg(semiTransparentBg.blue())
                              .arg(textColor.name()));
}

void ColorManager::registerMeasureWidget(QWidget* measureWidget)
{
    if (measureWidget && !m_measureWidgets.contains(measureWidget)) {
        m_measureWidgets.append(measureWidget);
        // 不管是否初始化，都立即用当前 m_globalBgColor 更新
        updateMeasureWidgetColor(measureWidget, m_globalBgColor);
    }
}

void ColorManager::unregisterMeasureWidget(QWidget* measureWidget)
{
    m_measureWidgets.removeAll(measureWidget);
}

void ColorManager::initFromUiColors(const QColor &bgColor, const QColor &lineColor)
{
    m_globalBgColor = bgColor;
    m_globalLineColor = lineColor;
    m_initialized = true;

    // 更新所有 ViewWidget
    for (ViewWidget* view : m_views) {
        if (view) {
            view->setChartColors(bgColor, lineColor);
        }
    }

    // 更新所有 measure_widget
    for (QWidget* widget : m_measureWidgets) {
        if (widget) {
            updateMeasureWidgetColor(widget, bgColor);
        }
    }

    // 更新所有 Axis
    for (ViewWidget* view : m_views) {
        if (view) {
            view->setChartColors(bgColor, lineColor);
            setAxisColors(view, bgColor);  // 新增：设置坐标轴颜色
        }
    }

    emit colorsChanged(bgColor, lineColor);
}

void ColorManager::registerView(ViewWidget* view)
{
    if (view && !m_views.contains(view)) {
        m_views.append(view);
        view->setChartColors(m_globalBgColor, m_globalLineColor);
        setAxisColors(view, m_globalBgColor);  // 新增：设置坐标轴颜色
    }
}

void ColorManager::unregisterView(ViewWidget* view)
{
    m_views.removeAll(view);
}

void ColorManager::setGlobalColors(const QColor &bgColor, const QColor &lineColor)
{
    if (m_globalBgColor == bgColor && m_globalLineColor == lineColor) {
        return;
    }

    m_globalBgColor = bgColor;
    m_globalLineColor = lineColor;
    m_initialized = true;

    // 更新所有 ViewWidget
    for (ViewWidget* view : m_views) {
        if (view) {
            view->setChartColors(bgColor, lineColor);
        }
    }

    // 更新所有 measure_widget
    for (QWidget* widget : m_measureWidgets) {
        if (widget) {
            updateMeasureWidgetColor(widget, bgColor);
        }
    }

    // 更新所有 Axis
    for (ViewWidget* view : m_views) {
        if (view) {
            view->setChartColors(bgColor, lineColor);
            setAxisColors(view, bgColor);  // 新增：设置坐标轴颜色
        }
    }

    emit colorsChanged(bgColor, lineColor);
}

// ViewWidget.cpp
void ColorManager::setAxisColors(ViewWidget* view, const QColor &bgColor)
{
    if (!view) return;

    // 计算亮度，决定坐标轴颜色
    double brightness = 0.2126 * bgColor.red() +
                        0.7152 * bgColor.green() +
                        0.0722 * bgColor.blue();
    QColor axisColor = (brightness > 128) ? QColor(100, 100, 100) : QColor(200, 200, 200);

    // 调用 ViewWidget 的方法设置坐标轴颜色
    view->setAxisColor(axisColor);
}