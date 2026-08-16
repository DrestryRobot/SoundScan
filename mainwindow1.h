#ifndef MAINWINDOW1_H
#define MAINWINDOW1_H

#include <QMainWindow>
#include <QAxObject>
#include <QFileDialog>
#include <QMessageBox>
#include <QStyleFactory>
#include <QWidget>
#include <QCoreApplication>
#include <QRegularExpression>
// 添加必要的头文件
#include <QListWidget>
#include <QDialog>
#include <QInputDialog>
#include <QProgressDialog>
#include <QFuture>
#include <QAtomicInt>

// 超声扫描
#include "client.h"
#include "Phaselink/dialog/viewwidget.h"
#include "Phaselink/dialog/dataprocessor.h"
#include "Phaselink/dialog/PacketDataSaver.h"
#include "Phaselink/dialog/measurewidget.h"
#include "Phaselink/dialog/viewmodel.h"
#include "Phaselink/dialog/sider.h"
#include "Phaselink/UI/UT/essentialwidget.h"
#include "Phaselink/UI/UT/acg_tcg_widget.h"
#include "Phaselink/dialog/addsud_group.h"

// 运动控制
#include "udpserver.h"
#include "ads_client.h"
#include "tcpserver.h"

// 用户自定义
#include "colormanager.h"
#include "debugoutput.h"
#include "delmiaworker.h"
#include "ads_poller.h"

class AdsStatusPoller;

enum GATE { GATE_A, GATE_B, GATE_C, GATE_I };
enum GATE_Sync { Sync_false, Sync_gate_I, Sync_gate_A, Sync_gate_B };
enum class ConfigState { Unknown, Connected, Unconnected, Failed };

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow1;
}
QT_END_NAMESPACE

class MainWindow1 : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow1(QWidget *parent = nullptr);

    ~MainWindow1();

    void on_pushButton_9();                  // 扫描开始（外部调用）

    void on_pushButton_19();                 // 扫描暂停（外部调用）

    void on_pushButton_20();                 // 扫描结束（外部调用）

private:
    void initWidget();                       // 初始化超声界面

    void initSlot();                         // 初始化超声信号

    void getcurrentPara();                   // 获取当前参数

    void setBtnchecked(bool check);

    void initDelmiaStatus();                 // 初始化仿真状态

    void initDelmiaSlot();                   // 初始化仿真信号

    void initThemeSwitch();                  // 初始化色彩模式

    void onRobotStatusReady(const AdsStatusPoller::Status &status); // 后台PLC轮询结果刷新UI
    void update();   // 旧版 UI 刷新（阻塞 ADS 读，已被 AdsStatusPoller 取代，待删除）

    void saveParameters();                   // 保存系统参数

    void loadParameters();                   // 加载系统参数

    void savePoseData(bool showPopup);       // 保存点位参数

    void loadPoseData();                     // 加载点位参数

    void applyLightTheme();                  // 应用深色主题

    void applyDarkTheme();                   // 应用浅色主题

    void setupIndicatorFromCheckBox(QCheckBox *checkBox, const QColor &onColor = QColor(0, 255, 0)); // 状态指示灯

    void onDeleteRow();                      // 删除当前行

    void onCellChanged(int row, int column); // 单元格响应

    void refreshTabwidget();                 // 刷新标签页

    int findNextAvailableIndex(const std::vector<int> &numbers);        // 查找可用索引

    void loadTcgPoints();                    // 加载TCG点

    int beamFromRow(int row) const { return row >= 0 && row < m_tcgPoints.size() ? m_tcgPoints[row].second : -1; }

    int indexFromRow(int row) const { return row >= 0 && row < m_tcgPoints.size() ? m_tcgPoints[row].first : -1; }

    std::vector<std::pair<int, int>> m_tcgPoints;                       // (index, beamIndex)

    void recordPosePoint(int pointIndex);    // 记录位姿点

    void refreshgroup();                     // 刷新工作组

    void createSwitchWindowDialog(const QList<QStringList> &docInfoList,
                                  QPointer<QDialog> &dialog,
                                  QPointer<QListWidget> &listWidget);   // 提取创建对话框的函数

    bool copyDirectory(const QString &srcPath, const QString &dstPath); // 递归复制目录

    bool calculateAndUpdateZBFourPoints(const QString &projectName);    // 计算并更新 ZB Part 中的四个点

    void startDebug();

    void init3DCheckerSlot();

    void start3DChecker();

    void onRequestStartDrawing();

    void onRequestStopDrawing();

    void updateRobotData(double x, double y, double z, double a, double b, double c);

signals:
    void requestStartDrawing();

    void requestStopDrawing();

protected:

    void closeEvent(QCloseEvent *event) override;

private slots:

    void on_pushButton_43_clicked(); // 新建文件

    void on_pushButton_clicked();    // 打开文件

    void on_pushButton_3_clicked();  // 保存文件

    void on_pushButton_8_clicked();  // 退出仿真

    void on_pushButton_4_clicked();  // 切换窗口

    void on_pushButton_44_clicked(); // 关闭文件

    void on_pushButton_2_clicked();  // 选择横向

    void on_pushButton_5_clicked();  // 选择纵向

    void on_pushButton_40_clicked(); // 分区扫描

    void on_pushButton_6_clicked();  // 创建分区

    void on_pushButton_10_clicked(); // 选择边界

    void on_pushButton_11_clicked(); // 选择路径

    void on_pushButton_46_clicked(); // 路径规划

    void on_pushButton_12_clicked(); // 创建路径

    void on_pushButton_17_clicked(); // 标定点位

    void on_pushButton_9_clicked();  // 扫描开始

    void on_pushButton_19_clicked(); // 扫描暂停

    void on_pushButton_20_clicked(); // 扫描结束

    void on_pushButton_22_clicked(); // 龙门开始

    void on_pushButton_18_clicked(); // 龙门暂停

    void on_pushButton_21_clicked(); // 龙门回零

    void on_pushButton_27_clicked(); // 电机使能

    void on_pushButton_28_clicked(); // 电机失能

    void on_pushButton_23_pressed();  // X++按下

    void on_pushButton_23_released(); // X++松开

    void on_pushButton_24_pressed();  // X--按下

    void on_pushButton_24_released(); // X--松开

    void on_pushButton_25_pressed();  // Y++按下

    void on_pushButton_25_released(); // Y++松开

    void on_pushButton_26_pressed();  // Y--按下

    void on_pushButton_26_released(); // Y--松开

    void on_pushButton_13_clicked(); // 位姿点一

    void on_pushButton_14_clicked(); // 位姿点二

    void on_pushButton_15_clicked(); // 位姿点三

    void on_pushButton_16_clicked(); // 位姿点四

    void on_pushButton_32_clicked(); // 保存点位

    void on_pushButton_35_clicked(); // 清空点位

    void on_pushButton_37_clicked(); // 加载点位

    void on_pushButton_41_clicked(); // 打开日志

    void on_pushButton_29_clicked(); // 打开仿真（快捷指令）

    void on_pushButton_30_clicked(); // 关闭仿真（快捷指令）

    void on_pushButton_31_clicked(); // 后置处理（快捷指令）

    void on_pushButton_34_clicked(); // 退出系统（快捷指令）

    void on_PRFSpinBox_valueChanged(double arg1);         // 采集帧率

    void on_MamplitudeBox_currentIndexChanged(int index); // 最大幅值

    void on_GainSpinBox_valueChanged(double arg1);        // 增益

    void on_RangstartSpinBox_valueChanged(double arg1);   // 范围起点

    void on_RangendSpinBox_valueChanged(double arg1);     // 范围终点

    void on_FilterLowBox_currentIndexChanged(int index);  // 低通滤波器

    void on_FilterHighBox_currentIndexChanged(int index); // 高通滤波器

    void on_vedioFilterBox_currentIndexChanged(int index); // 图像滤波

    void on_RectifierBox_currentIndexChanged(int index);  // 检波模式

    void on_Btn_right_clicked();                          // 闸门使能关闭

    void on_Btn_left_clicked();                           // 闸门使能开启

    void on_MeasureBox_currentIndexChanged(int index);    // 闸门测量方法

    void on_StartSpinBox_valueChanged(double arg1);       // 闸门开始

    void on_WidthSpinBox_valueChanged(double arg1);       // 闸门宽度

    void on_ThresholdSpinBox_valueChanged(double arg1);   // 闸门阈值

    void on_PaVoltageSpinBox_valueChanged(double arg1);   // 电压

    void on_ProbeFrequencySpinBox_valueChanged(double arg1); // 探头频率

    void on_PwidthSpinBox_valueChanged(double arg1);      // 脉冲宽度

    void on_SyncBox_currentIndexChanged(int index);       // 同步

    void on_SynAcqisitBox_currentIndexChanged(int index); // 同步采集

    void on_TCG_ON_Btn_clicked();                         // TCG使能 开启

    void on_Add_point_Btn_clicked();                      // 新增点

    void on_group_comboBox_currentIndexChanged(int index); // 当前工作组

    void on_add_Btn_clicked();                            // 添加工作组

    void on_suf_Btn_clicked();                            // 删除工作组

    void on_pushButton_47_clicked();                      // 保存参数

    void on_pushButton_48_clicked();                      // 开始扫查

    void on_pushButton_49_clicked();                      // 停止扫查

    void on_pushButton_50_clicked();                      // 龙门位置

    void on_pushButton_51_clicked();                      // 清除位置

    void on_comboBox_currentTextChanged(const QString &arg1); // 显示模式切换

    void on_comboBox_2_currentTextChanged(const QString &arg1); // 显示任务切换

    void on_doubleSpinBox_8_valueChanged(double arg1);    // 设置TOF深度

private:

    Ui::MainWindow1 *ui;
    
    // 运动控制
    UdpServer* server;
    TcpServer* kuka;
    QThread *m_udpThread = nullptr;
    QThread *m_adsThread = nullptr;
    QThread *m_tcpThread = nullptr;
    QThread *m_pollThread = nullptr;
    AdsStatusPoller *m_poller = nullptr;
    bool x_daowei = true;
    bool y_daowei = true;
    bool x_flag = false;
    bool y_flag = false;
    bool scan_continue_flag = true;
    // 扫描开始后等待“机器人开始扫板”信号，信号到达才触发 3dscan 开始绘制
    bool m_scanStartPending = false;

    // 电机失能检测
    double m_lastCheckX = 0.0;
    double m_lastCheckY = 0.0;
    int m_noMoveCount = 0;

    // 超声扫描
    Client &config;
    ConfigState IsConnect = ConfigState::Unconnected;
    DataProcessor *m_dataProcessor = nullptr;

    QThread *m_processorThread = nullptr;
    QMap<QDoubleSpinBox *, bool> isBlockSignal;
    GATE currentGate = GATE::GATE_I;
    QPushButton *deleteButton;
    bool tcg_enable = false;
    int m_currentBeam = 1;

    // 3D检查相关
    QFuture<void> m_future;
    QAtomicInt m_stopFlag;

    // 线程安全的数据访问
    QMutex m_adsMutex;

    // 3D库指针（线程安全访问）
    QMutex m_3dMutex;

    // 状态缓存
    int m_current3DStatus;
    int m_last3DStatus;

    QThread* m_3dThread;
    QTimer* m_3dTimer;
    bool m_isRunning;

    // 路径仿真
    QThread m_workerThread;
    DelmiaWorker *m_worker;

    QMap<QString, qint64> m_docOpenTime;


    // 文件路径
    QString posePath = "C:/超声扫描/点位/点位数据.txt";
    QDate today = QDate::currentDate();
    QString dateStr = today.toString("yyyy-MM-dd");
    QString logPath = "C:/超声扫描/日志/系统日志" + dateStr + ".txt";

    const QString rsiStartCode = R"(
DECL INT RET
;FOLD INI;%{PE}
;FOLD BASISTECH INI
GLOBAL INTERRUPT DECL 3 WHEN $STOPMESS==TRUE DO IR_STOPM ( )
INTERRUPT ON 3
BAS (#INITMOV,0 )
;ENDFOLD (BASISTECH INI)
;FOLD USER INI
;Make your modifications here

;ENDFOLD (USER INI)
;ENDFOLD (INI)

$OUT[100]=TRUE

$BWDSTART=FALSE
PDAT_ACT = PDEFAULT
FDAT_ACT = FHOME
BAS(#PTP_PARAMS, 100)
$H_POS = XHOME
PTP XHOME

; Create RSI Context
CONTID = 1
RET = RSI_CREATE("RSI_Ethernet",CONTID,TRUE)
IF (RET <> RSIOK) THEN
   HALT
ENDIF

; Start RSI execution
RET = RSI_ON(#RELATIVE)
IF (RET <> RSIOK) THEN
   HALT
ENDIF

)";

    const QString rsiEndCode = R"(
; Turn off RSI
RET = RSI_OFF()
IF (RET <> RSIOK) THEN
   HALT
ENDIF

$OUT[100]=TRUE

)";

};

#endif // MAINWINDOW1_H
