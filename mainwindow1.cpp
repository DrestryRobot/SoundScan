#include "mainwindow1.h"
#include "mainwindow2.h"
#include "ui_mainwindow1.h"
#include "Phaselink/datadispatch.h"
#include "3DScan/scandata.h"
#include "ads_poller.h"
#include <QPointer>
#include <QProcess>

ads_client adsClient;

extern bool tcp_ok;

extern double amp[], tof[], si;

extern int beam;

extern double robot_x, robot_y,robot_z,robot_a,robot_b,robot_c;

extern quint32 robot_ipoc;

double Thick = 0.0;

extern std::unordered_map<int, MaxAmplitude> indexToAmplitude;

extern std::unordered_map<MaxAmplitude, int> amplitudeToIndex;

static VideoFilter indexToVideoFilter[] = { VideoFilter::k1MHz, VideoFilter::k2MHz,
                                           VideoFilter::k3MHz, VideoFilter::k4MHz,
                                           VideoFilter::k5MHz, VideoFilter::k6MHz,
                                           VideoFilter::k7MHz, VideoFilter::kBypass };
static constexpr int videoFilterCount = sizeof(indexToVideoFilter) / sizeof(indexToVideoFilter[0]);

static int videoFilterToIndex(VideoFilter type)
{
    for (int i = 0; i < videoFilterCount; i++) {
        if (indexToVideoFilter[i] == type)
            return i;
    }
    return videoFilterCount - 1; // None
}

namespace {
    DataProcessor *g_dataProcessor = nullptr;
    std::atomic<int> g_dataSeq3 { 0 };

    void dataPacketCallback(const char *data, int length, int deviceId)
    {
        if (!g_dataProcessor)
            return;
        QByteArray qba(data, length);
        delete[] data;
        int seq = g_dataSeq3.load();
        QMetaObject::invokeMethod(g_dataProcessor, [=]() {
            if (seq == g_dataSeq3.load())
                g_dataProcessor->enqueueData(qba, deviceId);
        }, Qt::QueuedConnection);
    }
}

MainWindow1::MainWindow1(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow1)
    , config(Client::getInstance())
{
    // 初始化UI界面
    ui->setupUi(this);

    // 启用调试输出重定向
    DebugOutput* redirector = nullptr;
    redirector = new DebugOutput(ui->lineEdit_50, this);
    redirector->install();

    // 初始化超声界面
    initWidget();

    // 初始化超声信号
    initSlot();

    // 启动Delmia线程
    m_worker = new DelmiaWorker();
    m_worker->moveToThread(&m_workerThread);
    m_workerThread.start();

    // 初始化Delmia信号
    initDelmiaSlot();

    // 初始化Delmia状态
    initDelmiaStatus();

    // 加载点位数据
    loadPoseData();

    // 加载系统参数
    loadParameters();

    // 启动PLC的ADS通讯（后台线程，无硬件时不阻塞界面）
    m_adsThread = new QThread(this);
    m_adsThread->setObjectName(QStringLiteral("AdsConnectThread"));
    connect(m_adsThread, &QThread::started, [this]() {
        adsClient.AdsConnectRemote();
        m_adsThread->quit();
    });
    m_adsThread->start();

    // 启动KUKA的UDP通讯
    m_udpThread = new QThread(this);
    m_udpThread->setObjectName(QStringLiteral("UdpServerThread"));
    server = new UdpServer();
    server->moveToThread(m_udpThread);
    connect(m_udpThread, &QThread::started, [=]() { server->startListening(59153, "10.168.1.200"); });
    m_udpThread->start();

    // 启动KUKA的TCP通讯
    kuka = new TcpServer(this);
    kuka->startudp();

    // PLC 状态轮询移到后台线程，UI 线程只接收结果刷新控件，
    // 避免每 100ms 在 UI 线程做 20+ 次阻塞式 ADS 同步读导致界面卡顿。
    m_pollThread = new QThread(this);
    m_pollThread->setObjectName(QStringLiteral("AdsPollThread"));
    m_poller = new AdsStatusPoller();
    m_poller->moveToThread(m_pollThread);
    qRegisterMetaType<AdsStatusPoller::Status>();
    connect(m_pollThread, &QThread::finished, m_poller, &QObject::deleteLater);
    // 显式用 QueuedConnection：连接建立时两者都在主线程，AutoConnection 会误判为直连，
    // 导致 statusReady 在轮询线程直接调用 UI 槽（跨线程碰控件），界面不刷新。
    connect(m_poller, &AdsStatusPoller::statusReady, this,
            &MainWindow1::onRobotStatusReady, Qt::QueuedConnection);
    connect(m_pollThread, &QThread::started, m_poller, &AdsStatusPoller::start);
    m_pollThread->start();

    // 初始化3DChecker信号
    init3DCheckerSlot();

    // 启动3DChecker线程
    start3DChecker();

    // 初始化色彩模式
    initThemeSwitch();

    // 把四个 CheckBox 变成指示灯
    setupIndicatorFromCheckBox(ui->checkBox,   QColor(0, 255, 0));
    setupIndicatorFromCheckBox(ui->checkBox_2, QColor(0, 255, 0));
    setupIndicatorFromCheckBox(ui->checkBox_3, QColor(0, 255, 0));
    setupIndicatorFromCheckBox(ui->checkBox_4, QColor(0, 255, 0));

    // 在对话框初始化时添加
    ui->lineEdit_3->setPlaceholderText(tr("请输入新建的文件名称（*.CATPart, *.CATProduct）"));
    ui->lineEdit_2->setPlaceholderText(tr("显示新建和打开的文件路径"));
    ui->lineEdit_4->setPlaceholderText(tr("显示保存和关闭的文件路径"));

    // 恢复命令看门狗：暂停恢复偶发失效（PLC 不接受恢复命令、机器人不动）时，
    // 检测到机器人位姿未变化就按“先暂停8、再开始1”的可靠序列自动重发。
    m_resumeWatchdogTimer = new QTimer(this);
    m_resumeWatchdogTimer->setInterval(2000);
    connect(m_resumeWatchdogTimer, &QTimer::timeout, this, [this]() {
        if (!m_resumeWatchdogArmed) {
            m_resumeWatchdogTimer->stop();
            return;
        }
        // 位姿有变化说明机器人已恢复运动，关闭看门狗
        if (fabs(robot_x - m_resumeWx) > 1e-3 ||
            fabs(robot_y - m_resumeWy) > 1e-3 ||
            fabs(robot_z - m_resumeWz) > 1e-3) {
            m_resumeWatchdogArmed = false;
            m_resumeWatchdogTimer->stop();
            qDebug() << "[ScanCtrl] 恢复看门狗: 机器人已恢复运动";
            return;
        }
        if (++m_resumeWatchdogTries > 3) {
            m_resumeWatchdogArmed = false;
            m_resumeWatchdogTimer->stop();
            qWarning() << "[ScanCtrl] 恢复看门狗: 重试 3 次后机器人仍未动，请检查 PLC";
            return;
        }
        qDebug() << "[ScanCtrl] 恢复看门狗: 位姿未变化, 重发恢复命令 (try"
                 << m_resumeWatchdogTries << ")";
        adsClient.setIntVal(0x5E256, 8);   // 先回到 PLC 确认过的暂停态
        QTimer::singleShot(300, this, [this]() {
            if (!m_resumeWatchdogArmed) return;
            adsClient.setIntVal(0x5E256, 1);   // 重新发送开始命令
            m_resumeWx = robot_x;
            m_resumeWy = robot_y;
            m_resumeWz = robot_z;
        });
    });
}

MainWindow1::~MainWindow1()
{
    // 停止后台ADS连接线程
    if (m_adsThread) {
        m_adsThread->quit();
        m_adsThread->wait(3000);
    }
    if (m_pollThread) {
        if (m_poller) {
            QMetaObject::invokeMethod(m_poller, "stop", Qt::BlockingQueuedConnection);
        }
        m_pollThread->quit();
        m_pollThread->wait(3000);
    }

    // 退出系统走 QApplication::quit()，不会触发 closeEvent，
    // 这里统一停止所有子线程，避免 QThread 仍在运行时被父窗口析构
    // 触发 Qt 的 qFatal(abort)（0xC0000409 / Fatal program exit requested）。
    if (m_workerThread.isRunning()) {
        m_workerThread.quit();
        m_workerThread.wait(3000);
    }
    if (m_udpThread && m_udpThread->isRunning()) {
        if (server) {
            QMetaObject::invokeMethod(server, "stop", Qt::BlockingQueuedConnection);
        }
        m_udpThread->quit();
        m_udpThread->wait(3000);
    }
    if (m_processorThread && m_processorThread->isRunning()) {
        m_processorThread->quit();
        m_processorThread->wait(3000);
    }
    if (m_3dThread && m_3dThread->isRunning()) {
        m_isRunning = false;
        m_3dThread->quit();
        m_3dThread->wait(3000);
    }

    delete ui;
}

void MainWindow1::closeEvent(QCloseEvent *event)
{
    DataDispatch::removeProcessor(m_dataProcessor);

    // 删除数据处理
    config.startCapture(false);

    QThread::msleep(50);
    if (m_dataProcessor) {
        m_dataProcessor->stop();
        delete m_dataProcessor;
    }

    // 停止UDP线程
    if (m_udpThread) {
        QMetaObject::invokeMethod(server, "stop", Qt::BlockingQueuedConnection);
        m_udpThread->quit();
        m_udpThread->wait(3000);
        delete server;
        server = nullptr;
        delete m_udpThread;
        m_udpThread = nullptr;
    }

    // 停止Delmia线程
    m_workerThread.quit();
    m_workerThread.wait(3000);

    // 停止3DChecker线程
    if (m_3dThread && m_3dThread->isRunning()) {
        m_isRunning = false;
        m_3dThread->quit();
        m_3dThread->wait(3000);
    }

    tcp_ok = false;

    // 保存点位数据
    savePoseData(false);

    // 保存系统参数
    saveParameters();

    // 龙门电机失能
    MainWindow1::on_pushButton_28_clicked();

    // 扫描结束
    MainWindow1::on_pushButton_20_clicked();

    event->accept();
}

// 初始化超声界面
void MainWindow1::initWidget()
{
    m_processorThread = new QThread(this);
    m_processorThread->setObjectName(QStringLiteral("DataProcessorThread"));
    m_dataProcessor = new DataProcessor();
    m_dataProcessor->moveToThread(m_processorThread);
    g_dataProcessor = m_dataProcessor;
    m_processorThread->start();

    ui->View_1->onViewResized();

    // A扫
    ui->View_1->setScanView(ViewWidget::A_Scan);

    // 背景颜色
    ui->BgColorBox->clear();
    ui->BgColorBox->addItem("黑色", QColor(Qt::black));
    ui->BgColorBox->addItem("深色", QColor(30, 30, 30));
    ui->BgColorBox->addItem("蓝色", QColor(0, 10, 30));
    ui->BgColorBox->addItem("绿色", QColor(0, 30, 10));
    ui->BgColorBox->addItem("浅色", QColor(245, 245, 250));
    ui->BgColorBox->addItem("白色", QColor(Qt::white));
    ui->BgColorBox->setCurrentIndex(0);  // 黑色

    // 波形颜色
    ui->WaveColorBox->clear();
    ui->WaveColorBox->addItem("绿色", QColor(Qt::green));
    ui->WaveColorBox->addItem("黄色", QColor(Qt::yellow));
    ui->WaveColorBox->addItem("蓝色", QColor(32, 159, 223));
    ui->WaveColorBox->addItem("橙色", QColor(255, 100, 0));
    ui->WaveColorBox->addItem("红色", QColor(Qt::red));
    ui->WaveColorBox->setCurrentIndex(0);  // 默认荧光绿

    // 使用 QTimer 延迟读取
    QTimer::singleShot(10, this, [this]() {
        QColor initialBgColor = ui->BgColorBox->currentData().value<QColor>();
        QColor initialLineColor = ui->WaveColorBox->currentData().value<QColor>();
        ColorManager::instance()->initFromUiColors(initialBgColor, initialLineColor);

        ColorManager::instance()->registerView(ui->View_1);
        ColorManager::instance()->registerMeasureWidget(ui->measure_widget);
    });

    // 连接信号槽
    connect(ui->BgColorBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                QColor bgColor = ui->BgColorBox->itemData(index).value<QColor>();
                QColor lineColor = ui->WaveColorBox->currentData().value<QColor>();
                ColorManager::instance()->setGlobalColors(bgColor, lineColor);
            });

    connect(ui->WaveColorBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                QColor bgColor = ui->BgColorBox->currentData().value<QColor>();
                QColor lineColor = ui->WaveColorBox->itemData(index).value<QColor>();
                ColorManager::instance()->setGlobalColors(bgColor, lineColor);
            });

    ui->tableWidget->setFocusPolicy(Qt::NoFocus);
    ui->tableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(ui->tableWidget, &QTableWidget::cellChanged, this, &MainWindow1::onCellChanged);

    connect(App::getInstance(), &App::signal_tcgChanged, this, [this] {

        if (!config.getTcgEnable()) {
            tcg_enable = false;
            ui->TCG_ON_Btn->setChecked(false);
            ui->TCG_ON_Btn->setText(tr("TCG 关闭"));
        } else {
            tcg_enable = true;
            ui->TCG_ON_Btn->setChecked(true);
            ui->TCG_ON_Btn->setText(tr("TCG 开启"));
        }
        loadTcgPoints();
        refreshTabwidget();
    });

    refreshTabwidget();
}

// 初始化超声信号
void MainWindow1::initSlot()
{
    config.setDataPacketCallback(DataDispatch::dataPacketCallback);
    QMetaObject::invokeMethod(m_dataProcessor, [=]() {

        DataDispatch::addProcessor(m_dataProcessor);
        m_dataProcessor->setViews(ui->View_1, nullptr, nullptr, nullptr, ui->measure_widget);
    }, Qt::QueuedConnection);

    // 闸门按钮
    for (int i = 0; i < ui->GateBtnGroup->buttons().size(); i++) {
        ui->GateBtnGroup->setId(ui->GateBtnGroup->buttons().at(i), i);
        if (ui->GateBtnGroup->buttons().at(i)->isChecked()) {
            currentGate = static_cast<GATE>(i);
        }
    }

    ui->GateBtnGroup->addButton(ui->GateA_Btn, 0);
    ui->GateBtnGroup->addButton(ui->GateB_Btn, 1);
    ui->GateBtnGroup->addButton(ui->GateC_Btn, 2);
    ui->GateBtnGroup->addButton(ui->GateI_Btn, 3);
    ui->GateI_Btn->setChecked(true);

    connect(ui->GateBtnGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this,
            [=](QAbstractButton *Btn) {
                auto num = ui->GateBtnGroup->id(Btn);
                if (num == 0) {
                    currentGate = GATE::GATE_A;
                } else if (num == 1) {
                    currentGate = GATE::GATE_B;
                } else if (num == 2) {
                    currentGate = GATE::GATE_C;
                } else if (num == 3) {
                    currentGate = GATE::GATE_I;
                }
                getcurrentPara();
            });

    connect(App::getInstance(), &App::signal_Connected, this, &MainWindow1::refreshgroup);

    connect(App::getInstance(), &App::refresh_Allpara, this, &MainWindow1::getcurrentPara);

    connect(App::getInstance(), &App::signal_Connected, this, [this] {
        bool sync = config.getGateISyncSample();
        if (sync == false) {
            auto range_start = config.getRangeStart();
            if (range_start > config.getGateIStart()) {
                config.setRangeStart(range_start - config.getGateIStart());
            }
        }
    });

    // 连接服务
    config.setServerAddr("192.168.1.100", 0);

    {
        auto servers = config.getAllServers();
        bool found = false;
        for (auto &s : servers) {
            if (s.address == "192.168.1.100") {
                config.setCurrentServerId(s.serverId);
                found = true;
                break;
            }
        }
        if (!found)
            config.setCurrentServerId(0);
    }

    bool flag = config.Connect();

    if (flag) {
        IsConnect = ConfigState::Connected;

        QTimer::singleShot(0, this, [] {
            emit App::getInstance()->refresh_Allpara();
            emit App::getInstance()->signal_GateView_Refresh();
            emit App::getInstance()->signal_Connected();
            emit App::getInstance()->signal_tcgChanged();
        });
    } else {
        IsConnect = ConfigState::Failed;
    }

    // 开始扫查
    config.startCapture(true);

    m_dataProcessor->start();
}

void MainWindow1::getcurrentPara()
{
    // 基本
    ui->PRFSpinBox->blockSignals(true);
    ui->MamplitudeBox->blockSignals(true);
    ui->GainSpinBox->blockSignals(true);
    ui->RangstartSpinBox->blockSignals(true);
    ui->RangendSpinBox->blockSignals(true);
    ui->RectifierBox->blockSignals(true);
    ui->FilterHighBox->blockSignals(true);
    ui->FilterLowBox->blockSignals(true);
    ui->vedioFilterBox->blockSignals(true);
    ui->PaVoltageSpinBox->blockSignals(true);
    ui->ProbeFrequencySpinBox->blockSignals(true);
    ui->PwidthSpinBox->blockSignals(true);

    ui->PRFSpinBox->setValue(static_cast<double>(config.getFrameRate()));
    ui->MamplitudeBox->setCurrentIndex(amplitudeToIndex[config.getMaxAmplitude()]);
    ui->GainSpinBox->setValue(static_cast<double>(config.getGain()));
    ui->RangstartSpinBox->setValue(static_cast<double>(config.getRangeStart()));
    ui->RangendSpinBox->setValue(static_cast<double>(config.getRangeEnd()));
    ui->RectifierBox->setCurrentIndex(static_cast<int>(config.getRectifierMode()));
    if (config.getFilterHigh() == -1) { ui->FilterHighBox->setCurrentText(0); } else { ui->FilterHighBox->setCurrentText(QString::number(config.getFilterHigh())); }
    if (config.getFilterLow() == -1) { ui->FilterLowBox->setCurrentText(0); } else { ui->FilterLowBox->setCurrentText(QString::number(config.getFilterLow())); }
    ui->vedioFilterBox->setCurrentIndex(videoFilterToIndex(config.getVideoFilterMHz()));
    ui->PaVoltageSpinBox->setValue(static_cast<double>(config.getPaVoltage()));
    ui->ProbeFrequencySpinBox->setValue(static_cast<double>(config.getProbeFrequency()));
    ui->PwidthSpinBox->setValue(static_cast<double>(config.getPulseWidth()));

    ui->MamplitudeBox->blockSignals(false);
    ui->PRFSpinBox->blockSignals(false);
    ui->GainSpinBox->blockSignals(false);
    ui->RangstartSpinBox->blockSignals(false);
    ui->RangendSpinBox->blockSignals(false);
    ui->RectifierBox->blockSignals(false);
    ui->FilterHighBox->blockSignals(false);
    ui->FilterLowBox->blockSignals(false);
    ui->vedioFilterBox->blockSignals(false);
    ui->PaVoltageSpinBox->blockSignals(false);
    ui->ProbeFrequencySpinBox->blockSignals(false);
    ui->PwidthSpinBox->blockSignals(false);

    // 闸门
    ui->Btn_widget->blockSignals(true);
    ui->SyncBox->blockSignals(true);
    ui->StartSpinBox->blockSignals(true);
    ui->WidthSpinBox->blockSignals(true);
    ui->ThresholdSpinBox->blockSignals(true);
    ui->MeasureBox->blockSignals(true);
    ui->SynAcqisitBox->blockSignals(true);
    ui->Btn_left->blockSignals(true);
    ui->Btn_right->blockSignals(true);
    QListView *view = qobject_cast<QListView *>(ui->SyncBox->view());

    if (currentGate == GATE::GATE_I) {
        view->setRowHidden(3, true);
        view->setRowHidden(2, true);
        view->setRowHidden(1, true);

        if (config.getGateIEnable()) {
            ui->Btn_left->setChecked(true);
        } else {
            ui->Btn_right->setChecked(true);
        }
        ui->StartSpinBox->setValue(config.getGateIStart());
        ui->WidthSpinBox->setValue(config.getGateIEnd() - config.getGateIStart());
        auto threshold_I = config.getGateIThreshold();
        ui->ThresholdSpinBox->setValue(threshold_I);
        ui->MeasureBox->setCurrentIndex(static_cast<int>(config.getGateIMeasureType()));
        ui->SynAcqisitBox->show();
        ui->SynAcqisitlabel->show();

        ui->SynAcqisitBox->setCurrentIndex(config.getGateISyncSample());
        ui->SyncBox->setCurrentIndex(0);

    } else if (currentGate == GATE::GATE_A) {
        view->setRowHidden(3, true);
        view->setRowHidden(2, true);
        view->setRowHidden(1, false);

        // 同步模式加载：如果同步目标未使能，回退到Pulser
        auto syncA = config.getGateASynchronMode();
        if (syncA == GateSynchron::GateI && !config.getGateIEnable()) {
            syncA = GateSynchron::Pulser;
            config.setGateASynchronMode(GateSynchron::Pulser);
        }
        // 闸门A的SyncBox只有Pulser(0)和GateI(1)，确保index合法
        ui->SyncBox->setCurrentIndex(static_cast<int>(syncA) <= 1 ? static_cast<int>(syncA) : 0);
        ui->StartSpinBox->setValue(config.getGateAStart());

        if (config.getGateAEnable()) {
            ui->Btn_left->setChecked(true);
        } else {
            ui->Btn_right->setChecked(true);
        }

        ui->WidthSpinBox->setValue(config.getGateAEnd() - config.getGateAStart());
        auto threshold_A = config.getGateAThreshold();
        ui->ThresholdSpinBox->setValue(threshold_A);
        ui->MeasureBox->setCurrentIndex(static_cast<int>(config.getGateAMeasureType()));

        ui->SynAcqisitBox->hide();
        ui->SynAcqisitlabel->hide();

    } else if (currentGate == GATE::GATE_B) {
        view->setRowHidden(2, false);
        view->setRowHidden(1, false);
        view->setRowHidden(3, true);

        // 同步模式加载：如果同步目标未使能，回退到Pulser
        auto syncB = config.getGateBSynchronMode();
        if (syncB == GateSynchron::GateI && !config.getGateIEnable()) {
            syncB = GateSynchron::Pulser;
            config.setGateBSynchronMode(GateSynchron::Pulser);
        } else if (syncB == GateSynchron::GateA && !config.getGateAEnable()) {
            syncB = GateSynchron::Pulser;
            config.setGateBSynchronMode(GateSynchron::Pulser);
        }
        ui->SyncBox->setCurrentIndex(static_cast<int>(syncB));

        ui->StartSpinBox->setValue(config.getGateBStart());
        if (config.getGateBEnable()) {
            ui->Btn_left->setChecked(true);
        } else {
            ui->Btn_right->setChecked(true);
        }

        ui->WidthSpinBox->setValue(config.getGateBEnd() - config.getGateBStart());
        auto threshold_B = config.getGateBThreshold();
        ui->ThresholdSpinBox->setValue(threshold_B);
        ui->MeasureBox->setCurrentIndex(static_cast<int>(config.getGateBMeasureType()));
        ui->SynAcqisitBox->hide();
        ui->SynAcqisitlabel->hide();

    } else if (currentGate == GATE::GATE_C) {
        view->setRowHidden(2, false);
        view->setRowHidden(1, false);
        view->setRowHidden(3, false);

        auto syncC = config.getGateCSynchronMode();
        if (syncC == GateSynchron::GateI && !config.getGateIEnable()) {
            syncC = GateSynchron::Pulser;
            config.setGateCSynchronMode(GateSynchron::Pulser);
        } else if (syncC == GateSynchron::GateA && !config.getGateAEnable()) {
            syncC = GateSynchron::Pulser;
            config.setGateCSynchronMode(GateSynchron::Pulser);
        } else if (syncC == GateSynchron::GateB && !config.getGateBEnable()) {
            syncC = GateSynchron::Pulser;
            config.setGateCSynchronMode(GateSynchron::Pulser);
        }
        ui->SyncBox->setCurrentIndex(static_cast<int>(syncC));

        ui->StartSpinBox->setValue(config.getGateCStart());
        if (config.getGateCEnable()) {
            ui->Btn_left->setChecked(true);
        } else {
            ui->Btn_right->setChecked(true);
        }

        ui->WidthSpinBox->setValue(config.getGateCEnd() - config.getGateCStart());
        auto threshold_C = config.getGateCThreshold();
        ui->ThresholdSpinBox->setValue(threshold_C);
        ui->MeasureBox->setCurrentIndex(static_cast<int>(config.getGateCMeasureType()));
        ui->SynAcqisitBox->hide();
        ui->SynAcqisitlabel->hide();
    }

    ui->Btn_widget->blockSignals(false);
    ui->SyncBox->blockSignals(false);
    ui->StartSpinBox->blockSignals(false);
    ui->WidthSpinBox->blockSignals(false);
    ui->ThresholdSpinBox->blockSignals(false);
    ui->MeasureBox->blockSignals(false);
    ui->SynAcqisitBox->blockSignals(false);
    ui->Btn_left->blockSignals(false);
    ui->Btn_right->blockSignals(false);
}

void MainWindow1::setBtnchecked(bool check)
{
    if (check) {
        ui->Btn_left->click();
    } else {
        ui->Btn_right->click();
    }
}

// 初始化仿真状态
void MainWindow1::initDelmiaStatus()
{
    QMetaObject::invokeMethod(m_worker, "doGetCurrentDocumentInfo", Qt::QueuedConnection);
}

// 初始化仿真信号
void MainWindow1::initDelmiaSlot()
{
    // 连接信号
    connect(m_worker, &DelmiaWorker::currentDocumentInfo, this, [this](const QString &text) {
        ui->lineEdit_2->setText(text);
    });
    connect(m_worker, &DelmiaWorker::newFileResult, this, [this](const QString &docName) {
        ui->lineEdit_2->setText(docName + " (未保存)");
    });
    connect(m_worker, &DelmiaWorker::openFileResult, this, [this](const QString &path) {
        ui->lineEdit_2->setText(QDir::toNativeSeparators(path));
    });
    connect(m_worker, &DelmiaWorker::saveFileResult, this, [this](const QString &path) {
        ui->lineEdit_4->setText(path);
    });
    connect(m_worker, &DelmiaWorker::closeFileResult, this, [this](const QString &path) {
        if (path.isEmpty()) {
            ui->lineEdit_2->clear();
            ui->lineEdit_4->clear();
        } else {
            ui->lineEdit_2->setText(path);
        }
    });
    connect(m_worker, &DelmiaWorker::exitResult, this, [this]() {
        ui->lineEdit_2->clear();
        ui->lineEdit_3->clear();
        ui->lineEdit_4->clear();
    });
    connect(m_worker, &DelmiaWorker::closeFileInfo, this, [this](const QString &closedText, const QString &currentText) {
        ui->lineEdit_4->setText(closedText);
        if (currentText.isEmpty()) {
            ui->lineEdit_2->clear();
            ui->lineEdit_4->clear();
        } else {
            ui->lineEdit_2->setText(currentText);
        }
    });
    connect(m_worker, &DelmiaWorker::requestSaveFileDialog, this, [this](const QString &defaultPath, const QString &filter) {
        QString savePath = QFileDialog::getSaveFileName(
            this,
            tr("保存文件"),
            defaultPath,
            filter
            );

        if (!savePath.isEmpty()) {
            QFileInfo fileInfo(savePath);
            QSettings settings("HeWenTech", "CATIAAutomationTool");
            settings.setValue("LastSavePath", fileInfo.absolutePath());

            QMetaObject::invokeMethod(m_worker, "doSaveFile",
                                      Qt::QueuedConnection, Q_ARG(QString, QDir::toNativeSeparators(savePath)));
        }
    });

    // 1. 文件创建完成信号 - 更新四个点
    connect(m_worker, &DelmiaWorker::projectFilesCreated,
            this, [this](const QString &projectName, const QString &partZBDocName) {
                qDebug() << "[MainWindow1] 收到文件创建完成信号，开始更新四个点";

                // 计算并更新 ZB Part 的四个点
                if (!calculateAndUpdateZBFourPoints(projectName)) {
                    QMessageBox::warning(this, tr("警告"), tr("标定点位计算失败，请检查输入参数"));
                }
            });

    // 2. 项目创建完成信号 - 显示成功消息
    connect(m_worker, &DelmiaWorker::projectCreated, this,
            [this](bool success, const QString &message) {
                if (success) {
                    QMessageBox::information(this, tr("成功"), message);
                } else {
                    QMessageBox::warning(this, tr("错误"), message);
                }
                ui->lineEdit_3->clear();
            });
}

// 初始化色彩模式
void MainWindow1::initThemeSwitch() {
    // 连接信号槽（ComboBox 选项已在 UI 中设置）
    connect(ui->ColorCombox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                if (index == 0) {
                    applyDarkTheme();  // 索引0对应深色
                } else {
                    applyLightTheme(); // 索引1对应浅色
                }

                // 保存用户选择
                QSettings settings;
                settings.setValue("theme", index);
            });

    // 读取保存的主题设置
    QSettings settings;
    int themeIndex = settings.value("theme", 0).toInt();
    ui->ColorCombox->setCurrentIndex(themeIndex);

    // 应用初始主题
    if (themeIndex == 0) {
        applyDarkTheme();  // 索引0对应深色
    } else {
        applyLightTheme(); // 索引1对应浅色
    }
}

// 后台 PLC 轮询结果刷新 UI（阻塞式 ADS 读取已移到 AdsStatusPoller 线程）
void MainWindow1::onRobotStatusReady(const AdsStatusPoller::Status &s)
{
    // 临时诊断：确认 UI 线程收到了轮询结果
    static bool firstCall = true;
    if (firstCall) {
        firstCall = false;
        qDebug() << "[AdsPoll] UI slot first call, xPos=" << s.xPos << " progNo=" << s.progNo;
    }

    // 值未变化时不刷新控件，减少 UI 重绘
    auto setLabelText = [](QLabel *label, const QString &text) {
        if (label->text() != text)
            label->setText(text);
    };

    // 机器人位姿
    setLabelText(ui->label_17, QString::number(s.robotX, 'f', 0));
    setLabelText(ui->label_18, QString::number(s.robotY, 'f', 0));
    setLabelText(ui->label_19, QString::number(s.robotZ, 'f', 0));
    setLabelText(ui->label_20, QString::number(s.robotA, 'f', 0));
    setLabelText(ui->label_21, QString::number(s.robotB, 'f', 0));
    setLabelText(ui->label_22, QString::number(s.robotC, 'f', 0));

    // 龙门状态（使能状态）
    if (ui->checkBox->isChecked() != s.xEnable)
        ui->checkBox->setChecked(s.xEnable);
    if (ui->checkBox_2->isChecked() != s.yEnable)
        ui->checkBox_2->setChecked(s.yEnable);

    // 龙门状态（实时位置 实时速度）
    setLabelText(ui->label_37, QString::number(s.xPos, 'f', 0));
    QString xVelText = QString::number(s.xVel, 'f', 0);
    if (!xVelText.toDouble())
        xVelText = "0";
    setLabelText(ui->label_39, xVelText);
    setLabelText(ui->label_38, QString::number(s.yPos, 'f', 0));
    QString yVelText = QString::number(s.yVel, 'f', 0);
    if (!yVelText.toDouble())
        yVelText = "0";
    setLabelText(ui->label_40, yVelText);

    // 龙门运行到位
    x_daowei = !(s.xMove1 || s.xMove2 || s.xMove3 || s.xMove4 || x_flag);
    y_daowei = !(s.yMove1 || s.yMove2 || s.yMove3 || s.yMove4 || y_flag);
    if (ui->checkBox_3->isChecked() != x_daowei)
        ui->checkBox_3->setChecked(x_daowei);
    if (ui->checkBox_4->isChecked() != y_daowei)
        ui->checkBox_4->setChecked(y_daowei);

    // 当前程序号
    setLabelText(ui->label_12, QString::number(s.progNo));

    // 机器人模式
    if (s.modeT1)
        setLabelText(ui->label_13, "T1");
    if (s.modeT2)
        setLabelText(ui->label_13, "T2");
    if (s.modeAUT)
        setLabelText(ui->label_13, "AUT");
    if (s.modeEXT)
        setLabelText(ui->label_13, "EXT");
}

// 保存系统参数
void MainWindow1::saveParameters()
{
    // 在这里设置
    QCoreApplication::setOrganizationName("MyCompany");
    QCoreApplication::setApplicationName("MyApp");

    QSettings settings;
    settings.beginGroup("Parameters");
    settings.remove("");  // 清空旧数据

    // 保存当前选中的标签页索引
    settings.setValue("LastTabIndex", ui->tabWidget->currentIndex());

    // 系统参数
    settings.setValue("doubleSpinBox_6", ui->doubleSpinBox_6->text());
    settings.setValue("doubleSpinBox_7", ui->doubleSpinBox_7->text());
    settings.setValue("doubleSpinBox_8", ui->doubleSpinBox_8->text());
    settings.setValue("doubleSpinBox_9", ui->doubleSpinBox_9->text());
    settings.setValue("doubleSpinBox_10", ui->doubleSpinBox_10->text());
    settings.setValue("doubleSpinBox_11", ui->doubleSpinBox_11->text());
    settings.setValue("doubleSpinBox_12", ui->doubleSpinBox_12->text());
    settings.setValue("ColorCombox", ui->ColorCombox->currentIndex());
    settings.setValue("BgColorBox", ui->BgColorBox->currentIndex());
    settings.setValue("WaveColorBox", ui->WaveColorBox->currentIndex());

    // 运动控制参数
    settings.setValue("doubleSpinBox_4", ui->doubleSpinBox_4->text());
    settings.setValue("doubleSpinBox_5", ui->doubleSpinBox_5->text());
    settings.setValue("doubleSpinBox_3", ui->doubleSpinBox_3->text());
    settings.setValue("doubleSpinBox_2", ui->doubleSpinBox_2->text());
    settings.setValue("doubleSpinBox", ui->doubleSpinBox->text());
    settings.setValue("comboBox", ui->comboBox->currentIndex());
    settings.setValue("comboBox_2", ui->comboBox_2->currentIndex());

    settings.endGroup();
}

// 加载系统参数
void MainWindow1::loadParameters()
{
    // 在这里设置
    QCoreApplication::setOrganizationName("MyCompany");
    QCoreApplication::setApplicationName("MyApp");

    QSettings settings;
    settings.beginGroup("Parameters");

    // 恢复上次选中的标签页
    int lastTabIndex = settings.value("LastTabIndex", 0).toInt();
    if (lastTabIndex >= 0 && lastTabIndex < ui->tabWidget->count()) {
        ui->tabWidget->setCurrentIndex(lastTabIndex);
    }

    ui->doubleSpinBox_6->setValue(settings.value("doubleSpinBox_6").toDouble());
    ui->doubleSpinBox_7->setValue(settings.value("doubleSpinBox_7").toDouble());
    ui->doubleSpinBox_8->setValue(settings.value("doubleSpinBox_8").toDouble());
    ui->doubleSpinBox_9->setValue(settings.value("doubleSpinBox_9").toDouble());
    ui->doubleSpinBox_10->setValue(settings.value("doubleSpinBox_10").toDouble());
    ui->doubleSpinBox_11->setValue(settings.value("doubleSpinBox_11").toDouble());
    ui->doubleSpinBox_12->setValue(settings.value("doubleSpinBox_12").toDouble());
    if (settings.contains("ColorCombox")) ui->ColorCombox->setCurrentIndex(settings.value("ColorCombox").toInt());
    if (settings.contains("BgColorBox")) ui->BgColorBox->setCurrentIndex(settings.value("BgColorBox").toInt());
    if (settings.contains("WaveColorBox")) ui->WaveColorBox->setCurrentIndex(settings.value("WaveColorBox").toInt());

    // 运动控制参数
    ui->doubleSpinBox_4->setValue(settings.value("doubleSpinBox_4").toDouble());
    if(!settings.value("doubleSpinBox_4").toDouble()) ui->doubleSpinBox_4->clear();
    ui->doubleSpinBox_5->setValue(settings.value("doubleSpinBox_5").toDouble());
    if(!settings.value("doubleSpinBox_5").toDouble()) ui->doubleSpinBox_5->clear();
    ui->doubleSpinBox_3->setValue(settings.value("doubleSpinBox_3").toDouble());
    ui->doubleSpinBox_2->setValue(settings.value("doubleSpinBox_2").toDouble());
    ui->doubleSpinBox->setValue(settings.value("doubleSpinBox").toDouble());
    if (settings.contains("comboBox")) ui->comboBox->setCurrentIndex(settings.value("comboBox").toInt());
    if (settings.contains("comboBox_2")) ui->comboBox_2->setCurrentIndex(settings.value("comboBox_2").toInt());

    settings.endGroup();
}

// 保存点位参数
void MainWindow1::savePoseData(bool showPopup)
{
    struct DataGroup {
        double X, Y;
        double A[6];
    };

    QVector<DataGroup> dataGroups;

    // 字段名列表
    QStringList xFields = { "lineEdit_17", "lineEdit_19", "lineEdit_20", "lineEdit_21" };
    QStringList yFields = { "lineEdit_22", "lineEdit_23", "lineEdit_24", "lineEdit_25" };
    QList<QStringList> aFields = {
        { "lineEdit_26", "lineEdit_30", "lineEdit_34", "lineEdit_38", "lineEdit_42", "lineEdit_46" },
        { "lineEdit_27", "lineEdit_31", "lineEdit_35", "lineEdit_39", "lineEdit_43", "lineEdit_47" },
        { "lineEdit_28", "lineEdit_32", "lineEdit_36", "lineEdit_40", "lineEdit_44", "lineEdit_48" },
        { "lineEdit_29", "lineEdit_33", "lineEdit_37", "lineEdit_41", "lineEdit_45", "lineEdit_49" }
    };

    // 读取界面数据
    for (int i = 0; i < 4; ++i) {
        DataGroup group;
        group.X = findChild<QLineEdit*>(xFields[i])->text().toDouble();
        group.Y = findChild<QLineEdit*>(yFields[i])->text().toDouble();
        for (int j = 0; j < 6; ++j) {
            group.A[j] = findChild<QLineEdit*>(aFields[i][j])->text().toDouble();
        }
        dataGroups.append(group);
    }

    // ===== 新增：检查所有数据是否都为0，如果是则静默跳过 =====
    bool allZero = true;
    for (const auto& g : dataGroups) {
        if (g.X != 0.0 || g.Y != 0.0) {
            allZero = false;
            break;
        }
        for (int j = 0; j < 6; ++j) {
            if (g.A[j] != 0.0) {
                allZero = false;
                break;
            }
        }
        if (!allZero) break;
    }
    if (allZero) {
        return;  // 静默返回，不提示
    }

    // 生成当前数据字符串（用于比对）
    QString currentDataStr;
    QTextStream currStream(&currentDataStr);
    for (int i = 0; i < dataGroups.size(); ++i) {
        const auto& g = dataGroups[i];
        currStream << "Group" << (i + 1) << " < "
                   << "X=" << QString::number(g.X, 'f', 2)
                   << " Y=" << QString::number(g.Y, 'f', 2);
        for (int j = 0; j < 6; ++j)
            currStream << " A" << (j + 1) << "=" << QString::number(g.A[j], 'f', 2);
        currStream << " > ";
    }
    currentDataStr = currentDataStr.trimmed();

    // 生成文件路径
    QString currentDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString dailyPosePath = QString("%1/点位数据%2.txt").arg(QFileInfo(posePath).path(), currentDate);
    QDir().mkpath(QFileInfo(dailyPosePath).path());

    // 读取并处理历史数据
    QFile file(dailyPosePath);
    QStringList keptRecords;
    bool hasDuplicate = false;
    bool firstDuplicateFound = false;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        // 按空行分割记录
        QStringList records = content.split("\n\n", Qt::SkipEmptyParts);

        // 遍历所有历史记录，检查是否有与当前数据相同的
        for (const QString& record : records) {
            int pos = record.indexOf(" >> ");
            if (pos != -1) {
                QString recordData = record.mid(pos + 4).trimmed();
                if (recordData == currentDataStr) {
                    hasDuplicate = true;
                    if (!firstDuplicateFound) {
                        // 第一次发现重复，保留这条记录（最早的数据）
                        firstDuplicateFound = true;
                        keptRecords.append(record);
                    }
                    // 后续的重复记录不保留（删除）
                    continue;
                }
            }
            // 保留不重复的记录
            keptRecords.append(record);
        }
    }

    if (hasDuplicate) {
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            // 写入保留的记录
            for (int i = 0; i < keptRecords.size(); ++i) {
                out << keptRecords[i];
                if (i < keptRecords.size() - 1) {
                    out << "\n\n";
                }
            }
            file.close();
            if (showPopup) {
                QMessageBox::information(this, "跳过保存", "数据已存在，已跳过保存");
            }
            return;
        }
    }

    // 没有重复数据，正常追加保存
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        // 如果文件已有内容，先加换行
        if (file.size() > 0) {
            out << "\n\n";
        }
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " >> "
            << currentDataStr << "\n\n";
        file.close();

        qDebug() << QString("点位数据已保存到：%1").arg(dailyPosePath);

        if (showPopup) {
            QMessageBox::information(this, "保存成功", "点位数据保存成功");
        }
    } else if (showPopup) {
        QMessageBox::warning(this, "保存失败", dailyPosePath + "不存在！");
    }
}

// 加载点位参数
void MainWindow1::loadPoseData()
{
    // 获取今天的文件路径
    QString currentDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString dailyPosePath = QString("%1/点位数据%2.txt").arg(QFileInfo(posePath).path()).arg(currentDate);

    // 如果今天的文件不存在，则查找最近一次的文件
    QString fileToLoad;
    if (QFile::exists(dailyPosePath)) {
        fileToLoad = dailyPosePath;
    } else {
        // 获取目录下所有点位数据文件
        QDir dir(QFileInfo(posePath).path());
        QStringList filters;
        filters << "点位数据*.txt";
        QStringList files = dir.entryList(filters, QDir::Files, QDir::Time); // 按时间排序（最新的在前）

        if (files.isEmpty()) {
            qDebug() << "未找到任何点位数据文件";
            return;
        }

        // 取最新的文件（files[0]）
        fileToLoad = dir.absoluteFilePath(files[0]);
        qDebug() << "今天的点位文件不存在，加载最近的文件：" << fileToLoad;
    }

    QFile file(fileToLoad);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    QString lastLine;

    // 匹配以时间戳开头的行，例如 "2025-09-27 15:34:29 >>"
    QRegularExpression timestampRegex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} >>)");

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (timestampRegex.match(line).hasMatch()) {
            lastLine = line;
        }
    }

    file.close();
    if (lastLine.isEmpty()) return;

    // 去除时间戳部分
    int arrowIndex = lastLine.indexOf(">>");
    if (arrowIndex == -1) return;
    QString dataPart = lastLine.mid(arrowIndex + 2).trimmed();

    // 拆分为4组数据（按 > 分隔）
    QStringList groupParts = dataPart.split(">", Qt::SkipEmptyParts);
    if (groupParts.size() != 4) return;

    // 控件名列表
    QStringList xFields = { "lineEdit_17", "lineEdit_19", "lineEdit_20", "lineEdit_21" };
    QStringList yFields = { "lineEdit_22", "lineEdit_23", "lineEdit_24", "lineEdit_25" };

    QList<QStringList> aFields = {
        { "lineEdit_26", "lineEdit_30", "lineEdit_34", "lineEdit_38", "lineEdit_42", "lineEdit_46" },
        { "lineEdit_27", "lineEdit_31", "lineEdit_35", "lineEdit_39", "lineEdit_43", "lineEdit_47" },
        { "lineEdit_28", "lineEdit_32", "lineEdit_36", "lineEdit_40", "lineEdit_44", "lineEdit_48" },
        { "lineEdit_29", "lineEdit_33", "lineEdit_37", "lineEdit_41", "lineEdit_45", "lineEdit_49" }
    };

    for (int groupIndex = 0; groupIndex < 4; ++groupIndex) {
        QString groupStr = groupParts[groupIndex].trimmed();

        // 去掉 "GroupN <" 前缀和末尾 ">"
        int ltIndex = groupStr.indexOf("<");
        if (ltIndex == -1) continue;
        groupStr = groupStr.mid(ltIndex + 1).trimmed();

        // 拆分字段
        QStringList fields = groupStr.split(" ", Qt::SkipEmptyParts);
        if (fields.size() < 8) continue;

        QString xStr = fields[0].split("=")[1];
        QString yStr = fields[1].split("=")[1];

        QLineEdit* xEdit = this->findChild<QLineEdit*>(xFields[groupIndex]);
        QLineEdit* yEdit = this->findChild<QLineEdit*>(yFields[groupIndex]);
        if (xEdit) xEdit->setText(xStr);
        if (yEdit) yEdit->setText(yStr);

        for (int aIndex = 0; aIndex < 6; ++aIndex) {
            QString val = fields[2 + aIndex].split("=")[1];
            QLineEdit* aEdit = this->findChild<QLineEdit*>(aFields[groupIndex][aIndex]);
            if (aEdit) aEdit->setText(val);
        }
    }
}

// // 加载点位参数
// void MainWindow1::loadPoseData()
// {
//     // 获取今天和昨天的文件路径
//     QString currentDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
//     QString yesterdayDate = QDateTime::currentDateTime().addDays(-1).toString("yyyy-MM-dd");

//     QString dailyPosePath = QString("%1/点位数据%2.txt").arg(QFileInfo(posePath).path()).arg(currentDate);
//     QString yesterdayPosePath = QString("%1/点位数据%2.txt").arg(QFileInfo(posePath).path()).arg(yesterdayDate);

//     // 优先加载今天的文件，如果不存在则尝试加载昨天的文件
//     QString fileToLoad;
//     if (QFile::exists(dailyPosePath)) {
//         fileToLoad = dailyPosePath;
//     } else if (QFile::exists(yesterdayPosePath)) {
//         fileToLoad = yesterdayPosePath;
//     } else {
//         qDebug() << "未找到今天的点位文件：" << dailyPosePath;
//         return;
//     }

//     QFile file(fileToLoad);
//     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

//     QTextStream in(&file);
//     QString lastLine;

//     // 匹配以时间戳开头的行，例如 "2025-09-27 15:34:29 >>"
//     QRegularExpression timestampRegex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} >>)");

//     while (!in.atEnd()) {
//         QString line = in.readLine().trimmed();
//         if (timestampRegex.match(line).hasMatch()) {
//             lastLine = line;
//         }
//     }

//     file.close();
//     if (lastLine.isEmpty()) return;

//     // 去除时间戳部分
//     int arrowIndex = lastLine.indexOf(">>");
//     if (arrowIndex == -1) return;
//     QString dataPart = lastLine.mid(arrowIndex + 2).trimmed();

//     // 拆分为4组数据（按 > 分隔）
//     QStringList groupParts = dataPart.split(">", Qt::SkipEmptyParts);
//     if (groupParts.size() != 4) return;

//     // 控件名列表
//     QStringList xFields = { "lineEdit_17", "lineEdit_19", "lineEdit_20", "lineEdit_21" };
//     QStringList yFields = { "lineEdit_22", "lineEdit_23", "lineEdit_24", "lineEdit_25" };

//     QList<QStringList> aFields = {
//         { "lineEdit_26", "lineEdit_30", "lineEdit_34", "lineEdit_38", "lineEdit_42", "lineEdit_46" },
//         { "lineEdit_27", "lineEdit_31", "lineEdit_35", "lineEdit_39", "lineEdit_43", "lineEdit_47" },
//         { "lineEdit_28", "lineEdit_32", "lineEdit_36", "lineEdit_40", "lineEdit_44", "lineEdit_48" },
//         { "lineEdit_29", "lineEdit_33", "lineEdit_37", "lineEdit_41", "lineEdit_45", "lineEdit_49" }
//     };

//     for (int groupIndex = 0; groupIndex < 4; ++groupIndex) {
//         QString groupStr = groupParts[groupIndex].trimmed();

//         // 去掉 "GroupN <" 前缀和末尾 ">"
//         int ltIndex = groupStr.indexOf("<");
//         if (ltIndex == -1) continue;
//         groupStr = groupStr.mid(ltIndex + 1).trimmed();

//         // 拆分字段
//         QStringList fields = groupStr.split(" ", Qt::SkipEmptyParts);
//         if (fields.size() < 8) continue;

//         QString xStr = fields[0].split("=")[1];
//         QString yStr = fields[1].split("=")[1];

//         QLineEdit* xEdit = this->findChild<QLineEdit*>(xFields[groupIndex]);
//         QLineEdit* yEdit = this->findChild<QLineEdit*>(yFields[groupIndex]);
//         if (xEdit) xEdit->setText(xStr);
//         if (yEdit) yEdit->setText(yStr);

//         for (int aIndex = 0; aIndex < 6; ++aIndex) {
//             QString val = fields[2 + aIndex].split("=")[1];
//             QLineEdit* aEdit = this->findChild<QLineEdit*>(aFields[groupIndex][aIndex]);
//             if (aEdit) aEdit->setText(val);
//         }
//     }
// }

// 应用深色主题
void MainWindow1::applyDarkTheme()
{
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;

    // 主窗口背景 - 深灰蓝色
    darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::WindowText, QColor(240, 240, 240));

    // 输入框背景 - 调亮一些，与背景区分更柔和
    darkPalette.setColor(QPalette::Base, QColor(55, 55, 60));
    darkPalette.setColor(QPalette::AlternateBase, QColor(62, 62, 67));

    // 提示框
    darkPalette.setColor(QPalette::ToolTipBase, QColor(45, 48, 55));
    darkPalette.setColor(QPalette::ToolTipText, QColor(220, 220, 225));

    // 文字颜色 - 浅灰白
    darkPalette.setColor(QPalette::Text, QColor(225, 225, 230));

    // 按钮背景 - 中灰色
    darkPalette.setColor(QPalette::Button, QColor(70, 70, 76));
    darkPalette.setColor(QPalette::ButtonText, QColor(245, 245, 250));

    // 高亮颜色 - 亮蓝色
    darkPalette.setColor(QPalette::BrightText, QColor(255, 80, 80));
    darkPalette.setColor(QPalette::Link, QColor(65, 155, 245));
    darkPalette.setColor(QPalette::Highlight, QColor(0, 120, 215));
    darkPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));

    // 禁用状态
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 125));
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 125));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 125));

    qApp->setPalette(darkPalette);

    qApp->setStyleSheet(
        "QPushButton:checked {"
        "    background-color: rgb(0, 138, 200);"
        "    color: rgb(255, 255, 255);"
        "}"
        "#label_2 {"
        "    background-color: rgb(83, 83, 83);"
        "    color: rgb(255, 255, 255);"
        "}"
        );

    // 背景颜色
    // ui->BgColorBox->setCurrentIndex(1); // 深色
}

// 应用浅色主题
void MainWindow1::applyLightTheme()
{
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette lightPalette;

    // 主窗口背景 - 柔和的白灰色
    lightPalette.setColor(QPalette::Window, QColor(245, 245, 250));
    lightPalette.setColor(QPalette::WindowText, QColor(30, 30, 35));

    // 输入框背景 - 纯白色
    lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::AlternateBase, QColor(248, 248, 252));

    // 提示框
    lightPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::ToolTipText, QColor(60, 60, 65));

    // 文字颜色 - 深灰色
    lightPalette.setColor(QPalette::Text, QColor(35, 35, 40));

    // 按钮背景 - 加深，让按钮更明显
    lightPalette.setColor(QPalette::Button, QColor(210, 212, 218));
    lightPalette.setColor(QPalette::ButtonText, QColor(30, 30, 35));

    // 高亮颜色 - 优雅的蓝色
    lightPalette.setColor(QPalette::BrightText, QColor(220, 50, 50));
    lightPalette.setColor(QPalette::Link, QColor(0, 100, 200));
    lightPalette.setColor(QPalette::Highlight, QColor(0, 110, 220));
    lightPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));

    // 禁用状态
    lightPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(150, 150, 155));
    lightPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(150, 150, 155));
    lightPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(150, 150, 155));

    qApp->setPalette(lightPalette);

    qApp->setStyleSheet(
        "QPushButton:checked {"
        "    background-color: rgb(0, 138, 200);"
        "    color: rgb(255, 255, 255);"
        "}"
        "#label_2 {"
        "    background-color: rgb(205, 206, 210);"
        "    color: rgb(0, 0, 0);"
        "}"
        );

    // 背景颜色
    // ui->BgColorBox->setCurrentIndex(4); // 浅色
}

// 状态指示灯
void MainWindow1::setupIndicatorFromCheckBox(QCheckBox *checkBox, const QColor &onColor)
{
    if (!checkBox) return;

    checkBox->setText("");
    checkBox->setStyleSheet(QString(
                                // 基础样式
                                "QCheckBox {"
                                "    spacing: 0px;"
                                "}"

                                // 指示灯主体
                                "QCheckBox::indicator {"
                                "    width: 22px;"
                                "    height: 22px;"
                                "    border-radius: 11px;"
                                "}"

                                // 灭灯状态（带金属质感）
                                "QCheckBox::indicator:unchecked {"
                                "    background-color: qradialgradient("
                                "        cx:0.35, cy:0.35, radius:0.85,"
                                "        fx:0.3, fy:0.3,"
                                "        stop:0 #e8e8e8,"
                                "        stop:0.4 #b0b0b0,"
                                "        stop:0.7 #707070,"
                                "        stop:1 #404040"
                                "    );"
                                "    border: 1px solid #505050;"
                                "}"

                                // 亮灯状态（带发光效果）
                                "QCheckBox::indicator:checked {"
                                "    background-color: qradialgradient("
                                "        cx:0.3, cy:0.3, radius:0.9,"
                                "        fx:0.25, fy:0.25,"
                                "        stop:0 #ffffff,"
                                "        stop:0.15 %1,"
                                "        stop:0.5 %2,"
                                "        stop:0.85 %3,"
                                "        stop:1 %4"
                                "    );"
                                "    border: 1px solid %5;"
                                "}"

                                // 悬停效果
                                "QCheckBox::indicator:hover {"
                                "    border: 2px solid #aaa;"
                                "}"

                                // 按下效果
                                "QCheckBox::indicator:pressed {"
                                "    border: 2px solid #666;"
                                "}"
                                )
                                // 亮灯渐变颜色参数
                                .arg(onColor.lighter(180).name())      // 最亮的高光
                                .arg(onColor.lighter(130).name())      // 次亮区域
                                .arg(onColor.name())                   // 主色调
                                .arg(onColor.darker(140).name())       // 边缘阴影
                                .arg(onColor.darker(120).name()));     // 边框颜色
}

// 删除当前行
void MainWindow1::onDeleteRow()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (!button)
        return;

    int row = button->property("row").toInt();
    if (row < 0 || row >= static_cast<int>(m_tcgPoints.size()))
        return;

    int index = m_tcgPoints[row].first;
    int beamIndex = m_tcgPoints[row].second;

    config.setTcgPointDepth(index, -1.0, beamIndex);
    config.setTcgPointGain(index, -1.0, beamIndex);

    loadTcgPoints();
    refreshTabwidget();
}

// 单元格响应
void MainWindow1::onCellChanged(int row, int column)
{
    if (row < 0 || row >= static_cast<int>(m_tcgPoints.size()))
        return;

    int index = m_tcgPoints[row].first;
    int beamIndex = m_tcgPoints[row].second;

    QTableWidgetItem *item = ui->tableWidget->item(row, column);
    if (!item)
        return;

    bool ok;
    double value = item->text().toDouble(&ok);
    if (!ok)
        return;

    if (column == 2) {
        config.setTcgPointDepth(index, value, beamIndex);
    } else if (column == 3) {
        config.setTcgPointGain(index, value, beamIndex);
    }

    loadTcgPoints();
    refreshTabwidget();
}

// 刷新标签页
void MainWindow1::refreshTabwidget()
{
    int beamIndex = m_currentBeam - 1;

    ui->tableWidget->blockSignals(true);
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(m_tcgPoints.size());

    for (int i = 0; i < static_cast<int>(m_tcgPoints.size()); i++) {
        int index = m_tcgPoints[i].first;
        int beam = m_tcgPoints[i].second;

        auto indexItem = new QTableWidgetItem(QString::number(i + 1));
        indexItem->setTextAlignment(Qt::AlignCenter);
        indexItem->setFlags(indexItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, 0, indexItem);

        auto beamItem = new QTableWidgetItem(QString::number(beam + 1));
        beamItem->setTextAlignment(Qt::AlignCenter);
        beamItem->setFlags(beamItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, 1, beamItem);

        auto depthItem =
            new QTableWidgetItem(QString::number(config.getTcgPointDepth(index, beam)));
        depthItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->setEditTriggers(QAbstractItemView::AllEditTriggers);
        ui->tableWidget->setItem(i, 2, depthItem);

        auto gainItem = new QTableWidgetItem(QString::number(config.getTcgPointGain(index, beam)));
        gainItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->setItem(i, 3, gainItem);

        deleteButton = new QPushButton(tr("删除"));
        deleteButton->setProperty("row", i);
        deleteButton->setStyleSheet("font:14pt;");
        connect(deleteButton, &QPushButton::clicked, this, &MainWindow1::onDeleteRow);
        ui->tableWidget->setCellWidget(i, 4, deleteButton);
    }
    ui->tableWidget->blockSignals(false);
    ui->tableWidget->scrollToBottom();
}

// 查找可用索引
int MainWindow1::findNextAvailableIndex(const std::vector<int> &numbers)
{
    for (int nextIndex = 1;; ++nextIndex) {
        if (std::find(numbers.begin(), numbers.end(), nextIndex) == numbers.end())
            return nextIndex;
    }
    return -1;
}

// 加载TCG点
void MainWindow1::loadTcgPoints()
{
    m_tcgPoints.clear();
    int beamCount = qMax(1, config.getBeamCounts());

    for (int b = 0; b < beamCount; b++) {
        for (int i = 1;; i++) {
            double depth = config.getTcgPointDepth(i, b);
            if (depth < 0.0)
                break;
            double gain = config.getTcgPointGain(i, b);
            if (gain < 0.0)
                break;
            m_tcgPoints.push_back({ i, b });
        }
    }
}

// 记录位姿点
void MainWindow1::recordPosePoint(int pointIndex)
{
    // X Y 输入框映射
    QStringList xyFields = {
        "lineEdit_17", "lineEdit_22",  // 点1
        "lineEdit_19", "lineEdit_23",  // 点2
        "lineEdit_20", "lineEdit_24",  // 点3
        "lineEdit_21", "lineEdit_25"   // 点4
    };

    // A 输入框映射
    QStringList aFields = {
        "lineEdit_26", "lineEdit_30", "lineEdit_34", "lineEdit_38", "lineEdit_42", "lineEdit_46",  // 点1
        "lineEdit_27", "lineEdit_31", "lineEdit_35", "lineEdit_39", "lineEdit_43", "lineEdit_47",  // 点2
        "lineEdit_28", "lineEdit_32", "lineEdit_36", "lineEdit_40", "lineEdit_44", "lineEdit_48",  // 点3
        "lineEdit_29", "lineEdit_33", "lineEdit_37", "lineEdit_41", "lineEdit_45", "lineEdit_49"   // 点4
    };

    int idx = pointIndex - 1;

    // 发送 XML 请求获取 A 值
    QByteArray tempxml = kuka->create_xml(posetemp.cmd, posetemp.x, posetemp.y, posetemp.z, posetemp.a, posetemp.b, posetemp.c);
    QTcpSocket* clientConnection = kuka->getClientConnection();

    qDebug() << tcp_ok;

    if (!clientConnection || !tcp_ok) {
        qDebug() << "KUKA 未连接，无法记录位姿点";
        QMessageBox::information(nullptr, tr("提示"), tr("KUKA 未连接，无法记录位姿点"));
        return;
    }
    else
    {
        clientConnection->write(tempxml, tempxml.length());

        // qDebug() << "Sending XML:\n" << tempxml;

        double A[6] = {0};

        if (clientConnection->waitForReadyRead(1000)) {
            QByteArray readxml = clientConnection->readAll();
            // qDebug() << "Raw received XML:\n" << readxml;

            QDomDocument document;
            document.setContent(readxml);
            QDomElement root = document.documentElement();

            // for (int i = 0; i < 6; ++i) {
            //     A[i] = root.firstChildElement(QString("mA%1").arg(i + 1)).text().toFloat();
            // }
            // 笛卡尔坐标也用同样的数组
            A[0] = root.firstChildElement("mx").text().toFloat();           // X
            A[1] = root.firstChildElement("my").text().toFloat();           // Y
            A[2] = root.firstChildElement("mz").text().toFloat() + 159.5;   // Z
            A[3] = root.firstChildElement("ma").text().toFloat();           // A
            A[4] = root.firstChildElement("mb").text().toFloat();           // B
            A[5] = root.firstChildElement("mc").text().toFloat();           // C

            // 设置 A 值
            for (int i = 0; i < 6; ++i) {
                findChild<QLineEdit*>(aFields[idx * 6 + i])->setText(QString::number(A[i], 'f', 2));
            }

            // 设置 X Y 值
            findChild<QLineEdit*>(xyFields[idx * 2])->setText(QString::number(adsClient.getFloatVal(0x67D18), 'f', 2));
            findChild<QLineEdit*>(xyFields[idx * 2 + 1])->setText(QString::number(adsClient.getFloatVal(0x67D98), 'f', 2));

            qDebug() << "位姿点" << pointIndex << "记录成功";

        } else {
            QMessageBox::information(nullptr, tr("提示"), tr("请运行KUKA GROUP"));
            qDebug() << "请运行KUKA GROUP";
        }
    }
}

// 刷新工作组
void MainWindow1::refreshgroup()
{
    ui->group_comboBox->blockSignals(true);
    ui->group_comboBox->clear();
    auto Groupsize = config.getGroupsNo();

    for (int i = 0; i < Groupsize.size(); i++) {
        ui->group_comboBox->addItem(QString::number(Groupsize.at(i)));
    }
    int id = config.getCurrentGroup();
    ui->group_comboBox->setCurrentText(QString::number(id));
    ui->group_comboBox->blockSignals(false);
    emit App::getInstance()->signal_GateView_Refresh();
    emit App::getInstance()->refresh_Allpara();
}

// 新建文件
void MainWindow1::on_pushButton_43_clicked()
{
    // 创建自定义对话框
    QDialog dialog(this);
    dialog.setWindowTitle(tr("新建文件"));
    dialog.setFixedSize(400, 480);  // 增加高度以容纳新按钮
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_DeleteOnClose, false);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(25, 25, 25, 25);

    // ========== 标题区域 ==========
    QHBoxLayout *titleLayout = new QHBoxLayout();
    titleLayout->setContentsMargins(0, 0, 0, 10);
    QLabel *titleLabel = new QLabel(tr("新建文件"), &dialog);
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: palette(windowText);");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    mainLayout->addLayout(titleLayout);

    // 提示区域
    QLabel *subtitleLabel = new QLabel(tr("选择要创建的文件类型或新建项目"), &dialog);
    subtitleLabel->setStyleSheet("font-size: 11pt; color: rgba(128, 128, 128, 0.8); margin-bottom: 20px;");
    mainLayout->addWidget(subtitleLabel);

    // ========== 按钮区域 ==========
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->setSpacing(15);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    // 新建项目按钮 (放在最上面，突出显示)
    QPushButton *projectButton = new QPushButton(tr("  📁  新建项目"), &dialog);

    // 原有按钮
    QPushButton *partButton = new QPushButton(tr("  📦  CATPart"), &dialog);
    QPushButton *productButton = new QPushButton(tr("  🔧  CATProduct"), &dialog);
    QPushButton *processButton = new QPushButton(tr("  ⚙  CATProcess"), &dialog);

    // 按钮样式 - 新建项目按钮用不同的强调色
    QString buttonStyle =
        "QPushButton {"
        "    font-size: 13pt;"
        "    font-weight: 500;"
        "    text-align: left;"
        "    padding: 12px 20px;"
        "    border: 1px solid #dddddd;"
        "    border-radius: 8px;"
        "    background-color: palette(base);"
        "    color: palette(text);"
        "}"
        "QPushButton:hover {"
        "    background-color: palette(highlight);"
        "    color: palette(highlighted-text);"
        "    border-color: palette(highlight);"
        "    padding-left: 25px;"
        "}"
        "QPushButton:pressed {"
        "    background-color: palette(dark);"
        "}";

    // 新建项目按钮用稍微不同的样式（绿色调强调）
    QString projectButtonStyle =
        "QPushButton {"
        "    font-size: 13pt;"
        "    font-weight: 600;"
        "    text-align: left;"
        "    padding: 14px 20px;"
        "    border: 2px solid #2ecc71;"
        "    border-radius: 8px;"
        "    background-color: palette(base);"
        "    color: #2ecc71;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2ecc71;"
        "    color: white;"
        "    border-color: #27ae60;"
        "    padding-left: 25px;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #27ae60;"
        "}";

    projectButton->setStyleSheet(projectButtonStyle);
    partButton->setStyleSheet(buttonStyle);
    productButton->setStyleSheet(buttonStyle);
    processButton->setStyleSheet(buttonStyle);

    // 设置按钮大小策略
    QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    projectButton->setSizePolicy(policy);
    partButton->setSizePolicy(policy);
    productButton->setSizePolicy(policy);
    processButton->setSizePolicy(policy);

    projectButton->setMinimumHeight(55);
    partButton->setMinimumHeight(50);
    productButton->setMinimumHeight(50);
    processButton->setMinimumHeight(50);

    projectButton->setMaximumWidth(350);
    partButton->setMaximumWidth(350);
    productButton->setMaximumWidth(350);
    processButton->setMaximumWidth(350);

    // 添加分隔线
    QFrame *line = new QFrame(&dialog);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("background-color: #dddddd; max-height: 1px; margin: 10px 0;");

    buttonLayout->addWidget(projectButton);
    buttonLayout->addWidget(line);
    buttonLayout->addWidget(partButton);
    buttonLayout->addWidget(productButton);
    buttonLayout->addWidget(processButton);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();

    // ========== 取消按钮区域 ==========
    QHBoxLayout *cancelLayout = new QHBoxLayout();
    cancelLayout->setContentsMargins(0, 30, 0, 0);
    cancelLayout->addStretch();

    QPushButton *cancelButton = new QPushButton(tr("取消"), &dialog);

    QString cancelStyle =
        "QPushButton {"
        "    font-size: 13pt;"
        "    font-weight: 500;"
        "    padding: 8px 32px;"
        "    border: 1px solid #dddddd;"
        "    border-radius: 8px;"
        "    background-color: palette(base);"
        "    color: palette(text);"
        "}"
        "QPushButton:hover {"
        "    background-color: palette(highlight);"
        "    color: palette(highlighted-text);"
        "    border-color: palette(highlight);"
        "}"
        "QPushButton:pressed {"
        "    background-color: palette(dark);"
        "}";

    cancelButton->setStyleSheet(cancelStyle);
    cancelButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    cancelButton->setFixedSize(120, 45);

    cancelLayout->addWidget(cancelButton);
    cancelLayout->addStretch();

    mainLayout->addLayout(cancelLayout);


    // ========== 新建项目按钮逻辑 ==========
    connect(projectButton, &QPushButton::clicked, [&]() {
        dialog.accept();

        // 读取上次保存的路径
        QSettings settings("HeWenTech", "SoundScan");
        QString lastPartPath = settings.value("LastPartFilePath", QDir::homePath()).toString();
        QString lastProjectPath = settings.value("LastProjectPath", QDir::homePath()).toString();

        // 1. 选择 .CATPart 文件
        QString sourcePartPath = QFileDialog::getOpenFileName(
            this,
            tr("选择零件文件 (.CATPart)"),
            lastPartPath,
            tr("CATPart 文件 (*.CATPart)")
            );

        if (sourcePartPath.isEmpty()) {
            return;
        }

        QFileInfo sourcePartInfo(sourcePartPath);
        settings.setValue("LastPartFilePath", sourcePartInfo.absolutePath());

        // 提取项目名称
        QString projectName = sourcePartInfo.baseName();
        QRegularExpression illegalChars("[\\\\/:*?\"<>|]");
        projectName.replace(illegalChars, "_");

        if (projectName.isEmpty()) {
            QMessageBox::warning(this, tr("警告"), tr("无法获取有效的项目名称"));
            return;
        }

        // 2. 选择项目保存路径
        QString projectPath = QFileDialog::getExistingDirectory(
            this,
            tr("选择项目保存路径"),
            lastProjectPath,
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
            );

        if (projectPath.isEmpty()) {
            return;
        }

        settings.setValue("LastProjectPath", projectPath);

        // 3. 清空状态栏并调用 Worker 创建项目
        ui->lineEdit_3->setText(tr("正在创建项目..."));

        QMetaObject::invokeMethod(m_worker, "doCreateProject",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, sourcePartPath),
                                  Q_ARG(QString, projectPath),
                                  Q_ARG(QString, projectName));
    });

    // 原有按钮的连接保持不变
    connect(partButton, &QPushButton::clicked, [&]() {
        QString fileName = ui->lineEdit_3->text().trimmed();
        if (!fileName.isEmpty()) {
            QRegularExpression illegalChars("[\\\\/:*?\"<>|]");
            fileName.replace(illegalChars, "_");
        }
        ui->lineEdit_3->clear();
        dialog.accept();
        QMetaObject::invokeMethod(m_worker, "doNewFile",
                                  Qt::QueuedConnection, Q_ARG(QString, "CATPart"), Q_ARG(QString, fileName));
    });

    connect(productButton, &QPushButton::clicked, [&]() {
        QString fileName = ui->lineEdit_3->text().trimmed();
        if (!fileName.isEmpty()) {
            QRegularExpression illegalChars("[\\\\/:*?\"<>|]");
            fileName.replace(illegalChars, "_");
        }
        ui->lineEdit_3->clear();
        dialog.accept();
        QMetaObject::invokeMethod(m_worker, "doNewFile",
                                  Qt::QueuedConnection, Q_ARG(QString, "CATProduct"), Q_ARG(QString, fileName));
    });

    connect(processButton, &QPushButton::clicked, [&]() {
        ui->lineEdit_3->clear();
        dialog.accept();
        QMetaObject::invokeMethod(m_worker, "doNewFile",
                                  Qt::QueuedConnection, Q_ARG(QString, "CATProcess"), Q_ARG(QString, ""));
    });

    connect(cancelButton, &QPushButton::clicked, [&]() {
        ui->lineEdit_3->clear();
        dialog.reject();
    });

    dialog.exec();
}

// 递归复制目录
bool MainWindow1::copyDirectory(const QString &srcPath, const QString &dstPath)
{
    QDir sourceDir(srcPath);
    if (!sourceDir.exists()) {
        qDebug() << "源文件夹不存在:" << srcPath;
        return false;
    }

    // 创建目标目录
    QDir destDir;
    if (!destDir.mkpath(dstPath)) {
        qDebug() << "创建目标文件夹失败:" << dstPath;
        return false;
    }

    // 遍历源目录中的所有条目
    QStringList entries = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &entry : entries) {
        QString srcItemPath = srcPath + "/" + entry;
        QString dstItemPath = dstPath + "/" + entry;

        QFileInfo fileInfo(srcItemPath);

        if (fileInfo.isDir()) {
            // 递归复制子目录
            if (!copyDirectory(srcItemPath, dstItemPath)) {
                return false;
            }
        } else {
            // 复制文件，如果目标文件已存在则覆盖
            if (QFile::exists(dstItemPath)) {
                QFile::remove(dstItemPath);
            }
            if (!QFile::copy(srcItemPath, dstItemPath)) {
                qDebug() << "复制文件失败:" << srcItemPath << "->" << dstItemPath;
                return false;
            }
        }
    }

    return true;
}

// 计算并更新 ZB Part 中的四个点
bool MainWindow1::calculateAndUpdateZBFourPoints(const QString &projectName)
{
    // 龙门四组X、Y
    QStringList gantryFields = {
        "lineEdit_17", "lineEdit_22",  // 点1: X, Y
        "lineEdit_19", "lineEdit_23",  // 点2: X, Y
        "lineEdit_20", "lineEdit_24",  // 点3: X, Y
        "lineEdit_21", "lineEdit_25"   // 点4: X, Y
    };

    // 机器人末端相对于基座的偏移 (X, Y, Z)
    QStringList robotOffsetFields = {
        "lineEdit_26", "lineEdit_30", "lineEdit_34",  // 点1: dx, dy, dz
        "lineEdit_27", "lineEdit_31", "lineEdit_35",  // 点2: dx, dy, dz
        "lineEdit_28", "lineEdit_32", "lineEdit_36",  // 点3: dx, dy, dz
        "lineEdit_29", "lineEdit_33", "lineEdit_37"   // 点4: dx, dy, dz
    };

    QVector<QVector3D> worldPoints;

    double offsetX = -1371.414;
    double offsetY = 45.558;
    double offsetZ = 2637;

    for (int i = 0; i < 4; ++i) {
        QLineEdit *gantryXEdit = findChild<QLineEdit*>(gantryFields[i * 2]);
        QLineEdit *gantryYEdit = findChild<QLineEdit*>(gantryFields[i * 2 + 1]);

        double gantryX = gantryXEdit->text().toDouble();
        double gantryY = gantryYEdit->text().toDouble();
        double gantryZ = 0;

        QLineEdit *robotXEdit = findChild<QLineEdit*>(robotOffsetFields[i * 3]);
        QLineEdit *robotYEdit = findChild<QLineEdit*>(robotOffsetFields[i * 3 + 1]);
        QLineEdit *robotZEdit = findChild<QLineEdit*>(robotOffsetFields[i * 3 + 2]);

        double robotX = robotXEdit->text().toDouble();
        double robotY = robotYEdit->text().toDouble();
        double robotZ = robotZEdit->text().toDouble();

        double worldX =   gantryY + robotX + offsetX;
        double worldY =   gantryX - robotY + offsetY;
        double worldZ =   gantryZ - robotZ + offsetZ;

        worldPoints.append(QVector3D(worldX, worldY, worldZ));
    }

    if (worldPoints.size() != 4) {
        qDebug() << "世界坐标点计算失败，点数不足4个";
        return false;
    }

    // 更新 ZB Part 中的四个点
    QString partZBDocName = projectName + "_ZB.CATPart";

    QMetaObject::invokeMethod(m_worker, "doAddFourPointsToPart",
                              Qt::QueuedConnection,
                              Q_ARG(QString, partZBDocName),
                              Q_ARG(double, worldPoints[0].x()), Q_ARG(double, worldPoints[0].y()), Q_ARG(double, worldPoints[0].z()),
                              Q_ARG(double, worldPoints[1].x()), Q_ARG(double, worldPoints[1].y()), Q_ARG(double, worldPoints[1].z()),
                              Q_ARG(double, worldPoints[2].x()), Q_ARG(double, worldPoints[2].y()), Q_ARG(double, worldPoints[2].z()),
                              Q_ARG(double, worldPoints[3].x()), Q_ARG(double, worldPoints[3].y()), Q_ARG(double, worldPoints[3].z()));

    return true;
}

void MainWindow1::init3DCheckerSlot()
{
    connect(this, &MainWindow1::requestStartDrawing,
            this, &MainWindow1::onRequestStartDrawing,
            Qt::QueuedConnection);
    connect(this, &MainWindow1::requestStopDrawing,
            this, &MainWindow1::onRequestStopDrawing,
            Qt::QueuedConnection);
}

void MainWindow1::start3DChecker()
{
    m_isRunning = true;
    m_3dThread = new QThread(this);
    m_3dTimer = new QTimer();
    m_3dTimer->moveToThread(m_3dThread);

    connect(m_3dThread, &QThread::started, [=]() {
        m_3dTimer->setInterval(10);
        connect(m_3dTimer, &QTimer::timeout, [=]() {
            static int lastIs3D = -1;
            int currentIs3D = adsClient.getIntVal(0);

            if (lastIs3D != -1 && currentIs3D != lastIs3D) {
                qDebug() << "[3DChecker] PLC var0 changed:" << lastIs3D << "->" << currentIs3D;
                if (currentIs3D == 0) {
                    emit requestStartDrawing();
                } else {
                    emit requestStopDrawing();
                }
            }
            lastIs3D = currentIs3D;
        });
        m_3dTimer->start();
    });

    connect(m_3dThread, &QThread::finished, m_3dTimer, &QTimer::deleteLater);
    // 注意：不要连接 QThread 自身 deleteLater，否则线程一停对象就被删，
    // 成员指针悬垂，窗口析构时访问会触发 0xC0000005。
    // 线程对象作为窗口子对象，统一由 MainWindow1 析构清理。

    m_3dThread->start();
}

void MainWindow1::onRequestStartDrawing()
{

    // 仅在扫描进行中响应“机器人开始扫板”信号，避免启动阶段/结束后的误触发
    if (scan_continue_flag) {
        return;
    }

    // 机器人开始扫板信号已到达：触发 3dscan 开始绘制
    if (m_scanStartPending) {
        m_scanStartPending = false;
        if (MainWindow2::s_instance) {
            MainWindow3 *mw3 = MainWindow2::s_instance->getMainWindow3();
            if (mw3)
                mw3->startDrawing();
        }
    }

    m_start = true;

}

void MainWindow1::onRequestStopDrawing()
{
    // 仅在扫描进行中响应停止信号，避免启动阶段 var0 跳变提前发送停止命令
    if (scan_continue_flag) {
        return;
    }

    m_start = false;

    // 扫描结束
    MainWindow1::on_pushButton_20_clicked();

}

// 打开文件
void MainWindow1::on_pushButton_clicked()
{
    // 先获取文件路径（主线程，因为需要文件对话框）
    QSettings settings("HeWenTech", "CATIAAutomationTool");
    QString lastDir = settings.value("lastCATPartDir", QDir::homePath()).toString();
    QString filter = "所有文件(*.*);;零件(*.CATPart);;产品(*.CATProduct);;流程(*.CATProcess)";
    QString filePath = QFileDialog::getOpenFileName(this, tr("请选择文件"), lastDir, filter);

    if (filePath.isEmpty()) {
        return;
    }

    // 显示文件路径
    ui->lineEdit_2->setText(QDir::toNativeSeparators(filePath));

    // 保存路径到设置
    QFileInfo fileInfo(filePath);
    settings.setValue("lastCATPartDir", fileInfo.absolutePath());

    // 子线程执行打开文件
    QMetaObject::invokeMethod(m_worker, "doOpenFile",
                              Qt::QueuedConnection, Q_ARG(QString, filePath));
}

// 保存文件
void MainWindow1::on_pushButton_3_clicked()
{

    QMetaObject::invokeMethod(m_worker, "doSaveCurrentFile", Qt::QueuedConnection);
}

// 退出仿真
void MainWindow1::on_pushButton_8_clicked()
{
    QMetaObject::invokeMethod(m_worker, "doExitSimulation", Qt::QueuedConnection);
}

// 切换窗口
void MainWindow1::on_pushButton_4_clicked()
{
    // 创建一个静态指针来保存当前的对话框和列表控件
    static QPointer<QDialog> currentDialog = nullptr;
    static QPointer<QListWidget> currentListWidget = nullptr;

    // 如果已经有对话框打开，则将其提到前台
    if (currentDialog && currentDialog->isVisible()) {
        currentDialog->raise();
        currentDialog->activateWindow();
        return;
    }

    auto refreshListOnly = [](const QList<QStringList> &docInfoList, QListWidget *listWidget, QDialog *dialog) {
        if (!listWidget) return;

        listWidget->clear();

        if (docInfoList.isEmpty()) {
            QListWidgetItem *item = new QListWidgetItem(listWidget);
            item->setText(tr("没有打开的文档"));
            item->setFlags(Qt::NoItemFlags);
            listWidget->addItem(item);
            return;
        }

        int activeIndex = -1;
        QStringList addedDocNames;  // 用于去重，只显示每个文档名第一次出现

        for (int i = 0; i < docInfoList.size(); ++i) {
            const QStringList &info = docInfoList[i];
            QString docName = info[0];
            QString fullPath = info[1];
            bool isSaved = info[3] == "1";
            bool isActive = info[4] == "1";

            if (addedDocNames.contains(docName)) {
                // qDebug() << "[切换窗口] 跳过重复文档:" << docName;
                continue;
            }
            addedDocNames.append(docName);

            QString iconText;
            QString displayName;
            if (!isSaved) {
                iconText = "⚠️";
                displayName = docName + tr(" (未保存)");
            } else if (docName.endsWith(".CATPart", Qt::CaseInsensitive)) {
                iconText = "📦";
                displayName = docName;
            } else if (docName.endsWith(".CATProduct", Qt::CaseInsensitive)) {
                iconText = "🔧";
                displayName = docName;
            } else if (docName.endsWith(".CATProcess", Qt::CaseInsensitive)) {
                iconText = "⚙";
                displayName = docName;
            } else {
                iconText = "📄";
                displayName = docName;
            }

            QListWidgetItem *item = new QListWidgetItem(iconText + "  " + displayName);
            item->setData(Qt::UserRole, info[2].toInt());
            item->setToolTip(isSaved ? fullPath : tr("未保存的文档"));
            listWidget->addItem(item);

            if (isActive) activeIndex = i;
        }

        if (activeIndex >= 0 && activeIndex < listWidget->count()) {
            listWidget->setCurrentRow(activeIndex);
        } else if (listWidget->count() > 0) {
            listWidget->setCurrentRow(0);
        }
    };

    // 断开之前的刷新连接（如果存在）
    static QMetaObject::Connection refreshConnection;
    if (refreshConnection) {
        disconnect(refreshConnection);
    }

    // 连接刷新列表的信号（不创建新对话框）
    refreshConnection = connect(m_worker, &DelmiaWorker::documentListReady, this,
                                [this, refreshListOnly](const QList<QStringList> &docInfoList) {
                                    // 获取当前的对话框和列表控件
                                    static QPointer<QDialog> dialog = nullptr;
                                    static QPointer<QListWidget> listWidget = nullptr;

                                    // 如果没有对话框或对话框已关闭，则创建新对话框
                                    if (!dialog || !dialog->isVisible()) {
                                        // 创建新对话框
                                        createSwitchWindowDialog(docInfoList, dialog, listWidget);
                                    } else {
                                        // 只刷新列表
                                        if (listWidget) {
                                            refreshListOnly(docInfoList, listWidget, dialog);
                                        }
                                    }
                                });

    // 获取文档列表（子线程执行）
    QMetaObject::invokeMethod(m_worker, "doGetDocumentList", Qt::QueuedConnection);
}

// 提取创建对话框的函数
void MainWindow1::createSwitchWindowDialog(const QList<QStringList> &docInfoList,
                                          QPointer<QDialog> &dialog,
                                          QPointer<QListWidget> &listWidget)
{
    // 如果已有对话框，先关闭
    if (dialog) {
        dialog->deleteLater();
    }

    dialog = new QDialog(this);
    dialog->setWindowTitle(tr("切换窗口"));
    dialog->setFixedSize(500, 480);
    dialog->setModal(true);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(20);

    QLabel *titleLabel = new QLabel(tr("选择要切换的文档（双击或选中点打开）"), dialog);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: 500; color: palette(windowText);");
    mainLayout->addWidget(titleLabel);

    listWidget = new QListWidget(dialog);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    listWidget->setFocusPolicy(Qt::NoFocus);

    // 深色/浅色模式判断
    QColor textColor = palette().color(QPalette::WindowText);
    int luminance = (textColor.red() * 0.299 + textColor.green() * 0.587 + textColor.blue() * 0.114);
    bool isDarkMode = luminance > 128;

    QString selectedBgColor, selectedBorderColor, hoverBgColor, selectedTextColor;
    if (isDarkMode) {
        selectedBgColor = "rgba(0, 120, 215, 0.3)";
        selectedBorderColor = "#0078D4";
        hoverBgColor = "rgba(255, 255, 255, 0.08)";
        selectedTextColor = "palette(text)";
    } else {
        selectedBgColor = "rgba(0, 120, 215, 0.12)";
        selectedBorderColor = "#0078D4";
        hoverBgColor = "rgba(0, 0, 0, 0.04)";
        selectedTextColor = "palette(text)";
    }

    QString listStyleSheet = QString(
                                 "QListWidget {"
                                 "    font-size: 13pt;"
                                 "    border: 1px solid palette(mid);"
                                 "    border-radius: 8px;"
                                 "    background-color: palette(base);"
                                 "    outline: none;"
                                 "}"
                                 "QListWidget::item {"
                                 "    padding: 12px 16px;"
                                 "    border-bottom: 1px solid palette(mid);"
                                 "    color: palette(text);"
                                 "}"
                                 "QListWidget::item:selected {"
                                 "    background-color: %1;"
                                 "    border-left: 3px solid %2;"
                                 "    color: %4;"
                                 "}"
                                 "QListWidget::item:selected:hover {"
                                 "    background-color: %1;"
                                 "    color: %4;"
                                 "}"
                                 "QListWidget::item:hover {"
                                 "    background-color: %3;"
                                 "    color: palette(text);"
                                 "}"
                                 ).arg(selectedBgColor, selectedBorderColor, hoverBgColor, selectedTextColor);

    listWidget->setStyleSheet(listStyleSheet);

    // 填充列表
    int activeIndex = -1;
    QStringList addedDocNames;  // 用于去重

    for (int i = 0; i < docInfoList.size(); ++i) {
        const QStringList &info = docInfoList[i];
        QString docName = info[0];
        QString fullPath = info[1];
        bool isSaved = info[3] == "1";
        bool isActive = info[4] == "1";

        // 如果已经添加过同名的文档，跳过
        if (addedDocNames.contains(docName)) {
            // qDebug() << "[Delmia] 跳过重复文档:" << docName;
            continue;
        }
        addedDocNames.append(docName);

        QString iconText;
        QString displayName;
        if (!isSaved) {
            iconText = "⚠️";
            displayName = docName + tr(" (未保存)");
        } else if (docName.endsWith(".CATPart", Qt::CaseInsensitive)) {
            iconText = "📦";
            displayName = docName;
        } else if (docName.endsWith(".CATProduct", Qt::CaseInsensitive)) {
            iconText = "🔧";
            displayName = docName;
        } else if (docName.endsWith(".CATProcess", Qt::CaseInsensitive)) {
            iconText = "⚙";
            displayName = docName;
        } else {
            iconText = "📄";
            displayName = docName;
        }

        QListWidgetItem *item = new QListWidgetItem(iconText + "  " + displayName);
        item->setData(Qt::UserRole, info[2].toInt());
        item->setToolTip(isSaved ? fullPath : tr("未保存的文档"));
        listWidget->addItem(item);

        if (isActive) activeIndex = i;
    }

    if (activeIndex >= 0 && activeIndex < listWidget->count()) {
        listWidget->setCurrentRow(activeIndex);
    }

    mainLayout->addWidget(listWidget);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);
    buttonLayout->addStretch();

    QPushButton *okButton = new QPushButton(tr("打开"), dialog);
    QPushButton *saveButton = new QPushButton(tr("保存"), dialog);
    QPushButton *closeButton = new QPushButton(tr("关闭"), dialog);
    QPushButton *cancelButton = new QPushButton(tr("取消"), dialog);

    QString okButtonStyle, cancelButtonStyle, closeButtonStyle, saveButtonStyle;

    if (isDarkMode) {
        okButtonStyle = "QPushButton { font-size: 13pt; font-weight: 400; padding: 8px 28px; border-radius: 6px; background-color: #0078D4; color: white; border: none; } QPushButton:hover { background-color: #106EBE; } QPushButton:pressed { background-color: #005A9E; }";
        cancelButtonStyle = "QPushButton { font-size: 13pt; font-weight: 400; padding: 8px 28px; border-radius: 6px; background-color: rgba(255, 255, 255, 0.1); color: palette(text); border: 1px solid rgba(255, 255, 255, 0.2); } QPushButton:hover { background-color: rgba(255, 255, 255, 0.15); } QPushButton:pressed { background-color: rgba(255, 255, 255, 0.08); }";
        closeButtonStyle = "QPushButton { font-size: 13pt; font-weight: 400; padding: 8px 28px; border-radius: 6px; background-color: rgba(220, 53, 69, 0.8); color: white; border: none; } QPushButton:hover { background-color: rgba(220, 53, 69, 1.0); } QPushButton:pressed { background-color: rgba(200, 35, 51, 1.0); }";
        saveButtonStyle = "QPushButton { font-size: 13pt; font-weight: 400; padding: 8px 28px; border-radius: 6px; background-color: rgba(255, 152, 0, 0.8); color: white; border: none; } QPushButton:hover { background-color: rgba(255, 152, 0, 1.0); } QPushButton:pressed { background-color: rgba(230, 130, 0, 1.0); }";
    } else {
        okButtonStyle = "QPushButton { font-size: 13pt; font-weight: 400; padding: 8px 28px; border-radius: 6px; background-color: #0078D4; color: white; border: none; } QPushButton:hover { background-color: #106EBE; } QPushButton:pressed { background-color: #005A9E; }";
        cancelButtonStyle = "QPushButton { font-size: 13pt; font-weight: 400; padding: 8px 28px; border-radius: 6px; background-color: #f0f0f0; color: #333333; border: 1px solid #cccccc; } QPushButton:hover { background-color: #e0e0e0; } QPushButton:pressed { background-color: #d0d0d0; }";
        closeButtonStyle = "QPushButton { font-size: 13pt; font-weight: 400; padding: 8px 28px; border-radius: 6px; background-color: #dc3545; color: white; border: none; } QPushButton:hover { background-color: #c82333; } QPushButton:pressed { background-color: #bd2130; }";
        saveButtonStyle = "QPushButton { font-size: 13pt; font-weight: 400; padding: 8px 28px; border-radius: 6px; background-color: #FF9800; color: white; border: none; } QPushButton:hover { background-color: #E68900; } QPushButton:pressed { background-color: #CC7A00; }";
    }

    okButton->setStyleSheet(okButtonStyle);
    saveButton->setStyleSheet(saveButtonStyle);
    closeButton->setStyleSheet(closeButtonStyle);
    cancelButton->setStyleSheet(cancelButtonStyle);
    okButton->setDefault(true);

    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(closeButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // ========== 保存按钮 ==========
    connect(saveButton, &QPushButton::clicked, dialog, [this, dialog, listWidget]() {
        int row = listWidget->currentRow();
        if (row < 0) return;

        int docIndex = listWidget->item(row)->data(Qt::UserRole).toInt();

        // 先激活选中的文档
        QMetaObject::invokeMethod(m_worker, "doActivateDocument", Qt::QueuedConnection, Q_ARG(int, docIndex));

        // 延迟后调用外部保存函数
        QTimer::singleShot(200, this, [this]() {
            on_pushButton_3_clicked();  // 调用外部保存函数
        });

        // 延迟后刷新列表
        QTimer::singleShot(800, this, [this]() {
            QMetaObject::invokeMethod(m_worker, "doGetDocumentList", Qt::QueuedConnection);
        });
    });



    // ========== 关闭按钮 ==========
    connect(closeButton, &QPushButton::clicked, dialog, [this, dialog, listWidget]() {
        int row = listWidget->currentRow();
        if (row < 0) return;

        int docIndex = listWidget->item(row)->data(Qt::UserRole).toInt();

        // 先激活选中的文档
        QMetaObject::invokeMethod(m_worker, "doActivateDocument", Qt::QueuedConnection, Q_ARG(int, docIndex));

        // 延迟后调用外部关闭函数
        QTimer::singleShot(200, this, [this]() {
            on_pushButton_44_clicked();  // 调用外部关闭函数
        });

        // 延迟后刷新列表
        QTimer::singleShot(500, this, [this]() {
            QMetaObject::invokeMethod(m_worker, "doGetDocumentList", Qt::QueuedConnection);
        });
    });

    // ========== 双击切换 ==========
    connect(listWidget, &QListWidget::itemDoubleClicked, dialog, [this, dialog](QListWidgetItem *item) {
        int idx = item->data(Qt::UserRole).toInt();

        // 激活文档
        QMetaObject::invokeMethod(m_worker, "doActivateDocument", Qt::QueuedConnection, Q_ARG(int, idx));

        // 延迟后获取当前文档信息并更新显示
        QTimer::singleShot(200, this, [this]() {
            QMetaObject::invokeMethod(m_worker, "doGetCurrentDocumentInfo", Qt::QueuedConnection);
        });

        dialog->accept();
    });

    // ========== 打开/切换按钮 ==========
    connect(okButton, &QPushButton::clicked, dialog, [this, dialog, listWidget]() {
        int row = listWidget->currentRow();
        if (row >= 0) {
            int docIndex = listWidget->item(row)->data(Qt::UserRole).toInt();

            // 获取当前活动文档名称
            QString currentDocName;
            QAxObject *tempCatia = new QAxObject("DELMIA.Application", nullptr);
            if (tempCatia && !tempCatia->isNull()) {
                QAxObject *activeDoc = tempCatia->querySubObject("ActiveDocument");
                if (activeDoc && !activeDoc->isNull()) {
                    currentDocName = activeDoc->property("Name").toString();
                    delete activeDoc;
                }
                delete tempCatia;
            }

            // 获取选中文档的名称
            QString selectedDocName = listWidget->item(row)->text();
            // 移除图标前缀，只保留文档名
            selectedDocName.remove(QRegularExpression("^[📦🔧⚙⚠️📄]\\s+"));
            selectedDocName = selectedDocName.replace(" (未保存)", "");

            if (selectedDocName == currentDocName) {
                // 已经是当前文档，执行打开新文件功能
                dialog->accept();
                on_pushButton_clicked();  // 调用外部打开文件函数
            } else {
                // 切换到选中的文档
                QMetaObject::invokeMethod(m_worker, "doActivateDocument", Qt::QueuedConnection, Q_ARG(int, docIndex));

                // 延迟后获取当前文档信息并更新显示
                QTimer::singleShot(200, this, [this]() {
                    QMetaObject::invokeMethod(m_worker, "doGetCurrentDocumentInfo", Qt::QueuedConnection);
                });

                dialog->accept();
            }
        } else {
            dialog->accept();
        }
    });

    connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);

    // 对话框关闭时清空静态指针
    connect(dialog, &QDialog::destroyed, [&]() {
        dialog = nullptr;
        listWidget = nullptr;
    });

    dialog->exec();
}

// 关闭文件
void MainWindow1::on_pushButton_44_clicked()
{
    QMetaObject::invokeMethod(m_worker, "doCloseCurrentFile", Qt::QueuedConnection);
}

// 选择横向界限
void MainWindow1::on_pushButton_2_clicked()
{
    if (ui->lineEdit_2->text().contains(".CATPart", Qt::CaseInsensitive))
    {
        on_pushButton_29_clicked();
        QMetaObject::invokeMethod(m_worker, "doExecuteCommand",
                                  Qt::QueuedConnection, Q_ARG(QString, "选择横向界限"));
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请打开Part文件"));
    }
}

// 选择纵向界限
void MainWindow1::on_pushButton_5_clicked()
{
    if (ui->lineEdit_2->text().contains(".CATPart", Qt::CaseInsensitive))
    {
        on_pushButton_29_clicked();
        QMetaObject::invokeMethod(m_worker, "doExecuteCommand",
                                  Qt::QueuedConnection, Q_ARG(QString, "选择纵向界限"));
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请打开Part文件"));
    }
}

// 分区扫描
void MainWindow1::on_pushButton_40_clicked()
{
    if (ui->lineEdit_2->text().contains(".CATPart", Qt::CaseInsensitive))
    {
        on_pushButton_29_clicked();
        QMetaObject::invokeMethod(m_worker, "doExecuteCommand",
                                  Qt::QueuedConnection, Q_ARG(QString, "分区扫描"));
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请打开Part文件"));
    }
}

// 创建分区
void MainWindow1::on_pushButton_6_clicked()
{
    if (ui->lineEdit_2->text().contains(".CATPart", Qt::CaseInsensitive))
    {
        on_pushButton_29_clicked();
        QMetaObject::invokeMethod(m_worker, "doExecuteCommand",
                                  Qt::QueuedConnection, Q_ARG(QString, "创建分区"));
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请打开Part文件"));
    }
}

// 选择闭合边界
void MainWindow1::on_pushButton_10_clicked()
{
    if (ui->lineEdit_2->text().contains(".CATPart", Qt::CaseInsensitive))
    {
        on_pushButton_29_clicked();
        QMetaObject::invokeMethod(m_worker, "doExecuteCommand",
                                  Qt::QueuedConnection, Q_ARG(QString, "选择闭合边界"));
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请打开Part文件"));
    }
}

// 选择起始路径
void MainWindow1::on_pushButton_11_clicked()
{
    if (ui->lineEdit_2->text().contains(".CATPart", Qt::CaseInsensitive))
    {
        on_pushButton_29_clicked();
        QMetaObject::invokeMethod(m_worker, "doExecuteCommand",
                                  Qt::QueuedConnection, Q_ARG(QString, "选择起始路径"));
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请打开Part文件"));
    }
}

// 路径规划
void MainWindow1::on_pushButton_46_clicked()
{
    if (ui->lineEdit_2->text().contains(".CATPart", Qt::CaseInsensitive))
    {
        on_pushButton_29_clicked();
        QMetaObject::invokeMethod(m_worker, "doExecuteCommand",
                                  Qt::QueuedConnection, Q_ARG(QString, "路径规划"));
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请打开Part文件"));
    }
}

// 创建路径
void MainWindow1::on_pushButton_12_clicked()
{
    if (ui->lineEdit_2->text().contains(".CATPart", Qt::CaseInsensitive))
    {
        on_pushButton_29_clicked();
        QMetaObject::invokeMethod(m_worker, "doExecuteCommand",
                                  Qt::QueuedConnection, Q_ARG(QString, "创建路径"));
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("请打开Part文件"));
    }
}

// 标定点位
void MainWindow1::on_pushButton_17_clicked()
{
    QMetaObject::invokeMethod(m_worker, "doGetActiveDocumentName", Qt::QueuedConnection);

    static QMetaObject::Connection conn;
    conn = connect(m_worker, &DelmiaWorker::activeDocumentNameResult, this, [=](const QString &docName) {
        disconnect(conn);

        if (docName.isEmpty() || !docName.endsWith(".CATProcess", Qt::CaseInsensitive)) {
            QMessageBox::warning(this, tr("警告"), tr("请打开Process文件"));
            return;
        }

        // 从文档名提取项目名称（去掉 .CATProcess 后缀）
        QString projectName = docName;
        projectName.replace(".CATProcess", "", Qt::CaseInsensitive);

        // 使用通用函数更新 ZB Part 中的四个点
        if (calculateAndUpdateZBFourPoints(projectName)) {
            QMessageBox::information(this, tr("成功"), tr("标定点位成功"));
        } else {
            QMessageBox::warning(this, tr("警告"), tr("标定点位失败"));
        }
    });
}

// 扫描开始
void MainWindow1::on_pushButton_9_clicked()
{
    MainWindow1::on_pushButton_9();
}

// 扫描开始（外部调用）
void MainWindow1::on_pushButton_9()
{
    // 防重入：已处于扫描中（机器人已启动）时重复点击开始，直接忽略，
    // 避免重复下发 0x5E256 导致 PLC 状态错乱
    if (!scan_continue_flag && adsClient.getIntVal(0) != 0) {
        return;
    }

    // 暂停后恢复：不要失能/复位龙门电机。暂停时电机处于使能状态，
    // 失能+复位后需要重新使能才能运动，而 PLC 的“继续”流程不会重新使能，
    // 结果是机器人收到恢复命令也不动（日志中 var0 不再跳变）。
    bool resuming = false;
    if (MainWindow2::s_instance) {
        MainWindow3 *mw3 = MainWindow2::s_instance->getMainWindow3();
        if (mw3 && mw3->isDrawPaused())
            resuming = true;
    }
    if (!resuming)
        MainWindow1::on_pushButton_28_clicked(); // 龙门电机失能

    adsClient.setIntVal(0x5EB08, ui->comboBox_2->currentIndex()+1);

    if(adsClient.getIntVal(0) || scan_continue_flag)
    {
        scan_continue_flag = false;
        adsClient.setIntVal(0x5E256, 1);
        // 不立即开始绘制：等“机器人开始扫板”信号（PLC 变量0 跳变）到达后再触发
        m_scanStartPending = true;
        // 恢复命令看门狗：仅暂停恢复时启用；机器人 2s 内位姿未变化则自动重发
        if (resuming) {
            m_resumeWatchdogArmed = true;
            m_resumeWatchdogTries = 0;
            m_resumeWx = robot_x;
            m_resumeWy = robot_y;
            m_resumeWz = robot_z;
            if (m_resumeWatchdogTimer) m_resumeWatchdogTimer->start();
        } else {
            m_resumeWatchdogArmed = false;
            if (m_resumeWatchdogTimer) m_resumeWatchdogTimer->stop();
        }
        // 暂停后恢复：3DScan 处于暂停绘制状态时直接恢复绘制。
        // 暂停恢复时机器人已在板上继续运动，PLC var0 不会重新跳变，
        // 只靠 m_scanStartPending 等 var0 触发 startDrawing 永远不会执行。
        if (MainWindow2::s_instance) {
            MainWindow3 *mw3 = MainWindow2::s_instance->getMainWindow3();
            if (mw3 && mw3->isDrawPaused())
                mw3->startDrawing();
        }
    }
    // 调试：打印扫描开始写入/读回的 PLC 值
    qDebug() << "[ScanCtrl] 扫描开始: 0x5EB08=" << adsClient.getIntVal(0x5EB08)
             << " 0x5E256=" << adsClient.getIntVal(0x5E256)
             << " var0=" << adsClient.getIntVal(0)
             << " continue_flag=" << scan_continue_flag;
}

// 扫描暂停
void MainWindow1::on_pushButton_19_clicked()
{
    MainWindow1::on_pushButton_19();
}

// 扫描暂停（外部调用）
void MainWindow1::on_pushButton_19()
{
    scan_continue_flag = true;
    adsClient.setIntVal(0x5E256, 8);
    m_scanStartPending = false;
    // 暂停时解除恢复看门狗，避免暂停后旧的重试请求继续发命令
    m_resumeWatchdogArmed = false;
    if (m_resumeWatchdogTimer) m_resumeWatchdogTimer->stop();

    // 链接 3dscan：停止绘制（暂停，可继续）
    if (MainWindow2::s_instance) {
        MainWindow3 *mw3 = MainWindow2::s_instance->getMainWindow3();
        if (mw3)
            mw3->pauseDrawing();
    }
    // 调试：打印扫描暂停写入/读回的 PLC 值
    qDebug() << "[ScanCtrl] 扫描暂停: 0x5E256=" << adsClient.getIntVal(0x5E256)
             << " var0=" << adsClient.getIntVal(0);
}

// 扫描结束
void MainWindow1::on_pushButton_20_clicked()
{
    MainWindow1::on_pushButton_20();
}

// 扫描结束（外部调用）
void MainWindow1::on_pushButton_20()
{
    scan_continue_flag = true;
    adsClient.setIntVal(0x5E256, 8);
    adsClient.setIntVal(0x5EB08, 0);
    m_scanStartPending = false;
    m_resumeWatchdogArmed = false;
    if (m_resumeWatchdogTimer) m_resumeWatchdogTimer->stop();

    // 链接 3dscan：结束绘制
    if (MainWindow2::s_instance) {
        MainWindow3 *mw3 = MainWindow2::s_instance->getMainWindow3();
        if (mw3)
            mw3->finishDrawing();
    }
    // 调试：打印扫描结束写入/读回的 PLC 值
    qDebug() << "[ScanCtrl] 扫描结束: 0x5E256=" << adsClient.getIntVal(0x5E256)
             << " 0x5EB08=" << adsClient.getIntVal(0x5EB08)
             << " var0=" << adsClient.getIntVal(0);

    // 机器人位姿
    ui->label_17->clear();
    ui->label_18->clear();
    ui->label_19->clear();
    ui->label_20->clear();
    ui->label_21->clear();
    ui->label_22->clear();
}

// 龙门开始
void MainWindow1::on_pushButton_18_clicked()
{
    if (!scan_continue_flag) {
        qDebug() << "[龙门开始] 正在扫描，禁止手动移动";
        return;
    }
    // 清掉暂停/点动残留，避免上一条指令干扰本次移动
    adsClient.setIntVal(0x5EBDF, 0);
    adsClient.setIntVal(0x5EC2F, 0);
    adsClient.setIntVal(0x5EBDB, 0);
    adsClient.setIntVal(0x5EBDC, 0);
    adsClient.setIntVal(0x5EC2B, 0);
    adsClient.setIntVal(0x5EC2C, 0);

    if (ui->comboBox->currentText() == "绝对")
    {
        if (adsClient.getIntVal(0x67CE0) && adsClient.getIntVal(0x67D60) && ui->doubleSpinBox_3->text().toFloat() && !ui->doubleSpinBox_4->text().isEmpty() && !ui->doubleSpinBox_5->text().isEmpty()) // x 轴的使能状态 MotorStatusVary[0].bEnableStatus
        {
            if (ui->doubleSpinBox_4->text().toFloat()<=ui->doubleSpinBox_10->text().toFloat() && ui->doubleSpinBox_4->text().toFloat()>= ui->doubleSpinBox_9->text().toFloat() && ui->doubleSpinBox_5->text().toFloat()<= ui->doubleSpinBox_12->text().toFloat() && ui->doubleSpinBox_5->text().toFloat()>=ui->doubleSpinBox_11->text().toFloat())
            {
                adsClient.setFloatVal(0x5EBF4, ui->doubleSpinBox_3->text().toFloat()); // 给 x 轴电机赋值绝对移动速度 MotorControlVary[0].AbsoluteVelocity
                adsClient.setFloatVal(0x5EBF0, ui->doubleSpinBox_4->text().toFloat()); // 给 x 轴电机赋值绝对移动距离 MotorControlVary[0].AbsoluteDistance
                adsClient.setFloatVal(0x5EC44, ui->doubleSpinBox_3->text().toFloat()); // 给 Y 轴电机赋值绝对移动速度 MotorControlVary[2].AbsoluteVelocity
                adsClient.setFloatVal(0x5EC40, ui->doubleSpinBox_5->text().toFloat()); // 给 Y 轴电机赋值绝对移动距离 MotorControlVary[2].AbsoluteDistance

                adsClient.setIntVal(0x5EBDE, 1); // 执行上面赋值的绝对移动参数 MotorControlVary[0].bMoveAbsolute
                adsClient.setIntVal(0x5EC2E, 1); // 执行上面赋值的绝对移动参数 MotorControlVary[2].bMoveAbsolute
            }
            else
            {
                qDebug() << "龙门位置达到(" << "X轴:" << ui->doubleSpinBox_9->text().toFloat() << "~" << ui->doubleSpinBox_10->text().toFloat() << "，Y轴:" << ui->doubleSpinBox_11->text().toFloat() << "~" << ui->doubleSpinBox_12->text().toFloat() <<")上限";
            }
        }
        else
        {
            qDebug() << "龙门电机未使能或龙门位置未设置";
        }
    }
    else if (ui->comboBox->currentText() == "相对")
    {
        if (adsClient.getIntVal(0x67CE0) && adsClient.getIntVal(0x67D60) && ui->doubleSpinBox_3->text().toFloat() && !ui->doubleSpinBox_4->text().isEmpty() && !ui->doubleSpinBox_5->text().isEmpty()) // x 轴的使能状态 MotorStatusVary[0].bEnableStatus
        {
            if (ui->doubleSpinBox_4->text().toFloat()<=500 && ui->doubleSpinBox_4->text().toFloat()>= -500 && ui->doubleSpinBox_5->text().toFloat()<=500 && ui->doubleSpinBox_5->text().toFloat()>=-500)
            {
                adsClient.setFloatVal(0x5EBEC, ui->doubleSpinBox_3->text().toFloat()); // 给 x 轴电机赋值相对移动速度 MotorControlVary[0].RelativeVelocity
                adsClient.setFloatVal(0x5EBE8, ui->doubleSpinBox_4->text().toFloat()); // 给 x 轴电机赋值相对移动距离 MotorControlVary[0].RelativeDistance
                adsClient.setFloatVal(0x5EC3C, ui->doubleSpinBox_3->text().toFloat()); // 给 Y 轴电机赋值相对移动速度 MotorControlVary[2].RelativeVelocity
                adsClient.setFloatVal(0x5EC38, ui->doubleSpinBox_5->text().toFloat()); // 给 Y 轴电机赋值相对移动距离 MotorControlVary[2].RelativeDistance

                adsClient.setIntVal(0x5EBDD, 1); // 执行上面赋值的相对移动参数 MotorControlVary[0].bMoveRelative
                adsClient.setIntVal(0x5EC2D, 1); // 执行上面赋值的相对移动参数 MotorControlVary[2].bMoveRelative
            }
            else
            {
                qDebug() << "相对运动步长达到(-500~500)上限";
            }
        }
        else
        {
            qDebug() << "龙门电机未使能或相对运动步长未设置";
        }
    }
}

// 龙门暂停
void MainWindow1::on_pushButton_21_clicked()
{
    adsClient.setIntVal(0x5EBDF, 1); // 给 x 轴电机上复位 MotorControlVary[0].bStop_do
    adsClient.setIntVal(0x5EC2F, 1); // 给 Y 轴电机上复位 MotorControlVary[2].bStop_do
    // 同时清点动位，避免暂停后残留 bJog 把电机重新驱动起来
    adsClient.setIntVal(0x5EBDB, 0);
    adsClient.setIntVal(0x5EBDC, 0);
    adsClient.setIntVal(0x5EC2B, 0);
    adsClient.setIntVal(0x5EC2C, 0);
}

// 龙门回零
void MainWindow1::on_pushButton_22_clicked()
{
    // 弹窗提示
    QMessageBox::StandardButton reply;
    QString msg = QString("即将回零(%1, %2)，是否继续？")
                      .arg(QString::number(ui->doubleSpinBox_6->text().toFloat()))
                      .arg(QString::number(ui->doubleSpinBox_7->text().toFloat()));
    reply = QMessageBox::question(this, "提示", msg, QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        // 执行移动前清掉暂停/点动残留，避免上一条指令干扰本次移动
        adsClient.setIntVal(0x5EBDF, 0);
        adsClient.setIntVal(0x5EC2F, 0);
        adsClient.setIntVal(0x5EBDB, 0);
        adsClient.setIntVal(0x5EBDC, 0);
        adsClient.setIntVal(0x5EC2B, 0);
        adsClient.setIntVal(0x5EC2C, 0);
        if (adsClient.getIntVal(0x67CE0) && adsClient.getIntVal(0x67D60) && ui->doubleSpinBox_3->text().toFloat())
        {
            adsClient.setFloatVal(0x5EBF4, ui->doubleSpinBox_3->text().toFloat()); // 给 x 轴电机赋值绝对移动速度 MotorControlVary[0].AbsoluteVelocity
            adsClient.setFloatVal(0x5EBF0, ui->doubleSpinBox_6->text().toFloat()); // 给 x 轴电机赋值绝对移动距离 MotorControlVary[0].AbsoluteDistance
            adsClient.setFloatVal(0x5EC44, ui->doubleSpinBox_3->text().toFloat()); // 给 Y 轴电机赋值绝对移动速度 MotorControlVary[2].AbsoluteVelocity
            adsClient.setFloatVal(0x5EC40, ui->doubleSpinBox_7->text().toFloat()); // 给 Y 轴电机赋值绝对移动距离 MotorControlVary[2].AbsoluteDistance

            adsClient.setIntVal(0x5EBDE, 1); // 执行上面赋值的绝对移动参数 MotorControlVary[0].bMoveAbsolute
            adsClient.setIntVal(0x5EC2E, 1); // 执行上面赋值的绝对移动参数 MotorControlVary[2].bMoveAbsolute
        }
    }
}

// 电机使能
void MainWindow1::on_pushButton_27_clicked()
{
    if (!scan_continue_flag) {
        qDebug() << "[电机使能] 当前正在扫描，无法使能（scan_continue_flag=false）";
        return;
    }

    // 直接读 PLC 程序号，避免依赖 100ms 轮询刷新的 label（旧值会导致“要等/要
    // 先点扫描结束才能使能”的假象）。程序号非 0 表示任务尚未结束，禁止使能。
    int progNo = adsClient.getIntVal(0x5EB08);
    if (progNo != 0) {
        qDebug() << "[电机使能] 当前任务号" << progNo
                 << "非0，请先点击“扫描结束”再使能";
        return;
    }

    // 先清复位位（避免上电后 bReset 残留导致使能被 PLC 拒绝），再上使能
    adsClient.setIntVal(0x5EBE0, 0); // X bReset_do
    adsClient.setIntVal(0x5EC30, 0); // Y bReset_do
    adsClient.setIntVal(0x5EBD8, 1); // X bEnable
    adsClient.setIntVal(0x5EC28, 1); // Y bEnable

    // 回读使能状态确认，100ms 后检查 PLC 是否接受
    QTimer::singleShot(100, this, [this]() {
        bool xOn = adsClient.getIntVal(0x67CE0) != 0;
        bool yOn = adsClient.getIntVal(0x67D60) != 0;
        qDebug() << "[电机使能] 回读状态 X=" << xOn << " Y=" << yOn
                 << (xOn && yOn ? "（使能成功）" : "（使能未生效，请检查急停/复位/任务号）");
    });
}

// 电机失能
void MainWindow1::on_pushButton_28_clicked()
{
    // adsClient.setIntVal(0x5EB0B, 1);

    adsClient.setIntVal(0x5EBD8, 0); // 给 x 轴电机上失能 MotorControlVary[0].bEnable
    adsClient.setIntVal(0x5EC28, 0); // 给 Y 轴电机上失能 MotorControlVary[2].bEnable

    adsClient.setIntVal(0x5EBE0, 1); // 给 x 轴电机上复位 MotorControlVary[0].bReset_do
    adsClient.setIntVal(0x5EC30, 1); // 给 Y 轴电机上复位 MotorControlVary[2].bReset_do

    qDebug()<< "龙门电机失能";
}

// X++按下
void MainWindow1::on_pushButton_23_pressed()
{
    if (!scan_continue_flag || adsClient.getIntVal(0x5EB08) != 0
        || adsClient.getIntVal(0x67CE0) == 0) {
        qDebug() << "[点动] 扫描中/任务号非0/未使能，已忽略 X+";
        return;
    }
    adsClient.setIntVal(0x5EBDF, 0); // 清 X 停止
    adsClient.setIntVal(0x5EBDC, 0); // 清 X 反向点动（互斥）
    adsClient.setFloatVal(0x5EBE4, ui->doubleSpinBox_2->text().toFloat()); // 设置 x 轴的点动速度 MotorControlVary[0].JogVelocity

    adsClient.setIntVal(0x5EBDB, 1); // 控制 x 轴向前点动 MotorControlVary[0].bJogForwards
}

// X++松开
void MainWindow1::on_pushButton_23_released()
{
    adsClient.setFloatVal(0x5EBE4, 0); // 设置 x 轴的点动速度 MotorControlVary[0].JogVelocity

    adsClient.setIntVal(0x5EBDB, 0); // 控制 x 轴向前点动 MotorControlVary[0].bJogForwards
}

// X--按下
void MainWindow1::on_pushButton_24_pressed()
{
    if (!scan_continue_flag || adsClient.getIntVal(0x5EB08) != 0
        || adsClient.getIntVal(0x67CE0) == 0) {
        qDebug() << "[点动] 扫描中/任务号非0/未使能，已忽略 X-";
        return;
    }
    adsClient.setIntVal(0x5EBDF, 0); // 清 X 停止
    adsClient.setIntVal(0x5EBDB, 0); // 清 X 正向点动（互斥）
    adsClient.setFloatVal(0x5EBE4, ui->doubleSpinBox_2->text().toFloat()); // 设置 x 轴的点动速度 MotorControlVary[0].JogVelocity

    adsClient.setIntVal(0x5EBDC, 1); // 控制 x 轴向后点动 MotorControlVary[0].bJogBackwards
}

// X--松开
void MainWindow1::on_pushButton_24_released()
{
    adsClient.setFloatVal(0x5EBE4, 0); // 设置 x 轴的点动速度 MotorControlVary[0].JogVelocity

    adsClient.setIntVal(0x5EBDC, 0); // 控制 x 轴向后点动 MotorControlVary[0].bJogBackwards
}

// Y++按下
void MainWindow1::on_pushButton_25_pressed()
{
    if (!scan_continue_flag || adsClient.getIntVal(0x5EB08) != 0
        || adsClient.getIntVal(0x67D60) == 0) {
        qDebug() << "[点动] 扫描中/任务号非0/未使能，已忽略 Y+";
        return;
    }
    adsClient.setIntVal(0x5EC2F, 0); // 清 Y 停止
    adsClient.setIntVal(0x5EC2C, 0); // 清 Y 反向点动（互斥）
    adsClient.setFloatVal(0x5EC34, ui->doubleSpinBox->text().toFloat()); // 设置 Y 轴的点动速度 MotorControlVary[2].JogVelocity

    adsClient.setIntVal(0x5EC2B, 1); // 控制 y 轴向前点动 MotorControlVary[2].bJogForwards
}

// Y++松开
void MainWindow1::on_pushButton_25_released()
{
    adsClient.setFloatVal(0x5EC34, 0); // 设置 Y 轴的点动速度 MotorControlVary[2].JogVelocity

    adsClient.setIntVal(0x5EC2B, 0); // 控制 y 轴向前点动 MotorControlVary[2].bJogForwards
}

// Y--按下
void MainWindow1::on_pushButton_26_pressed()
{
    if (!scan_continue_flag || adsClient.getIntVal(0x5EB08) != 0
        || adsClient.getIntVal(0x67D60) == 0) {
        qDebug() << "[点动] 扫描中/任务号非0/未使能，已忽略 Y-";
        return;
    }
    adsClient.setIntVal(0x5EC2F, 0); // 清 Y 停止
    adsClient.setIntVal(0x5EC2B, 0); // 清 Y 正向点动（互斥）
    adsClient.setFloatVal(0x5EC34, ui->doubleSpinBox->text().toFloat()); // 设置 Y 轴的点动速度 MotorControlVary[2].JogVelocity

    adsClient.setIntVal(0x5EC2C, 1); // 控制 y 轴向后点动 MotorControlVary[2].bJogBackwards
}

// Y--松开
void MainWindow1::on_pushButton_26_released()
{
    adsClient.setFloatVal(0x5EC34, 0); // 设置 Y 轴的点动速度 MotorControlVary[2].JogVelocity

    adsClient.setIntVal(0x5EC2C, 0); // 控制 y 轴向后点动 MotorControlVary[2].bJogBackwards
}

// 位姿点一
void MainWindow1::on_pushButton_13_clicked()
{
    recordPosePoint(1);
}

// 位姿点二
void MainWindow1::on_pushButton_14_clicked()
{
    recordPosePoint(2);
}

// 位姿点三
void MainWindow1::on_pushButton_15_clicked()
{
    recordPosePoint(3);
}

// 位姿点四
void MainWindow1::on_pushButton_16_clicked()
{
    recordPosePoint(4);
}

// 保存点位
void MainWindow1::on_pushButton_32_clicked()
{
    savePoseData(true);
}

// 清空点位
void MainWindow1::on_pushButton_35_clicked()
{
    // qDebug() << "用户点击清空点位";

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "清空点位",
        "确定清空点位数据吗？",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
        );

    if (reply == QMessageBox::Yes) {
        // 所有XY置0
        ui->lineEdit_17->setText("0.00");
        ui->lineEdit_22->setText("0.00");
        ui->lineEdit_19->setText("0.00");
        ui->lineEdit_23->setText("0.00");
        ui->lineEdit_20->setText("0.00");
        ui->lineEdit_24->setText("0.00");
        ui->lineEdit_21->setText("0.00");
        ui->lineEdit_25->setText("0.00");

        // 所有A值置0
        QStringList aFieldsFlat = {
            "lineEdit_26","lineEdit_30","lineEdit_34","lineEdit_38","lineEdit_42","lineEdit_46",
            "lineEdit_27","lineEdit_31","lineEdit_35","lineEdit_39","lineEdit_43","lineEdit_47",
            "lineEdit_28","lineEdit_32","lineEdit_36","lineEdit_40","lineEdit_44","lineEdit_48",
            "lineEdit_29","lineEdit_33","lineEdit_37","lineEdit_41","lineEdit_45","lineEdit_49"
        };
        for (auto& name : aFieldsFlat) {
            if (auto *e = findChild<QLineEdit*>(name)) e->setText("0.00");
        }
        qDebug() << "所有点位数据已清空！";
    }
}


// 加载点位
void MainWindow1::on_pushButton_37_clicked()
{
    QString currentFilePath;  // 保存当前选择的文件路径

    while (true) {
        // 让用户选择点位文件
        if (currentFilePath.isEmpty()) {
            currentFilePath = QFileDialog::getOpenFileName(this, "选择点位数据文件",
                                                           QFileInfo(posePath).path(),
                                                           "文本文件 (*.txt)");
            if (currentFilePath.isEmpty()) return;
        }

        // 读取文件中的所有点位记录
        QFile file(currentFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "错误", "无法打开点位文件！");
            return;
        }

        QTextStream in(&file);
        QList<QString> records;
        QList<QString> timeStamps;
        QList<QString> summaries;

        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.isEmpty()) continue;

            int sepIndex = line.indexOf(" >> ");
            if (sepIndex > 0) {
                QString timeStamp = line.left(sepIndex);
                QString record = line.mid(sepIndex + 4);
                records.append(record);
                timeStamps.append(timeStamp);

                QString summary = QString("【%1】\n").arg(timeStamp);
                for (int group = 1; group <= 4; ++group) {
                    QString groupTag = QString("Group%1 <").arg(group);
                    int startPos = record.indexOf(groupTag);
                    if (startPos != -1) {
                        int endPos = record.indexOf(">", startPos);
                        QString groupData = record.mid(startPos + groupTag.length(), endPos - startPos - groupTag.length());

                        int xPos = groupData.indexOf("X=");
                        int yPos = groupData.indexOf("Y=");
                        int a1Pos = groupData.indexOf("A1=");

                        QString xVal = groupData.mid(xPos + 2, yPos - xPos - 3);
                        QString yVal = groupData.mid(yPos + 2, a1Pos - yPos - 3);

                        QString aVals[6];
                        for (int i = 1; i <= 6; ++i) {
                            QString aiTag = QString("A%1=").arg(i);
                            int aiPos = groupData.indexOf(aiTag);
                            if (aiPos != -1) {
                                int nextPos = groupData.indexOf(" ", aiPos + aiTag.length());
                                if (nextPos == -1) nextPos = groupData.length();
                                aVals[i-1] = groupData.mid(aiPos + aiTag.length(), nextPos - aiPos - aiTag.length());
                            }
                        }

                        summary += QString("  点%1: X=%2, Y=%3, A1=%4, A2=%5, A3=%6, A4=%7, A5=%8, A6=%9\n")
                                       .arg(group)
                                       .arg(xVal, 9)
                                       .arg(yVal, 9)
                                       .arg(aVals[0], 7)
                                       .arg(aVals[1], 7)
                                       .arg(aVals[2], 7)
                                       .arg(aVals[3], 7)
                                       .arg(aVals[4], 7)
                                       .arg(aVals[5], 7);
                    }
                }
                summaries.append(summary);
            }
        }
        file.close();

        if (records.isEmpty()) {
            QMessageBox::warning(this, "提示", "文件中没有找到点位数据！");
            return;
        }

        // 创建选择对话框
        QDialog dialog(this);
        QFileInfo fileInfo(currentFilePath);
        dialog.setWindowTitle(QString("%1").arg(fileInfo.fileName()));
        dialog.setFixedSize(850, 650);
        dialog.setModal(true);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(20, 20, 20, 20);

        QLabel *label = new QLabel(QString("共找到 %1 条点位记录，请选择要加载的：").arg(records.size()), &dialog);
        label->setStyleSheet("font-size: 12pt;");
        layout->addWidget(label);

        QListWidget *listWidget = new QListWidget(&dialog);
        for (const QString &summary : summaries) {
            QListWidgetItem *item = new QListWidgetItem(summary);
            item->setFont(QFont("Consolas", 10));
            listWidget->addItem(item);
        }
        listWidget->setCurrentRow(records.size() - 1);
        listWidget->setUniformItemSizes(false);
        layout->addWidget(listWidget);

        // 按钮区域
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();

        QPushButton *loadBtn = new QPushButton("加载", &dialog);
        QPushButton *reSelectBtn = new QPushButton("重选", &dialog);
        QPushButton *deleteBtn = new QPushButton("删除", &dialog);
        QPushButton *viewBtn = new QPushButton("编辑", &dialog);
        QPushButton *cancelBtn = new QPushButton("取消", &dialog);

        reSelectBtn->setFixedSize(100, 35);
        deleteBtn->setFixedSize(100, 35);
        viewBtn->setFixedSize(100, 35);
        loadBtn->setFixedSize(100, 35);
        cancelBtn->setFixedSize(100, 35);

        btnLayout->addWidget(loadBtn);
        btnLayout->addWidget(reSelectBtn);
        btnLayout->addWidget(deleteBtn);
        btnLayout->addWidget(viewBtn);
        btnLayout->addWidget(cancelBtn);
        btnLayout->addStretch();
        layout->addLayout(btnLayout);

        bool reselect = false;

        // 加载按钮
        connect(loadBtn, &QPushButton::clicked, [&]() {
            int row = listWidget->currentRow();
            if (row >= 0 && row < records.size()) {
                QString record = records[row];

                for (int group = 1; group <= 4; ++group) {
                    QString groupTag = QString("Group%1 <").arg(group);
                    int startPos = record.indexOf(groupTag);
                    if (startPos == -1) continue;

                    int endPos = record.indexOf(">", startPos);
                    if (endPos == -1) continue;

                    QString groupData = record.mid(startPos + groupTag.length(), endPos - startPos - groupTag.length());

                    int xPos = groupData.indexOf("X=");
                    int yPos = groupData.indexOf("Y=");
                    int a1Pos = groupData.indexOf("A1=");

                    float x = groupData.mid(xPos + 2, yPos - xPos - 3).toFloat();
                    float y = groupData.mid(yPos + 2, a1Pos - yPos - 3).toFloat();

                    float A[6] = {0};
                    for (int i = 1; i <= 6; ++i) {
                        QString aiTag = QString("A%1=").arg(i);
                        int aiPos = groupData.indexOf(aiTag);
                        if (aiPos != -1) {
                            int nextPos = groupData.indexOf(" ", aiPos + aiTag.length());
                            if (nextPos == -1) nextPos = groupData.length();
                            A[i-1] = groupData.mid(aiPos + aiTag.length(), nextPos - aiPos - aiTag.length()).toFloat();
                        }
                    }

                    int idx = group - 1;

                    QStringList xyFields = {
                        "lineEdit_17", "lineEdit_22", "lineEdit_19", "lineEdit_23",
                        "lineEdit_20", "lineEdit_24", "lineEdit_21", "lineEdit_25"
                    };

                    QStringList aFields = {
                        "lineEdit_26", "lineEdit_30", "lineEdit_34", "lineEdit_38", "lineEdit_42", "lineEdit_46",
                        "lineEdit_27", "lineEdit_31", "lineEdit_35", "lineEdit_39", "lineEdit_43", "lineEdit_47",
                        "lineEdit_28", "lineEdit_32", "lineEdit_36", "lineEdit_40", "lineEdit_44", "lineEdit_48",
                        "lineEdit_29", "lineEdit_33", "lineEdit_37", "lineEdit_41", "lineEdit_45", "lineEdit_49"
                    };

                    QLineEdit *xEdit = findChild<QLineEdit*>(xyFields[idx * 2]);
                    QLineEdit *yEdit = findChild<QLineEdit*>(xyFields[idx * 2 + 1]);
                    if (xEdit) xEdit->setText(QString::number(x, 'f', 2));
                    if (yEdit) yEdit->setText(QString::number(y, 'f', 2));

                    for (int i = 0; i < 6; ++i) {
                        QLineEdit *aEdit = findChild<QLineEdit*>(aFields[idx * 6 + i]);
                        if (aEdit) aEdit->setText(QString::number(A[i], 'f', 2));
                    }
                }

                // qDebug()<< QString("已加载点位记录：%1").arg(timeStamps[row]);
                qDebug()<< "已加载点位记录：" << timeStamps[row];
            }
            dialog.accept();
        });

        // 重选文件按钮
        connect(reSelectBtn, &QPushButton::clicked, [&]() {
            reselect = true;
            dialog.accept();
        });

        // 删除按钮 - 删除前确认，删除后不提醒
        connect(deleteBtn, &QPushButton::clicked, [&]() {
            int row = listWidget->currentRow();
            if (row < 0 || row >= records.size()) {
                QMessageBox::warning(&dialog, "提示", "请先选择要删除的记录！");
                return;
            }

            // 确认删除
            int ret = QMessageBox::question(&dialog, "确认删除",
                                            QString("确定要删除以下记录吗？\n\n%1")
                                                .arg(timeStamps[row]),
                                            QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes) return;

            // 从列表中移除选中的记录
            records.removeAt(row);
            timeStamps.removeAt(row);
            summaries.removeAt(row);

            // 立即保存到文件
            QFile saveFile(currentFilePath);
            if (saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&saveFile);
                for (int i = 0; i < records.size(); ++i) {
                    out << timeStamps[i] << " >> " << records[i];
                    if (i < records.size() - 1) {
                        out << "\n\n";
                    }
                }
                saveFile.close();
                qDebug() << QString("已删除记录并保存文件");
            } else {
                QMessageBox::warning(&dialog, "错误", "无法保存文件！");
                return;
            }

            // 更新列表显示
            listWidget->clear();
            for (const QString &summary : summaries) {
                QListWidgetItem *item = new QListWidgetItem(summary);
                item->setFont(QFont("Consolas", 10));
                listWidget->addItem(item);
            }

            // 更新标签显示
            label->setText(QString("共找到 %1 条点位记录，请选择要加载的：").arg(records.size()));

            // 如果删除后没有记录了，关闭对话框
            if (records.isEmpty()) {
                QMessageBox::information(&dialog, "提示", "所有记录已删除，文件已清空。");
                dialog.accept();
                return;
            }

            // 选中上一条或下一条
            if (row >= records.size()) {
                listWidget->setCurrentRow(records.size() - 1);
            } else {
                listWidget->setCurrentRow(row);
            }

            // 删除后不弹出成功提醒，静默完成
        });

        // 查看文件按钮
        connect(viewBtn, &QPushButton::clicked, [&]() {
            QProcess::startDetached("notepad.exe", QStringList() << currentFilePath);
        });

        connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

        dialog.exec();

        if (!reselect) break;  // 如果不是重选文件，退出循环
        currentFilePath.clear();  // 重选文件，清空路径重新选择
    }
}


// // 加载点位
// void MainWindow1::on_pushButton_37_clicked()
// {
//     QString currentFilePath;  // 保存当前选择的文件路径

//     while (true) {
//         // 让用户选择点位文件
//         if (currentFilePath.isEmpty()) {
//             currentFilePath = QFileDialog::getOpenFileName(this, "选择点位数据文件",
//                                                            QFileInfo(posePath).path(),
//                                                            "文本文件 (*.txt)");
//             if (currentFilePath.isEmpty()) return;
//         }

//         // 读取文件中的所有点位记录
//         QFile file(currentFilePath);
//         if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//             QMessageBox::warning(this, "错误", "无法打开点位文件！");
//             return;
//         }

//         QTextStream in(&file);
//         QList<QString> records;
//         QList<QString> timeStamps;
//         QList<QString> summaries;

//         while (!in.atEnd()) {
//             QString line = in.readLine();
//             if (line.isEmpty()) continue;

//             int sepIndex = line.indexOf(" >> ");
//             if (sepIndex > 0) {
//                 QString timeStamp = line.left(sepIndex);
//                 QString record = line.mid(sepIndex + 4);
//                 records.append(record);
//                 timeStamps.append(timeStamp);

//                 QString summary = QString("【%1】\n").arg(timeStamp);
//                 for (int group = 1; group <= 4; ++group) {
//                     QString groupTag = QString("Group%1 <").arg(group);
//                     int startPos = record.indexOf(groupTag);
//                     if (startPos != -1) {
//                         int endPos = record.indexOf(">", startPos);
//                         QString groupData = record.mid(startPos + groupTag.length(), endPos - startPos - groupTag.length());

//                         int xPos = groupData.indexOf("X=");
//                         int yPos = groupData.indexOf("Y=");
//                         int a1Pos = groupData.indexOf("A1=");

//                         QString xVal = groupData.mid(xPos + 2, yPos - xPos - 3);
//                         QString yVal = groupData.mid(yPos + 2, a1Pos - yPos - 3);

//                         QString aVals[6];
//                         for (int i = 1; i <= 6; ++i) {
//                             QString aiTag = QString("A%1=").arg(i);
//                             int aiPos = groupData.indexOf(aiTag);
//                             if (aiPos != -1) {
//                                 int nextPos = groupData.indexOf(" ", aiPos + aiTag.length());
//                                 if (nextPos == -1) nextPos = groupData.length();
//                                 aVals[i-1] = groupData.mid(aiPos + aiTag.length(), nextPos - aiPos - aiTag.length());
//                             }
//                         }

//                         summary += QString("  点%1: X=%2, Y=%3, A1=%4, A2=%5, A3=%6, A4=%7, A5=%8, A6=%9\n")
//                                        .arg(group)
//                                        .arg(xVal, 9)
//                                        .arg(yVal, 9)
//                                        .arg(aVals[0], 7)
//                                        .arg(aVals[1], 7)
//                                        .arg(aVals[2], 7)
//                                        .arg(aVals[3], 7)
//                                        .arg(aVals[4], 7)
//                                        .arg(aVals[5], 7);
//                     }
//                 }
//                 summaries.append(summary);
//             }
//         }
//         file.close();

//         if (records.isEmpty()) {
//             QMessageBox::warning(this, "提示", "文件中没有找到点位数据！");
//             return;
//         }

//         // 创建选择对话框
//         QDialog dialog(this);
//         QFileInfo fileInfo(currentFilePath);
//         // dialog.setWindowTitle(QString("选择要加载的点位记录 - %1").arg(fileInfo.fileName()));
//         dialog.setWindowTitle(QString("%1").arg(fileInfo.fileName()));
//         dialog.setFixedSize(850, 650);
//         dialog.setModal(true);

//         QVBoxLayout *layout = new QVBoxLayout(&dialog);
//         layout->setContentsMargins(20, 20, 20, 20);

//         QLabel *label = new QLabel(QString("共找到 %1 条点位记录，请选择要加载的：").arg(records.size()), &dialog);
//         label->setStyleSheet("font-size: 12pt;");
//         layout->addWidget(label);

//         QListWidget *listWidget = new QListWidget(&dialog);
//         for (const QString &summary : summaries) {
//             QListWidgetItem *item = new QListWidgetItem(summary);
//             item->setFont(QFont("Consolas", 10));
//             listWidget->addItem(item);
//         }
//         listWidget->setCurrentRow(records.size() - 1);
//         listWidget->setUniformItemSizes(false);
//         layout->addWidget(listWidget);

//         // 按钮区域
//         QHBoxLayout *btnLayout = new QHBoxLayout();
//         btnLayout->addStretch();

//         QPushButton *loadBtn = new QPushButton("加载", &dialog);
//         QPushButton *reSelectBtn = new QPushButton("重选", &dialog);
//         QPushButton *viewBtn = new QPushButton("编辑", &dialog);
//         QPushButton *cancelBtn = new QPushButton("取消", &dialog);

//         reSelectBtn->setFixedSize(100, 35);
//         viewBtn->setFixedSize(100, 35);
//         loadBtn->setFixedSize(100, 35);
//         cancelBtn->setFixedSize(100, 35);


//         btnLayout->addWidget(loadBtn);
//         btnLayout->addWidget(reSelectBtn);
//         btnLayout->addWidget(viewBtn);
//         btnLayout->addWidget(cancelBtn);
//         btnLayout->addStretch();
//         layout->addLayout(btnLayout);

//         bool reselect = false;


//         // 加载按钮
//         connect(loadBtn, &QPushButton::clicked, [&]() {
//             int row = listWidget->currentRow();
//             if (row >= 0 && row < records.size()) {
//                 QString record = records[row];

//                 for (int group = 1; group <= 4; ++group) {
//                     QString groupTag = QString("Group%1 <").arg(group);
//                     int startPos = record.indexOf(groupTag);
//                     if (startPos == -1) continue;

//                     int endPos = record.indexOf(">", startPos);
//                     if (endPos == -1) continue;

//                     QString groupData = record.mid(startPos + groupTag.length(), endPos - startPos - groupTag.length());

//                     int xPos = groupData.indexOf("X=");
//                     int yPos = groupData.indexOf("Y=");
//                     int a1Pos = groupData.indexOf("A1=");

//                     float x = groupData.mid(xPos + 2, yPos - xPos - 3).toFloat();
//                     float y = groupData.mid(yPos + 2, a1Pos - yPos - 3).toFloat();

//                     float A[6] = {0};
//                     for (int i = 1; i <= 6; ++i) {
//                         QString aiTag = QString("A%1=").arg(i);
//                         int aiPos = groupData.indexOf(aiTag);
//                         if (aiPos != -1) {
//                             int nextPos = groupData.indexOf(" ", aiPos + aiTag.length());
//                             if (nextPos == -1) nextPos = groupData.length();
//                             A[i-1] = groupData.mid(aiPos + aiTag.length(), nextPos - aiPos - aiTag.length()).toFloat();
//                         }
//                     }

//                     int idx = group - 1;

//                     QStringList xyFields = {
//                         "lineEdit_17", "lineEdit_22", "lineEdit_19", "lineEdit_23",
//                         "lineEdit_20", "lineEdit_24", "lineEdit_21", "lineEdit_25"
//                     };

//                     QStringList aFields = {
//                         "lineEdit_26", "lineEdit_30", "lineEdit_34", "lineEdit_38", "lineEdit_42", "lineEdit_46",
//                         "lineEdit_27", "lineEdit_31", "lineEdit_35", "lineEdit_39", "lineEdit_43", "lineEdit_47",
//                         "lineEdit_28", "lineEdit_32", "lineEdit_36", "lineEdit_40", "lineEdit_44", "lineEdit_48",
//                         "lineEdit_29", "lineEdit_33", "lineEdit_37", "lineEdit_41", "lineEdit_45", "lineEdit_49"
//                     };

//                     QLineEdit *xEdit = findChild<QLineEdit*>(xyFields[idx * 2]);
//                     QLineEdit *yEdit = findChild<QLineEdit*>(xyFields[idx * 2 + 1]);
//                     if (xEdit) xEdit->setText(QString::number(x, 'f', 2));
//                     if (yEdit) yEdit->setText(QString::number(y, 'f', 2));

//                     for (int i = 0; i < 6; ++i) {
//                         QLineEdit *aEdit = findChild<QLineEdit*>(aFields[idx * 6 + i]);
//                         if (aEdit) aEdit->setText(QString::number(A[i], 'f', 2));
//                     }
//                 }

//                 qDebug()<< QString("已加载点位记录：%1").arg(timeStamps[row]);
//                 // QMessageBox::information(&dialog, "成功", QString("已加载点位记录：%1").arg(timeStamps[row]));
//             }
//             dialog.accept();
//         });

//         // 重选文件按钮
//         connect(reSelectBtn, &QPushButton::clicked, [&]() {
//             reselect = true;
//             dialog.accept();
//         });

//         // 查看文件按钮
//         connect(viewBtn, &QPushButton::clicked, [&]() {
//             QProcess::startDetached("notepad.exe", QStringList() << currentFilePath);
//         });


//         connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

//         dialog.exec();

//         if (!reselect) break;  // 如果不是重选文件，退出循环
//         currentFilePath.clear();  // 重选文件，清空路径重新选择
//     }
// }

// 打开日志
void MainWindow1::on_pushButton_41_clicked()
{
    if (!QFile::exists(logPath)) {
        QMessageBox::warning(this, "日志不存在", "未找到系统日志文件！");
        return;
    }

    QProcess::startDetached("notepad.exe", QStringList() << logPath);
}

// 打开仿真（快捷指令）
void MainWindow1::on_pushButton_29_clicked()
{
    QMetaObject::invokeMethod(m_worker, "doOpenSimulation", Qt::QueuedConnection);
}

// 关闭仿真（快捷指令）
void MainWindow1::on_pushButton_30_clicked()
{
    QMetaObject::invokeMethod(m_worker, "doCloseSimulation", Qt::QueuedConnection);
}

// 后置处理（快捷指令）
void MainWindow1::on_pushButton_31_clicked()
{
    QSettings settings("YourCompany", "YourApp");

    QString lastInputPath = settings.value("PostProcess/LastInputPath", "").toString();
    QString lastOutputPath = settings.value("PostProcess/LastOutputPath", "").toString();

    QStringList inputFiles = QFileDialog::getOpenFileNames(this, "选择 .dat 和 .src 文件",
                                                           lastInputPath,
                                                           "DAT/SRC文件 (*.dat *.src)");
    if (inputFiles.isEmpty()) return;

    if (!inputFiles.isEmpty()) {
        QFileInfo firstFile(inputFiles.first());
        settings.setValue("PostProcess/LastInputPath", firstFile.absolutePath());
    }

    QString outputDir = QFileDialog::getExistingDirectory(this, "选择保存目录",
                                                          lastOutputPath);
    if (outputDir.isEmpty()) return;

    settings.setValue("PostProcess/LastOutputPath", outputDir);

    if (!outputDir.endsWith('/') && !outputDir.endsWith('\\')) {
    }

    for (const QString& inputFile : inputFiles) {
        QFile inFile(inputFile);
        QFileInfo fileInfo(inputFile);
        QString baseName = fileInfo.completeBaseName(); // 获取不带扩展名的文件名
        QString suffix = fileInfo.suffix(); // 获取扩展名
        // 构建新文件名：原始名称 + "_New" + 扩展名
        QString outputFilePath = outputDir + "/" + baseName + "_New." + suffix;

        QFile outFile(outputFilePath);

        if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "错误", "无法打开文件：" + inputFile);
            continue;
        }

        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "错误", "无法创建输出文件：" + outputFilePath);
            inFile.close();
            continue;
        }

        QTextStream in(&inFile);
        QTextStream out(&outFile);
        QStringList lines;
        while (!in.atEnd()) {
            lines << in.readLine();
        }
        inFile.close();

        if (inputFile.endsWith(".dat", Qt::CaseInsensitive)) {
            for (QString line : lines) {
                if (line.contains("DECL E6POS") && line.contains("s ")) {
                    int sIndex = line.indexOf("s ");
                    if (sIndex != -1) {
                        int commaIndex = line.indexOf(",", sIndex);
                        if (commaIndex != -1) {
                            line.replace(sIndex, commaIndex - sIndex, "s 0");
                        }
                    }
                }

                // 新增：将 TOOL_NO 0 替换为 TOOL_NO 1
                if (line.contains("TOOL_NO 0")) {
                    line.replace("TOOL_NO 0", "TOOL_NO 1");
                }

                out << line << "\n";
            }
        }
        else if (inputFile.endsWith(".src", Qt::CaseInsensitive)) {
            bool insideFunction = false;
            bool isTag1Block = false;

            for (int i = 0; i < lines.size(); ++i) {
                QString line = lines[i];
                QString trimmed = line.trimmed();

                // ===== 移除 Tag0 行的 CONT =====
                if (line.contains(";FOLD LIN Tag0") && line.contains("CONT")) {
                    line.replace(" CONT ", " ");
                }

                // ===== 移除 Tag0 行的 CONT =====
                if (line.contains(";FOLD LIN Tag1 CONT") && line.contains("Vel= 0.5 m/s")) {
                    isTag1Block = true;
                    line.replace("Vel= 0.5 m/s", "Vel= 0.1 m/s");
                }

                // 替换 %P 参数字段中的速度值
                if (line.contains(";FOLD LIN Tag1 CONT") && line.contains("%P")) {
                    QRegularExpression pVelRegex(R"(5:\s*([\d.]+))");
                    line.replace(pVelRegex, QString("5:%1").arg(0.1));
                }

                if (trimmed == ";ENDFOLD") {
                    isTag1Block = false; // 退出当前块
                }

                // ===== 新增：修改 BAS(#VEL_CP,0.5) =====
                // 只有在 Tag1 块内部，并且当前行包含 BAS(#VEL_CP 时才修改
                if (isTag1Block && line.contains("BAS(#VEL_CP,")) {
                    // 将 0.5 替换为 0.2。使用正则表达式更精确，但简单替换也可行
                    line.replace("BAS(#VEL_CP,0.5)", "BAS(#VEL_CP,0.1)");
                }

                // 新增：将 Tool[0] 替换为 Tool[1]
                if (line.contains("Tool[0]")) {
                    line.replace("Tool[0]", "Tool[1]");
                }

                // 开启 RSI
                if (trimmed.startsWith("DEF ") && trimmed.contains("(")) {
                    insideFunction = true;
                    out << line << "\n";
                    out << rsiStartCode << "\n";
                    continue;
                }

                // 关闭 RSI
                if (trimmed == "END" && insideFunction) {
                    out << rsiEndCode << "\n";
                    out << line << "\n";
                    insideFunction = false;
                    continue;
                }

                // 其他行正常写入
                out << line << "\n";

                // ===== 在 ;ENDFOLD 和 ;FOLD LIN Tag1 之间插入 $OUT[100]=FALSE =====
                bool isEndFold = line.trimmed() == ";ENDFOLD";
                bool nextIsTag1 = (i + 1 < lines.size()) && lines[i + 1].contains(";FOLD LIN Tag1");

                if (isEndFold && nextIsTag1) {
                    QString nextLine = lines[i + 1];
                    QRegularExpression tag1Regex(";FOLD LIN Tag1\\b");
                    if (tag1Regex.match(nextLine).hasMatch()) {
                        out << "$OUT[100]=FALSE\n";
                    }
                }
            }
        }

        outFile.close();
    }

    QMessageBox::information(this, "处理完成", "所有文件处理完成！");
}

// // 后置处理（快捷指令）
// void MainWindow1::on_pushButton_31_clicked()
// {
//     QStringList inputFiles = QFileDialog::getOpenFileNames(this, "选择 .dat 和 .src 文件", "", "DAT/SRC文件 (*.dat *.src)");
//     if (inputFiles.isEmpty()) return;

//     QString outputDir = QFileDialog::getExistingDirectory(this, "选择保存目录");
//     if (outputDir.isEmpty()) return;

//     int datCount = 0;
//     int srcInsertCount = 0;
//     int toolNoReplaceCount = 0;   // 统计 TOOL_NO 替换次数
//     int toolArrayReplaceCount = 0; // 统计 Tool[0] 替换次数

//     for (const QString& inputFile : inputFiles) {
//         QFile inFile(inputFile);
//         QFileInfo fileInfo(inputFile);
//         QString baseName = fileInfo.completeBaseName(); // 获取不带扩展名的文件名
//         QString suffix = fileInfo.suffix(); // 获取扩展名
//         // 构建新文件名：原始名称 + "_New" + 扩展名
//         QString outputFilePath = outputDir + "/" + baseName + "_New." + suffix;

//         QFile outFile(outputFilePath);

//         if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
//             QMessageBox::warning(this, "错误", "无法打开文件：" + inputFile);
//             continue;
//         }

//         if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
//             QMessageBox::warning(this, "错误", "无法创建输出文件：" + outputFilePath);
//             inFile.close();
//             continue;
//         }

//         QTextStream in(&inFile);
//         QTextStream out(&outFile);
//         QStringList lines;
//         while (!in.atEnd()) {
//             lines << in.readLine();
//         }
//         inFile.close();

//         if (inputFile.endsWith(".dat", Qt::CaseInsensitive)) {
//             for (QString line : lines) {
//                 if (line.contains("DECL E6POS") && line.contains("s ")) {
//                     int sIndex = line.indexOf("s ");
//                     if (sIndex != -1) {
//                         int commaIndex = line.indexOf(",", sIndex);
//                         if (commaIndex != -1) {
//                             line.replace(sIndex, commaIndex - sIndex, "s 0");
//                             datCount++;
//                         }
//                     }
//                 }

//                 // 新增：将 TOOL_NO 0 替换为 TOOL_NO 1
//                 if (line.contains("TOOL_NO 0")) {
//                     line.replace("TOOL_NO 0", "TOOL_NO 1");
//                     toolNoReplaceCount++;
//                 }

//                 out << line << "\n";
//             }
//         }
//         else if (inputFile.endsWith(".src", Qt::CaseInsensitive)) {
//             bool insideFunction = false;
//             double targetVel = 0.5; // 可配置目标速度

//             for (int i = 0; i < lines.size(); ++i) {
//                 QString line = lines[i];
//                 QString trimmed = line.trimmed();

//                 // ===== 移除 Tag0 行的 CONT =====
//                 if (line.contains(";FOLD LIN Tag0") && line.contains("CONT")) {
//                     line.replace(" CONT ", " ");
//                 }

//                 if (line.contains("LIN XTag0 C_VEL")) {
//                     line.replace(" C_VEL ", " ");
//                 }

//                 // 替换速度
//                 if (line.contains(";FOLD LIN") && line.contains("Vel=")) {
//                     // 替换速度值
//                     QRegularExpression velRegex(R"(Vel=\s*([\d.]+))");
//                     line.replace(velRegex, QString("Vel= %1").arg(targetVel));
//                 }

//                 // 替换 %P 参数字段中的速度值（如 5:0.75 → 5:0.5）
//                 if (line.contains(";FOLD LIN") && line.contains("%P")) {
//                     QRegularExpression pVelRegex(R"(5:\s*([\d.]+))");
//                     line.replace(pVelRegex, QString("5:%1").arg(targetVel));
//                 }

//                 // 同步替换 BAS(#VEL_CP,...) 行
//                 if (line.contains("BAS(#VEL_CP")) {
//                     QRegularExpression velSetRegex(R"(BAS\(#VEL_CP\s*,\s*([\d.]+)\))");
//                     line.replace(velSetRegex, QString("BAS(#VEL_CP,%1)").arg(targetVel));
//                 }

//                 // 新增：将 Tool[0] 替换为 Tool[1]
//                 if (line.contains("Tool[0]")) {
//                     line.replace("Tool[0]", "Tool[1]");
//                     toolArrayReplaceCount++;
//                 }

//                 // 检测函数定义
//                 if (trimmed.startsWith("DEF ") && trimmed.contains("(")) {
//                     insideFunction = true;
//                     out << line << "\n";
//                     out << rsiStartCode << "\n";
//                     srcInsertCount++;
//                     continue;
//                 }

//                 // 函数体结束
//                 if (trimmed == "END" && insideFunction) {
//                     out << rsiEndCode << "\n";
//                     out << line << "\n";
//                     insideFunction = false;
//                     srcInsertCount++;
//                     continue;
//                 }

//                 // 其他行正常写入
//                 out << line << "\n";

//                 // ===== 在 ;ENDFOLD 和 ;FOLD LIN Tag1 之间插入 $OUT[100]=FALSE =====
//                 bool isEndFold = line.trimmed() == ";ENDFOLD";
//                 bool nextIsTag1 = (i + 1 < lines.size()) && lines[i + 1].contains(";FOLD LIN Tag1");

//                 if (isEndFold && nextIsTag1) {
//                     out << "$OUT[100]=FALSE\n";
//                     srcInsertCount++;
//                     qDebug() << "[后置处理] 在 ENDFOLD 和 Tag1 之间插入 $OUT[100]=FALSE";
//                 }
//             }
//         }

//         outFile.close();
//     }

//     QMessageBox::information(this, "处理完成",
//                              QString("所有文件处理完成！\n"
//                                      "共修改 %1 个 .dat 点位的 s 参数。\n"
//                                      "共替换 %2 个 TOOL_NO 0 -> TOOL_NO 1。\n"
//                                      "共替换 %3 个 Tool[0] -> Tool[1]。\n"
//                                      "插入了 %4 个 .src RSI 代码段。")
//                                  .arg(datCount).arg(toolNoReplaceCount).arg(toolArrayReplaceCount).arg(srcInsertCount));
// }

// 退出系统（快捷指令）
void MainWindow1::on_pushButton_34_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认退出", "确定要退出吗？",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QApplication::quit();
    }
}

// 采集帧率
void MainWindow1::on_PRFSpinBox_valueChanged(double prf)
{
    if (!isBlockSignal[ui->PRFSpinBox]) {
        config.setFrameRate(static_cast<int32_t>(prf));
    }
}

// 最大幅值
void MainWindow1::on_MamplitudeBox_currentIndexChanged(int index)
{
    config.setMaxAmplitude(indexToAmplitude[ui->MamplitudeBox->currentIndex()]);
    emit App::getInstance()->signal_GateView_Refresh();
    emit App::getInstance()->refresh_Allpara();
}

// 增益
void MainWindow1::on_GainSpinBox_valueChanged(double gain)
{
    if (!isBlockSignal[ui->GainSpinBox]) {
        config.setGain(gain);
    }
}

// 范围起点
void MainWindow1::on_RangstartSpinBox_valueChanged(double start)
{
    if (!isBlockSignal[ui->RangstartSpinBox]) {
        config.setRangeStart(start);
        getcurrentPara();
        emit App::getInstance()->signal_RangeChanged();
    }
}

// 范围终点
void MainWindow1::on_RangendSpinBox_valueChanged(double end)
{
    if (!isBlockSignal[ui->RangendSpinBox]) {
        config.setRangeEnd(end);
        getcurrentPara();
        emit App::getInstance()->signal_RangeChanged();
    }
}

// 低通滤波器
void MainWindow1::on_FilterLowBox_currentIndexChanged(int index)
{
    if (index == 0) {
        config.setFilterHigh(-1);
    }
    config.setFilterLow(ui->FilterLowBox->currentText().toFloat());
}

// 高通滤波器
void MainWindow1::on_FilterHighBox_currentIndexChanged(int index)
{
    if (index == 0) {
        config.setFilterHigh(-1);
    }
    config.setFilterHigh(ui->FilterHighBox->currentText().toFloat());
}

// 图像滤波
void MainWindow1::on_vedioFilterBox_currentIndexChanged(int index)
{
    if (index >= 0 && index < videoFilterCount) {
        config.setVideoFilterMHz(indexToVideoFilter[index]);
    }
    emit App::getInstance()->refresh_Allpara();
}

// 检波模式
void MainWindow1::on_RectifierBox_currentIndexChanged(int index)
{
    config.setRectifierMode(static_cast<RectifierType>(index));
    emit App::getInstance()->signal_RectifierChanged();
    emit App::getInstance()->refresh_Allpara();
    emit App::getInstance()->signal_showhideGatestatus();
}

// 闸门使能关闭
void MainWindow1::on_Btn_right_clicked()
{
    if (currentGate == GATE::GATE_I) {
        config.setGateIEnable(false);
        if (config.getGateASynchronMode() == GateSynchron::GateI) {
            config.setGateASynchronMode(GateSynchron::Pulser);
        }
        if (config.getGateBSynchronMode() == GateSynchron::GateI) {
            config.setGateBSynchronMode(GateSynchron::Pulser);
        }
        if (config.getGateCSynchronMode() == GateSynchron::GateI) {
            config.setGateCSynchronMode(GateSynchron::Pulser);
        }
        if (config.getGateASynchronMode() != GateSynchron::GateI
            && config.getGateBSynchronMode() != GateSynchron::GateI
            && config.getGateCSynchronMode() != GateSynchron::GateI) {
            config.setGateIMeasureType(measureType::MaxPeak);
        }
    } else if (currentGate == GATE::GATE_A) {
        config.setGateAEnable(false);
        if (config.getGateBSynchronMode() == GateSynchron::GateA) {
            config.setGateBSynchronMode(GateSynchron::Pulser);
        }
        if (config.getGateCSynchronMode() == GateSynchron::GateA) {
            config.setGateCSynchronMode(GateSynchron::Pulser);
        }
    } else if (currentGate == GATE::GATE_B) {
        config.setGateBEnable(false);
        if (config.getGateCSynchronMode() == GateSynchron::GateB) {
            config.setGateCSynchronMode(GateSynchron::Pulser);
        }
    } else if (currentGate == GATE::GATE_C) {
        config.setGateCEnable(false);
    }
    emit App::getInstance()->signal_showhideGatestatus();
}

// 闸门使能开启
void MainWindow1::on_Btn_left_clicked()
{
    if (currentGate == GATE::GATE_I) {
        config.setGateIEnable(true);
    } else if (currentGate == GATE::GATE_A) {
        config.setGateAEnable(true);
    } else if (currentGate == GATE::GATE_B) {
        config.setGateBEnable(true);
    } else if (currentGate == GATE::GATE_C) {
        config.setGateCEnable(true);
    }
    emit App::getInstance()->signal_showhideGatestatus();
}

// 闸门测量方法
void MainWindow1::on_MeasureBox_currentIndexChanged(int index)
{
    if (currentGate == GATE::GATE_I) {
        config.setGateIMeasureType(static_cast<measureType>(index));
    } else if (currentGate == GATE::GATE_A) {
        config.setGateAMeasureType(static_cast<measureType>(index));
    } else if (currentGate == GATE::GATE_B) {
        config.setGateBMeasureType(static_cast<measureType>(index));
    } else if (currentGate == GATE::GATE_C) {
        config.setGateCMeasureType(static_cast<measureType>(index));
    }
}

// 闸门开始
void MainWindow1::on_StartSpinBox_valueChanged(double start)
{
    if (!isBlockSignal[ui->StartSpinBox]) {
        if (currentGate == GATE::GATE_I) {
            auto newEnd = ui->WidthSpinBox->value() + start;
            config.setGateIEnd(newEnd);
            config.setGateIStart(start);
        } else if (currentGate == GATE::GATE_A) {
            auto newEnd = ui->WidthSpinBox->value() + start;
            config.setGateAEnd(newEnd);
            config.setGateAStart(start);
        } else if (currentGate == GATE::GATE_B) {
            auto newEnd = ui->WidthSpinBox->value() + start;
            config.setGateBEnd(newEnd);
            config.setGateBStart(start);
        } else if (currentGate == GATE::GATE_C) {
            auto newEnd = ui->WidthSpinBox->value() + start;
            config.setGateCEnd(newEnd);
            config.setGateCStart(start);
        }
        emit App::getInstance()->signal_GateView_Refresh();
    }
}

// 闸门宽度
void MainWindow1::on_WidthSpinBox_valueChanged(double width)
{
    if (!isBlockSignal[ui->WidthSpinBox]) {
        if (currentGate == GATE::GATE_I) {
            config.setGateIEnd(width + config.getGateIStart());
        } else if (currentGate == GATE::GATE_A) {
            config.setGateAEnd(width + config.getGateAStart());
        } else if (currentGate == GATE::GATE_B) {
            config.setGateBEnd(width + config.getGateBStart());
        } else if (currentGate == GATE::GATE_C) {
            config.setGateCEnd(width + config.getGateCStart());
        }
        emit App::getInstance()->signal_GateView_Refresh();
    }
}

// 闸门阈值
void MainWindow1::on_ThresholdSpinBox_valueChanged(double threshold)
{
    if (!isBlockSignal[ui->ThresholdSpinBox]) {
        auto value = threshold;
        if (value <= 0) {
            value = 0;
            ui->ThresholdSpinBox->setValue(0);
        } else if (value > 100) {
            value = 100;
            ui->ThresholdSpinBox->setValue(100);
        }

        if (currentGate == GATE::GATE_I) {
            config.setGateIThreshold(value);
        } else if (currentGate == GATE::GATE_A) {
            config.setGateAThreshold(value);
        } else if (currentGate == GATE::GATE_B) {
            config.setGateBThreshold(value);
        } else if (currentGate == GATE::GATE_C) {
            config.setGateCThreshold(value);
        }
        emit App::getInstance()->signal_GateView_Refresh();

        if (currentGate == GATE::GATE_I) {
            ui->WidthSpinBox->setValue(config.getGateIEnd() - config.getGateIStart());
        } else if (currentGate == GATE::GATE_A) {
            ui->WidthSpinBox->setValue(config.getGateAEnd() - config.getGateAStart());
        } else if (currentGate == GATE::GATE_B) {
            ui->WidthSpinBox->setValue(config.getGateBEnd() - config.getGateBStart());
        } else if (currentGate == GATE::GATE_C) {
            ui->WidthSpinBox->setValue(config.getGateCEnd() - config.getGateCStart());
        }
    }
}

// 激发电压
void MainWindow1::on_PaVoltageSpinBox_valueChanged(double PaVoltage)
{
    if (!isBlockSignal[ui->PaVoltageSpinBox]) {
        config.setPaVoltage(static_cast<int32_t>(PaVoltage));
    }
}

// 中心频率
void MainWindow1::on_ProbeFrequencySpinBox_valueChanged(double ProbeFrequency)
{
    if (!isBlockSignal[ui->ProbeFrequencySpinBox]) {
        config.setProbeFrequency(ProbeFrequency);
    }
}

// 脉冲宽度
void MainWindow1::on_PwidthSpinBox_valueChanged(double PulseWidth)
{
    if (!isBlockSignal[ui->PwidthSpinBox]) {
        config.setPulseWidth(static_cast<int32_t>(PulseWidth));
    }
}

// 同步模式
void MainWindow1::on_SyncBox_currentIndexChanged(int index)
{
    if (currentGate == GATE::GATE_I) {
    } else if (currentGate == GATE::GATE_A) {
        // index: 0=Pulser, 1=GateI
        if (index == 1 && config.getGateIEnable()) {
            // 闸门I使能时，闸门A可同步GateI
            config.setGateASynchronMode(GateSynchron::GateI);
        } else {
            // 闸门I未使能时，回退到Pulser
            config.setGateASynchronMode(GateSynchron::Pulser);
            if (index == 1) {
                // UI修正：闸门I未使能，回退SyncBox显示
                ui->SyncBox->blockSignals(true);
                ui->SyncBox->setCurrentIndex(0);
                ui->SyncBox->blockSignals(false);
            }
        }
        auto newStart = ui->StartSpinBox->value();
        auto newEnd = ui->WidthSpinBox->value() + newStart;
        bool flag = config.setGateAEnd(newEnd);
        if (flag == false) {
            config.setGateAStart(newStart);
            config.setGateAEnd(newEnd);
        }
        config.setGateAStart(newStart);

    } else if (currentGate == GATE::GATE_B) {
        // index: 0=Pulser, 1=GateI, 2=GateA
        if (index == 1 && config.getGateIEnable()) {
            // 闸门I使能时，闸门B可同步GateI
            config.setGateBSynchronMode(GateSynchron::GateI);
        } else if (index == 2 && config.getGateAEnable()) {
            // 闸门A使能时，闸门B可同步GateA
            config.setGateBSynchronMode(GateSynchron::GateA);
        } else {
            // 闸门I/A未使能时，回退到Pulser
            config.setGateBSynchronMode(GateSynchron::Pulser);
            if ((index == 1 && !config.getGateIEnable())
                || (index == 2 && !config.getGateAEnable())) {
                // UI修正：同步目标未使能，回退SyncBox显示
                ui->SyncBox->blockSignals(true);
                ui->SyncBox->setCurrentIndex(0);
                ui->SyncBox->blockSignals(false);
            }
        }
        auto newStartB = ui->StartSpinBox->value();
        auto newEndB = ui->WidthSpinBox->value() + newStartB;
        bool flag = config.setGateBEnd(newEndB);
        if (flag == false) {
            config.setGateBStart(newStartB);
            config.setGateBEnd(newEndB);
        }
        config.setGateBStart(newStartB);

    } else if (currentGate == GATE::GATE_C) {
        // index: 0=Pulser, 1=GateI, 2=GateA, 3=GateB
        if (index == 1 && config.getGateIEnable()) {
            config.setGateCSynchronMode(GateSynchron::GateI);
        } else if (index == 2 && config.getGateAEnable()) {
            config.setGateCSynchronMode(GateSynchron::GateA);
        } else if (index == 3 && config.getGateBEnable()) {
            config.setGateCSynchronMode(GateSynchron::GateB);
        } else {
            config.setGateCSynchronMode(GateSynchron::Pulser);
            if ((index == 1 && !config.getGateIEnable()) || (index == 2 && !config.getGateAEnable())
                || (index == 3 && !config.getGateBEnable())) {
                ui->SyncBox->blockSignals(true);
                ui->SyncBox->setCurrentIndex(0);
                ui->SyncBox->blockSignals(false);
            }
        }
        auto newStartC = ui->StartSpinBox->value();
        auto newEndC = ui->WidthSpinBox->value() + newStartC;
        bool flagC = config.setGateCEnd(newEndC);
        if (flagC == false) {
            config.setGateCStart(newStartC);
            config.setGateCEnd(newEndC);
        }
        config.setGateCStart(newStartC);
    }
    // 闸门A、B或C同步GateI时，闸门I自动设为波前模式
    if (config.getGateASynchronMode() == GateSynchron::GateI
        || config.getGateBSynchronMode() == GateSynchron::GateI
        || config.getGateCSynchronMode() == GateSynchron::GateI) {
        config.setGateIMeasureType(measureType::WaveFront);
    } else {
        config.setGateIMeasureType(measureType::MaxPeak);
    }

    if (config.getGateBSynchronMode() == GateSynchron::GateA
        || config.getGateCSynchronMode() == GateSynchron::GateA) {
        config.setGateAMeasureType(measureType::WaveFront);
    } else {
        config.setGateAMeasureType(measureType::MaxPeak);
    }
    if (config.getGateCSynchronMode() == GateSynchron::GateB) {
        config.setGateBMeasureType(measureType::WaveFront);
    } else if (config.getGateCSynchronMode() != GateSynchron::GateA) {
        // 只在没有其他闸门同步到GateB时恢复
        config.setGateBMeasureType(measureType::MaxPeak);
    }
    if (config.getGateCSynchronMode() == GateSynchron::GateA
        || config.getGateCSynchronMode() == GateSynchron::GateB) {
        // 闸门C同步到A或B时，目标闸门设为波前模式
        if (config.getGateCSynchronMode() == GateSynchron::GateA)
            config.setGateAMeasureType(measureType::WaveFront);
    }
    emit App::getInstance()->signal_showhideGatestatus();
    emit App::getInstance()->signal_GateView_Refresh();
}

// 同步采集
void MainWindow1::on_SynAcqisitBox_currentIndexChanged(int index)
{
    if (currentGate == GATE::GATE_I) {
        if (index) { // 开启同步采集
            config.setGateISyncSample(true);
        } else { // 关闭同步采集
            config.setGateISyncSample(false);
        }
    }
    emit App::getInstance()->signal_showhideGatestatus();
    emit App::getInstance()->signal_GateView_Refresh();
}

// TCG使能
void MainWindow1::on_TCG_ON_Btn_clicked()
{
    if (tcg_enable) {
        tcg_enable = false;
        config.setTcgEnable(false);
        ui->TCG_ON_Btn->setChecked(false);
        ui->TCG_ON_Btn->setText(tr("TCG 关闭"));
    } else {
        tcg_enable = true;
        config.setTcgEnable(true);
        ui->TCG_ON_Btn->setChecked(true);
        ui->TCG_ON_Btn->setText(tr("TCG 开启"));
    }
}

// 新增点
void MainWindow1::on_Add_point_Btn_clicked()
{
    int beamIndex = m_currentBeam - 1;

    int nextIndex = 1;
    for (;; nextIndex++) {
        double depth = config.getTcgPointDepth(nextIndex, beamIndex);
        if (depth < 0.0)
            break;
    }

    config.setTcgPointDepth(nextIndex, ui->TCG_pathSpinBox->value(), beamIndex);
    config.setTcgPointGain(nextIndex, ui->TCG_GainSpinBox->value(), beamIndex);

    loadTcgPoints();
    refreshTabwidget();
}

// 当前工作组
void MainWindow1::on_group_comboBox_currentIndexChanged(int index)
{
    if (index < 0) return;
    QString itemData = ui->group_comboBox->itemText(index);
    int val = itemData.toInt();

    config.setCurrentGroup(val);
    refreshgroup();
    emit App::getInstance()->signal_GateView_Refresh();
}

// 添加工作组
void MainWindow1::on_add_Btn_clicked()
{
    if (config.copyGroup()) {
        refreshgroup();
    }
}

// 删除工作组
void MainWindow1::on_suf_Btn_clicked()
{
    if(ui->group_comboBox->currentText()!="0")
    {
        if (config.getGroupsNo().size() <= 1)
            return;
        QString itemData = ui->group_comboBox->currentText();
        int index = itemData.toInt();
        if (config.removeGroup(index)) {
            refreshgroup();
        }
    }
}

// 保存参数
void MainWindow1::on_pushButton_47_clicked()
{
    emit App::getInstance()->refresh_Allpara();
    emit App::getInstance()->signal_GateView_Refresh();
    emit App::getInstance()->signal_Connected();
    emit App::getInstance()->signal_tcgChanged();

    saveParameters();

    QMessageBox::information(this, "提示", "参数保存成功！");
}

// 开始扫查
void MainWindow1::on_pushButton_48_clicked()
{
    ui->View_1->clearSeries();

    config.setServerAddr("192.168.1.100", 0);

    config.startCapture(true);
}

// 停止扫查
void MainWindow1::on_pushButton_49_clicked()
{
    config.startCapture(false);
}

// 龙门位置
void MainWindow1::on_pushButton_50_clicked()
{
    qDebug() << "用户点击获取龙门位置";

    // 执行宏
    QMetaObject::invokeMethod(m_worker, "doExecuteCommand",
                              Qt::QueuedConnection,
                              Q_ARG(QString, "获取龙门"));

    // 延迟 1.5 秒后读取注册表
    QTimer::singleShot(500, [this]() {
        QSettings settings("HKEY_CURRENT_USER\\Software\\SoundScan", QSettings::NativeFormat);

        double x = settings.value("GantryX_mm", "0").toString().toDouble();
        double y = settings.value("GantryY_mm", "0").toString().toDouble();

        qDebug() << "[MainWindow1] 龙门位置: X=" << x << " mm, Y=" << y << " mm";

        ui->doubleSpinBox_4->setValue(x);
        ui->doubleSpinBox_5->setValue(y);
    });
}


void MainWindow1::on_pushButton_51_clicked()
{
    ui->doubleSpinBox_4->clear();
    ui->doubleSpinBox_5->clear();
}

void MainWindow1::on_comboBox_currentTextChanged(const QString &arg1)
{
    qDebug() << "切换到" << ui->comboBox->currentText() << "模式";
}


void MainWindow1::on_comboBox_2_currentTextChanged(const QString &arg1)
{
    qDebug() << "切换到" << ui->comboBox_2->currentText();
}

void MainWindow1::on_doubleSpinBox_8_valueChanged(double arg1)
{
    Thick = ui->doubleSpinBox_8->value();
}

