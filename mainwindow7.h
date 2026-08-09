#ifndef MAINWINDOW7_H
#define MAINWINDOW7_H

#pragma once

// ===== 解决 Windows min/max 宏冲突 =====
#ifndef NOMINMAX
#define NOMINMAX
#endif

// 取消 Windows 定义的 min/max 宏
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif


#include "mainwindow.h"
#include <QMainWindow>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkCamera.h>
#include <vtkLightKit.h>

#include <vtkPSphereSource.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

#include <vtkAxesActor.h>
#include <vtkCaptionActor2D.h>
#include <vtkTextProperty.h>

#include <vtkTransform.h>
#include <vtkGlyph3D.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPointData.h>

#include <QFileDialog>
#include <QMessageBox>

#include <QString>
#include <fstream>
#include <sstream>

#include <vtkPlaneSource.h>

#include <vtkPointPicker.h>
#include <vtkCallbackCommand.h>

#include <vtkPNGWriter.h>
#include <vtkWindowToImageFilter.h>

#include <vtkKdTree.h>
#include <vtkPropPicker.h>

#include <QProgressDialog>
#include <QPainter>

#include <vtkImageActor.h>
#include <vtkDoubleArray.h>
#include <vtkLookupTable.h>
#include <vtkImageMapper.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkImageData.h>
#include <vtkDoubleArray.h>
#include <vtkImageProperty.h>
#include <vtkStaticPointLocator.h>
#include <vtkImageThreshold.h>
#include <vtkImageMapToColors.h>
#include <vtkImageGaussianSmooth.h>

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkMaskPoints.h>
#include <vtkSurfaceReconstructionFilter.h>
#include <vtkContourFilter.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkDecimatePro.h>
#include <vtkPolyDataNormals.h>
#include <vtkStaticPointLocator.h>
#include <vtkUnsignedCharArray.h>

#include <vtkImageConvolve.h>

#include <QTimer>

#include <QMutex>

#include <QQueue>

// #include <vtkWebGPUComputePointCloudMapper.h>


// ===== 全局变量声明（在 mainwindow.cpp 中定义） =====
extern double amp[];
extern double tof[];
extern double si;
extern int beam;
extern double robot_x, robot_y, robot_z, robot_a, robot_b, robot_c;
extern quint32 robot_ipoc;
extern double longmen[];
extern bool start;

// 在类定义之前添加 DrawData 结构体
struct DrawData {
    std::array<double, 6> pose;
    std::array<double, 64> amp;
    std::array<double, 64> tof;
    double si;
    int beam;
    std::array<double, 2> longmen;
    quint32 ipoc;

    DrawData() : si(0.0), beam(0), ipoc(0) {
        pose.fill(0.0);
        amp.fill(0.0);
        tof.fill(0.0);
        longmen.fill(0.0);
    }
};

// mainwindow7.h

class DataCollector : public QObject
{
    Q_OBJECT

public:
    explicit DataCollector(QObject *parent = nullptr);
    ~DataCollector();

public slots:
    void run();
    void stop();
    void setVTKObjects(vtkSmartPointer<vtkRenderer> renderer,
                       vtkSmartPointer<vtkRenderWindow> renderWindow,
                       vtkSmartPointer<vtkPoints> points,
                       vtkSmartPointer<vtkCellArray> vertices,
                       vtkSmartPointer<vtkUnsignedCharArray> colors,
                       vtkSmartPointer<vtkPolyData> polyData,
                       vtkSmartPointer<vtkPolyDataMapper> mapper,
                       vtkSmartPointer<vtkActor> actor);

signals:
    void renderRequested();

private:
    void doDrawing(const DrawData &data);
    void addPoint(const DrawData &data);
    void GetColorFromValue(double value, unsigned char& r, unsigned char& g, unsigned char& b);

    // ===== 队列相关 =====
    QMutex m_queueMutex;
    QQueue<DrawData> m_dataQueue;
    bool m_isProcessing = false;

    // ===== 运行状态 =====
    bool m_running = false;
    quint32 m_lastIpoc = 0;
    bool m_firstRun = true;
    int m_frameCount = 0;
    int m_pointCount = 0;
    bool m_pointCloudInitialized = false;
    bool m_vtkInitialized = false;

    // ===== VTK对象 =====
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkPoints> m_points;
    vtkSmartPointer<vtkCellArray> m_vertices;
    vtkSmartPointer<vtkUnsignedCharArray> m_colors;
    vtkSmartPointer<vtkPolyData> m_polyData;
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_actor;
};

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow7;
}
QT_END_NAMESPACE

class MainWindow7 : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow7(QWidget *parent = nullptr);

    ~MainWindow7();

    void Drawing(double pose[], double amp[], double tof[], double si, int beam, double longmen[]);
protected:
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void on_pushButton_clicked();    // 开始绘制

    void on_pushButton_2_clicked();  // 停止绘制

    void on_pushButton_3_clicked();  // 结束绘制

    void on_pushButton_5_clicked();  // 保存数据

    void on_pushButton_6_clicked();  // 加载数据

    void on_pushButton_4_clicked();  // 重置数据

    void on_pushButton_12_clicked(); // 显示模式

    void on_pushButton_8_clicked();  // 数据模式

    void on_pushButton_10_clicked(); // 数据测量

    void on_pushButton_9_clicked();  // 对齐视点

    void on_pushButton_11_clicked(); // 窗口截图


private:
    Ui::MainWindow7 *ui;

    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkRenderWindow> renderWindow;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor;
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> style;
    vtkSmartPointer<vtkCamera> camera;
    vtkSmartPointer<vtkLightKit> lightKit;
    vtkSmartPointer<vtkAxesActor> axesActor;
    vtkSmartPointer<vtkPoints> points;
    vtkSmartPointer<vtkCellArray> vertices;
    vtkSmartPointer<vtkPolyData> polyData;
    vtkSmartPointer<vtkPolyDataMapper> mapper;
    // vtkSmartPointer<vtkWebGPUComputePointCloudMapper> mapper;  // 新 Mapper
    vtkSmartPointer<vtkActor> actor;
    vtkSmartPointer<vtkUnsignedCharArray> colors;
    vtkSmartPointer<vtkActor> point1Actor;
    vtkSmartPointer<vtkActor> point2Actor;

    // 显示模式
    bool isCloudMode = true;

    // 数据模式
    bool isAmpMode = true;
    std::vector<double> savedAmpValues;
    std::vector<double> savedTofValues;

    // 空间测量
    bool isMeasuring = false;
    double lastPoint[3];
    bool hasLastPoint = false;

    // ===== 网格相关成员变量 =====
    std::vector<double> gridX;
    std::vector<double> gridY;
    std::vector<double> gridAmpValues;
    std::vector<double> gridTofValues;

    bool gridStartSet = false;
    double gridOriginX = 0.0;
    double gridOriginY = 0.0;
    double gridDirection = 0.0;
    double gridStartZ = 0.0;

    double gridResolution = 0.1;
    static constexpr double GRID_RANGE = 4000.0;

    double gridMinX = 0.0, gridMaxX = 0.0;
    double gridMinY = 0.0, gridMaxY = 0.0;
    int gridDimX = 0, gridDimY = 0;

    bool isGridMode = false;
    bool isGridAmpMode = true;
    bool gridInitialized = false;

    vtkSmartPointer<vtkImageData> gridImage;
    vtkSmartPointer<vtkImageActor> gridActor;
    vtkSmartPointer<vtkLookupTable> gridLUT;
    vtkSmartPointer<vtkDoubleArray> gridScalars;

    bool isDrawing = false;

    bool enableInterpolation = true;  // 是否启用插值

    // ===== 子线程相关 =====
    QThread *m_collectorThread;
    DataCollector *m_collector;

    // ===== 统计 =====
    QTimer *m_statisticsTimer;
    int m_frameCount = 0;      // 接收到的帧数
    int m_drawCount = 0;       // 实际绘制的帧数
    int m_pointCount = 0;      // 点云点数

    bool m_isInteracting = false;  // 交互状态

    void initParam();      //初始化参数
    void initWidget();     // 初始化界面
    void initSlot();       // 初始化信号
    void initVTK();        // 初始化VTK
    void initPointCloud(); // 初始化点云
    void AddPoint(double pose[], double amp[], double tof[], double si, int beam);                   // 添加点云
    static void onMouseClick(vtkObject *obj, unsigned long event, void *clientData, void *callData); // 鼠标点击
    void pickPoint(int x, int y);   // 测量点
    void GetColorFromValue(double value, unsigned char &r, unsigned char &g, unsigned char &b);
    void computeAndStoreGridData(double pose[], double amp[], double tof[], double si, int beam);
    void setGridStartPoint(double x, double y, double angle);
    bool isPointInGrid(double x, double y);
    void addDataToGrid(double x, double y, double amp, double tof);
    void buildPixelGrid(bool skipInterpolation);
    void switchGridColorMode(bool ampMode);
    void onRenderRequested();
};
#endif // MAINWINDOW7_H
