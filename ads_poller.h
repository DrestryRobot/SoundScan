#ifndef ADS_POLLER_H
#define ADS_POLLER_H

#include <QObject>
#include <QTimer>

// 后台线程轮询 PLC 状态，避免在 UI 线程做阻塞式 ADS 同步读。
// 工作线程每次 poll() 只做 ADS 读取并 emit 结果，UI 线程只负责刷新控件。
class AdsStatusPoller : public QObject
{
    Q_OBJECT

public:
    struct Status
    {
        double robotX = 0, robotY = 0, robotZ = 0;
        double robotA = 0, robotB = 0, robotC = 0;

        bool xEnable = false;
        bool yEnable = false;

        double xPos = 0, xVel = 0;
        double yPos = 0, yVel = 0;

        bool xMove1 = false, xMove2 = false, xMove3 = false, xMove4 = false;
        bool yMove1 = false, yMove2 = false, yMove3 = false, yMove4 = false;

        int progNo = 0;

        bool modeT1 = false, modeT2 = false, modeAUT = false, modeEXT = false;

        double longmenX = 0, longmenY = 0;
    };

    explicit AdsStatusPoller(QObject *parent = nullptr);

public slots:
    void start();
    void stop();

private slots:
    void poll();

signals:
    void statusReady(const AdsStatusPoller::Status &status);

private:
    // 必须作为子对象创建，moveToThread 才会把它一起移到轮询线程
    QTimer m_timer{this};
};

Q_DECLARE_METATYPE(AdsStatusPoller::Status)

#endif // ADS_POLLER_H
