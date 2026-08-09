// DebugOutput.h
#ifndef DEBUGOUTPUT_H
#define DEBUGOUTPUT_H

#include <QObject>
#include <QLineEdit>
#include <QList>
#include <QString>
#include <QDate>
#include <QMutex>

class DebugOutput : public QObject
{
    Q_OBJECT

public:
    explicit DebugOutput(QLineEdit* target, QObject* parent = nullptr);

    void install();
    void addOutputWidget(QLineEdit* widget);
    void removeOutputWidget(QLineEdit* widget);
    void clearOutputWidgets();
    void log(const QString& msg);

    static DebugOutput* getInstance();

private:
    QList<QLineEdit*> m_outputEdits;

    static DebugOutput* instance;
    static QString currentLogPath;
    static QDate currentDate;
    static QMutex fileMutex;

    static void updateLogPath();
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    static void appendToFile(const QString& text);
};

#endif // DEBUGOUTPUT_H

