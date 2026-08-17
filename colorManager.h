#ifndef COLORMANAGER_H
#define COLORMANAGER_H

#include <QObject>
#include <QColor>
#include <QList>
#include <QWidget>
#include <QGraphicsBlurEffect>

class ViewWidget;

class ColorManager : public QObject
{
    Q_OBJECT

public:
    static ColorManager* instance();

    // ViewWidget 相关
    void registerView(ViewWidget* view);
    void unregisterView(ViewWidget* view);

    // measure_widget 相关
    void registerMeasureWidget(QWidget* measureWidget);
    void unregisterMeasureWidget(QWidget* measureWidget);

    void setGlobalColors(const QColor &bgColor, const QColor &lineColor);
    void initFromUiColors(const QColor &bgColor, const QColor &lineColor);

    QColor globalBgColor() const { return m_globalBgColor; }
    QColor globalLineColor() const { return m_globalLineColor; }

signals:
    void colorsChanged(const QColor &bgColor, const QColor &lineColor);

private:
    explicit ColorManager(QObject* parent = nullptr);
    ~ColorManager();

    void updateMeasureWidgetColor(QWidget* widget, const QColor &bgColor);

    void setAxisColors(ViewWidget* view, const QColor &bgColor);

    QList<ViewWidget*> m_views;
    QList<QWidget*> m_measureWidgets;  // 存储所有 measure_widget
    QColor m_globalBgColor;
    QColor m_globalLineColor;
    bool m_initialized = false;
};

#endif // COLORMANAGER_H
