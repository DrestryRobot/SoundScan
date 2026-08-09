#include "mainwindow5.h"
#include "datadispatch.h"


MainWindow5 *MainWindow5::s_instance = nullptr;

namespace {
    DataProcessor *g_dataProcessor5 = nullptr;
    std::atomic<int> g_dataSeq3 { 0 };

    void dataPacketCallback(const char *data, int length, int deviceId)
    {
        QByteArray qba(data, length);
        delete[] data;

        if (MainWindow5::s_instance)
            MainWindow5::s_instance->onDataPacket(qba, deviceId);

        if (g_dataProcessor5) {
            int seq = g_dataSeq3.load();
            QMetaObject::invokeMethod(g_dataProcessor5, [=]() {
                if (seq == g_dataSeq3.load())
                    g_dataProcessor5->enqueueData(qba, deviceId);
            }, Qt::QueuedConnection);
        }
    }
}

MainWindow5::MainWindow5(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow5)
    , config(Client::getInstance())
{

    s_instance = this;
    ui->setupUi(this);

    setupChildWindow();

    // 初始化超声界面
    initWidget();

    // 初始化超声信号
    initSlot();

    m_dataProcessor->start();
}

MainWindow5::~MainWindow5()
{
    DataDispatch::removeProcessor(m_dataProcessor);
    DataDispatch::removeDirectSink(m_directSink);
    m_3dThread->quit();
    m_3dThread->wait();
    delete ui;
}

// 初始化超声界面
void MainWindow5::initWidget()
{
    m_processorThread = new QThread(this);
    m_processorThread->setObjectName(QStringLiteral("DataProcessorThread"));
    m_dataProcessor = new DataProcessor();
    m_dataProcessor->moveToThread(m_processorThread);
    g_dataProcessor5 = m_dataProcessor;
    m_processorThread->start();

    m_beamCount = config.getBeamCounts();

    QTimer::singleShot(10, this, [this]() {
        ColorManager::instance()->registerView(ui->View_1);
        ColorManager::instance()->registerMeasureWidget(ui->measure_widget);
    });

    ui->View_1->setScanView(ViewWidget::A_Scan);

    ui->View_2->setScanView(ViewWidget::E_Scan);
    ui->View_2->setColorPalette(ui->S_Scan_Color->getColors());

    ui->ruler_range->setDirections(Directions::Vertical_left);
    ui->ruler_range->setPosStart(0.0);
    ui->ruler_range->setPosEnd(50.0);

    double end = (config.getBeamCounts() - 1) * 0.6 * 1;

    ui->ruler_distance_s->setPosStart(0);
    ui->ruler_distance_s->setPosEnd(end);

    // // 创建3D视图
    m_3dView = new ViewModel();  // 不设置父对象
    // m_3dView->setParent(ui->widget);
    // m_3dView->setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
    // m_3dView->setAttribute(Qt::WA_DontCreateNativeAncestors);
    // m_3dView->setAttribute(Qt::WA_NativeWindow);

    // // 创建新布局
    // QVBoxLayout *layout = new QVBoxLayout(ui->widget);
    // layout->setContentsMargins(0, 0, 0, 0);
    // layout->setSpacing(0);
    // layout->addWidget(m_3dView);

    // // 设置大小策略
    // m_3dView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // // 强制更新
    // m_3dView->show();

    // 3D数据处理线程
    m_3dThread = new QThread(this);
    m_3dThread->setObjectName(QStringLiteral("3DViewThread"));
    m_3dWorker = new QObject();
    m_3dWorker->moveToThread(m_3dThread);
    connect(m_3dThread, &QThread::finished, m_3dWorker, &QObject::deleteLater);
    m_3dThread->start();
}

// 初始化超声信号
void MainWindow5::initSlot()
{
    config.setDataPacketCallback(DataDispatch::dataPacketCallback);

    QMetaObject::invokeMethod(m_dataProcessor, [=]() {
        DataDispatch::addProcessor(m_dataProcessor);
        m_dataProcessor->setViews(ui->View_1, ui->View_2, nullptr, nullptr, ui->measure_widget);
    }, Qt::QueuedConnection);

    m_directSink = DataDispatch::addDirectSink([this](const QByteArray &data, int deviceId) {
        onDataPacket(data, deviceId);
    });

    connect(App::getInstance(), &App::signal_GateView_Refresh, this,
            &MainWindow5::slot_rulerWidgetChanged);

    connect(App::getInstance(), &App::signal_probeChange, this,
            &MainWindow5::slot_rulerProbeChanged);

    connect(App::getInstance(), &App::signal_paletteChanged, this, [=] {
        ui->View_2->setColorPalette(ui->S_Scan_Color->getColors());
    });
}

void MainWindow5::setupChildWindow()
{
    if (!mainWindow7) {
        // ===== 创建 MainWindow7 =====
        mainWindow7 = new MainWindow7();

        // ===== 设置父窗口 =====
        mainWindow7->setParent(ui->widget);

        // ===== 关键：设置窗口标志 =====
        mainWindow7->setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);

        // ===== 关键：设置原生窗口属性 =====
        mainWindow7->setAttribute(Qt::WA_DontCreateNativeAncestors);
        mainWindow7->setAttribute(Qt::WA_NativeWindow);

        // ===== 布局，填满父窗口 =====
        QVBoxLayout *layout = new QVBoxLayout(ui->widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(mainWindow7);
        ui->widget->setLayout(layout);
    }

    mainWindow7->show();
}

// 扫描开始（快捷指令）
void MainWindow5::on_pushButton_29_clicked()
{
    mainWindow->on_pushButton_9();
}

// 扫描暂停（快捷指令）
void MainWindow5::on_pushButton_30_clicked()
{
    mainWindow->on_pushButton_19();
}

// 扫描结束（快捷指令）
void MainWindow5::on_pushButton_38_clicked()
{
    mainWindow->on_pushButton_20();
}

// 退出系统（快捷指令）
void MainWindow5::on_pushButton_34_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认退出", "确定要退出吗？",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QCoreApplication::exit(0);
    }
}

void MainWindow5::slot_rulerWidgetChanged()
{
    ui->ruler_range->setRulerUnit("mm");
    ui->ruler_range->setDirections(Directions::Vertical_left);
    ;
    ui->ruler_range->setPosStart(config.getRangeStart());
    ui->ruler_range->setPosEnd(config.getRangeEnd());
}

void MainWindow5::slot_rulerProbeChanged()
{
    auto num =
        config.getBeamLastElement() - config.getBeamFirstElement() - config.getBeamAperture() + 1;
    num = num / config.getBeamElementStep() + 1;
    double end = (num - 1) * config.getProbePrimaryElementsPitch() * config.getBeamElementStep();
    ui->ruler_distance_s->setPosStart(0);
    ui->ruler_distance_s->setPosEnd(end);
}

void MainWindow5::onDataPacket(const QByteArray &data, int deviceId)
{
    int beamCount = m_beamCount;
    QMetaObject::invokeMethod(m_3dWorker, [=]() {
        m_3dView->slot_Recive_date(beamCount, data);
    }, Qt::QueuedConnection);
}
