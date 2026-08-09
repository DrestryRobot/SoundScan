#ifndef DELMIAWORKER_H
#define DELMIAWORKER_H

#include <QObject>
#include <QString>
#include <QAxObject>
#include <QSettings>
#include <QThread>
#include <QCoreApplication>

class DelmiaWorker : public QObject
{
    Q_OBJECT
public:
    explicit DelmiaWorker(QObject *parent = nullptr);

public slots:
    void doGetCurrentDocumentInfo();
    void doNewFile(const QString &type, const QString &fileName);
    void doOpenFile(const QString &path);
    void doSaveFile(const QString &path);
    void doCloseFile(int docIndex = -1);
    void doExecuteCommand(const QString &cmd);
    void doExitSimulation();
    void doOpenSimulation();
    void doCloseSimulation();
    void doGetDocumentList();
    void doSaveCurrentFile();
    void doGetActiveDocumentName();
    void doCloseCurrentFile();
    void doActivateDocument(int docIndex);
    void doSaveFileWithPath(const QString &path);
    void doAssemblePartToProduct(const QString &productPath, const QString &partPath);
    void doAssembleProductToParent(const QString &parentProductPath, const QString &childProductPath);
    void doActivateDocumentByName(const QString &docName);
    void doCloseDocumentByName(const QString &docName);
    void doAddProductToProcess(const QString &processPath, const QString &productPath);
    void doAddFourPointsToPart(const QString &partDocName,
                               double x1, double y1, double z1,
                               double x2, double y2, double z2,
                               double x3, double y3, double z3,
                               double x4, double y4, double z4);
    void doGetGantryPosition(const QString &gantryName);
    void doCreateProject(const QString &sourcePartPath, const QString &projectPath, const QString &projectName);

signals:
    void newFileResult(const QString &docName);
    void openFileResult(const QString &path);
    void saveFileResult(const QString &path);
    void closeFileResult(const QString &path);
    void exitResult();
    void openResult();
    void closeResult();
    void documentListReady(const QList<QStringList> &docInfoList);
    void currentDocumentInfo(const QString &displayText);
    void requestSaveFileDialog(const QString &defaultPath, const QString &filter);
    void activeDocumentNameResult(const QString &docName);
    void closeFileInfo(const QString &closedText, const QString &currentText);
    void partAssembled(const QString &partName, const QString &productName);
    void productAssembled(const QString &childName, const QString &parentName);
    void productAddedToProcess(const QString &productName, const QString &processName);
    void gantryPositionResult(const QString &gantryName, double x, double y);
    void commandExecuted(const QString &cmd);
    void projectFilesCreated(const QString &projectName, const QString &partZBDocName);
    void projectCreated(bool success, const QString &message);
};

#endif