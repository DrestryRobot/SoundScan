#include "debugoutput.h"
#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <QtConcurrent/QtConcurrent>
#include <QPointer>

// 静态成员定义（只在这里定义一次）
DebugOutput* DebugOutput::instance = nullptr;
QString DebugOutput::currentLogPath;
QDate DebugOutput::currentDate;
QMutex DebugOutput::fileMutex;

DebugOutput::DebugOutput(QLineEdit* target, QObject* parent)
    : QObject(parent)
{
    addOutputWidget(target);
    updateLogPath();
}

void DebugOutput::install()
{
    instance = this;
    qInstallMessageHandler(messageHandler);
}

void DebugOutput::addOutputWidget(QLineEdit* widget)
{
    if (widget && !m_outputEdits.contains(widget)) {
        m_outputEdits.append(widget);
    }
}

void DebugOutput::removeOutputWidget(QLineEdit* widget)
{
    m_outputEdits.removeAll(widget);
}

void DebugOutput::clearOutputWidgets()
{
    m_outputEdits.clear();
}

void DebugOutput::log(const QString& msg)
{
    QString timestamped = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + " >> " + msg;

    for (QLineEdit* edit : m_outputEdits) {
        if (edit) {
            edit->setText(timestamped);
            edit->home(false);
        }
    }

    appendToFile(timestamped);
}

void DebugOutput::updateLogPath()
{
    QDate today = QDate::currentDate();

    if (currentDate != today || currentLogPath.isEmpty()) {
        currentDate = today;

        QString baseDir = "C:/超声扫描/日志";
        QString dateStr = today.toString("yyyy-MM-dd");
        currentLogPath = baseDir + "/系统日志" + dateStr + ".txt";

        QDir dir(baseDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
    }
}

void DebugOutput::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{



    // 定义需要屏蔽的错误关键词列表
    const QStringList blockedKeywords = {
        // 系统错误
        "UpdateLayeredWindowIndirect failed",
        "-2147467259",

        // ActiveDocument 相关错误
        "ActiveDocument",
        "CATIAApplication",
        "QAxBase",

        // Buffer overflow 相关
        "Buffer overflow detected",
        "invalid point count",

        // 调试/日志输出
        "[Batch Render]",
        "s_x", "s_y",
        "Law info updated",
        "Frequency Stats",
        "LibKuka3D",

        "TimestampInterpolator",

        "CSCAN_3D",

        "Defect cloud",

        "OpenGLSurfaceViewer"
    };

    // 统一过滤
    for (const QString& keyword : blockedKeywords) {
        if (msg.contains(keyword)) {
            return;
        }
    }
    // // 屏蔽 UpdateLayeredWindowIndirect 错误
    // if (msg.contains("UpdateLayeredWindowIndirect failed")) {
    //     return;
    // }

    // // 屏蔽 CATIA/DELMIA ActiveDocument 错误
    // if (msg.contains("ActiveDocument") && msg.contains("CATIAApplication")) {
    //     return;
    // }

    // // 屏蔽所有 QAxBase 相关的 ActiveDocument 错误
    // if (msg.contains("QAxBase: Error calling IDispatch member ActiveDocument")) {
    //     return;
    // }

    // // 更通用的屏蔽：包含异常代码 -2147467259 的错误
    // if (msg.contains("-2147467259")) {
    //     return;
    // }

    // // ========== 新增：屏蔽 Buffer overflow 相关错误 ==========
    // // 屏蔽 "Buffer overflow detected or invalid point count!" 错误
    // if (msg.contains("Buffer overflow detected") || msg.contains("invalid point count")) {
    //     return;
    // }

    // if (msg.contains("[Batch Render]")) {
    //     return;
    // }

    // // 屏蔽 s_x, s_y 输出
    // if (msg.contains("s_x") || msg.contains("s_y")) {
    //     return;
    // }

    // // 屏蔽 Law info updated 输出
    // if (msg.contains("Law info updated")) {
    //     return;
    // }

    // // ========== 新增：屏蔽 CSCAN_3D 的 buffering 消息 ==========
    // if (msg.contains("CSCAN_3D") && msg.contains("Buffering scan data, waiting for robot positions")) {
    //     return;
    // }

    // // 屏蔽 Law info updated 输出
    // if (msg.contains("Frequency Stats")) {
    //     return;
    // }


    // // 屏蔽 Law info updated 输出
    // if (msg.contains("LibKuka3D")) {
    //     return;
    // }

    // // 屏蔽 Law info updated 输出
    // if (msg.contains("CSCAN_3D")) {
    //     return;
    // }


    Q_UNUSED(type)
    Q_UNUSED(context)

    updateLogPath();

    QString timestamped = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + " >> " + msg;

    // messageHandler 可能被任意工作线程（如 Scan 线程）调用，
    // QLineEdit 只能在主线程操作，这里把控件更新投递到主线程执行，
    // 避免跨线程访问 Qt 控件导致死锁/卡死。
    if (instance) {
        QPointer<DebugOutput> guard(instance);
        QMetaObject::invokeMethod(instance, [guard, timestamped]() {
            if (!guard)
                return;
            for (QLineEdit* edit : guard->m_outputEdits) {
                if (edit) {
                    edit->setText(timestamped);
                    edit->home(false);
                }
            }
        }, Qt::QueuedConnection);
    }

    QtConcurrent::run([timestamped]() {
        appendToFile(timestamped);
    });
}

void DebugOutput::appendToFile(const QString& text)
{
    QMutexLocker locker(&fileMutex);

    QFile file(currentLogPath);

    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        QThread::msleep(5);
        if (!file.open(QIODevice::Append | QIODevice::Text)) {
            return;
        }
    }

    QTextStream out(&file);
    out << text << "\n";

    out.flush();
    file.close();
}

DebugOutput* DebugOutput::getInstance()
{
    return instance;
}
