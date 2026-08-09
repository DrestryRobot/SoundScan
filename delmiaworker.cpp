#include "DelmiaWorker.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <windows.h>
#include <QSettings>

DelmiaWorker::DelmiaWorker(QObject *parent) : QObject(parent) {}

void DelmiaWorker::doGetCurrentDocumentInfo()
{
    qDebug() << "[Delmia] 获取当前文档信息";

    CoInitialize(NULL);
    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        emit currentDocumentInfo("");
        return;
    }

    QString resultText;
    QAxObject *activeDoc = catia->querySubObject("ActiveDocument");
    if (activeDoc && !activeDoc->isNull()) {
        QString path = activeDoc->property("FullName").toString();
        QString name = activeDoc->property("Name").toString();

        if (!path.isEmpty() && QFile::exists(path)) {
            resultText = QDir::toNativeSeparators(path);
        } else {
            resultText = name + " (未保存)";
        }
        delete activeDoc;
    }

    delete catia;
    CoUninitialize();

    emit currentDocumentInfo(resultText);
}

void DelmiaWorker::doNewFile(const QString &type, const QString &fileName)
{
    CoInitialize(NULL);

    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        return;
    }

    QAxObject *documents = catia->querySubObject("Documents");
    if (documents && !documents->isNull()) {
        QAxObject *newDoc = documents->querySubObject("Add(const QString&)", type);
        if (newDoc && !newDoc->isNull()) {
            if (!fileName.isEmpty()) {
                QAxObject *product = newDoc->querySubObject("Product");
                if (product && !product->isNull()) {
                    // 直接使用传入的文件名，不添加前缀
                    product->setProperty("PartNumber", fileName);
                    product->setProperty("Name", fileName);
                    delete product;
                }
            }
            QString docName = newDoc->property("Name").toString();
            emit newFileResult(docName);
            delete newDoc;
        }
        delete documents;
    }

    delete catia;
    CoUninitialize();
}

void DelmiaWorker::doOpenFile(const QString &path)
{
    qDebug() << "[Delmia] 打开文件:" << path;

    CoInitialize(NULL);

    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        CoUninitialize();
        return;
    }

    QAxObject *documents = catia->querySubObject("Documents");
    if (documents && !documents->isNull()) {
        QString nativePath = QDir::toNativeSeparators(path);
        QAxObject *newDoc = documents->querySubObject("Open(const QString&)", nativePath);
        if (newDoc && !newDoc->isNull()) {
            emit openFileResult(path);
            delete newDoc;
        }
        delete documents;
    }

    delete catia;
    CoUninitialize();
}

void DelmiaWorker::doSaveFile(const QString &path)
{
    qDebug() << "[Delmia] 保存文件:" << (path.isEmpty() ? "直接保存" : path);

    CoInitialize(NULL);
    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        emit saveFileResult("");
        return;
    }

    QAxObject *activeDoc = catia->querySubObject("ActiveDocument");
    if (activeDoc && !activeDoc->isNull()) {
        if (path.isEmpty()) {
            // 直接保存
            activeDoc->dynamicCall("Save()");
            QString savedPath = activeDoc->property("FullName").toString();
            emit saveFileResult(savedPath);
        } else {
            // 另存为
            QString nativePath = QDir::toNativeSeparators(path);
            activeDoc->dynamicCall("SaveAs(const QString&)", nativePath);
            QString savedPath = activeDoc->property("FullName").toString();
            emit saveFileResult(savedPath);
        }
        delete activeDoc;
    } else {
        emit saveFileResult("");
    }

    delete catia;
    CoUninitialize();
}

void DelmiaWorker::doCloseFile(int docIndex)
{
    qDebug() << "[Delmia] 关闭文件，索引:" << docIndex;

    CoInitialize(NULL);
    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        emit closeFileResult("");
        return;
    }

    QString closedName;
    bool wasSaved = false;

    // 获取要关闭的文档信息（在关闭前）
    if (docIndex != -1) {
        QAxObject *documents = catia->querySubObject("Documents");
        if (documents && !documents->isNull()) {
            QAxObject *targetDoc = documents->querySubObject("Item(int)", docIndex);
            if (targetDoc && !targetDoc->isNull()) {
                closedName = targetDoc->property("Name").toString();
                QString fullPath = targetDoc->property("FullName").toString();
                wasSaved = !fullPath.isEmpty() && QFile::exists(fullPath);
                targetDoc->dynamicCall("Activate()");
                delete targetDoc;
            }
            delete documents;
        }
    }

    // 关闭当前活动文档
    QAxObject *activeDoc = catia->querySubObject("ActiveDocument");
    if (activeDoc && !activeDoc->isNull()) {
        if (closedName.isEmpty()) {
            closedName = activeDoc->property("Name").toString();
            QString fullPath = activeDoc->property("FullName").toString();
            wasSaved = !fullPath.isEmpty() && QFile::exists(fullPath);
        }
        activeDoc->dynamicCall("Close()");
        delete activeDoc;
    }

    // 获取关闭后的当前活动文档
    QString resultPath;
    QAxObject *newActiveDoc = catia->querySubObject("ActiveDocument");
    if (newActiveDoc && !newActiveDoc->isNull()) {
        QString newPath = newActiveDoc->property("FullName").toString();
        QString newName = newActiveDoc->property("Name").toString();
        bool isSaved = !newPath.isEmpty() && QFile::exists(newPath);

        if (isSaved) {
            resultPath = QDir::toNativeSeparators(newPath);
        } else {
            resultPath = newName + " (未保存)";
        }
        delete newActiveDoc;
    }

    delete catia;
    CoUninitialize();

    // 发送关闭的文档信息和新活动文档路径
    QString closedDisplay = wasSaved ? QDir::toNativeSeparators(closedName) : closedName + " (未保存)";
    emit closeFileInfo(closedDisplay, resultPath);
    emit closeFileResult(resultPath);
}

void DelmiaWorker::doExitSimulation()
{
    qDebug() << "[Delmia] 退出仿真";

    CoInitialize(NULL);

    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (catia && !catia->isNull()) {
        QAxObject* documents = catia->querySubObject("Documents");
        if (documents) {
            while (true) {
                int docCount = documents->property("Count").toInt();
                if (docCount == 0) break;
                QAxObject* activeDoc = catia->querySubObject("ActiveDocument");
                if (!activeDoc) break;
                activeDoc->dynamicCall("Close()");
                delete activeDoc;
            }
            delete documents;
        }
        catia->dynamicCall("Quit()");
        delete catia;
    }

    emit exitResult();
    CoUninitialize();
}

void DelmiaWorker::doExecuteCommand(const QString &cmd)
{
    qDebug() << "[Delmia] 执行命令:" << cmd;

    CoInitialize(NULL);

    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (catia && !catia->isNull()) {
        catia->dynamicCall("StartCommand(QString)", cmd);
        delete catia;
    }

    CoUninitialize();

    // 发送命令执行完成信号
    emit commandExecuted(cmd);
}

void DelmiaWorker::doOpenSimulation()
{
    qDebug() << "[Delmia] 打开仿真";

    CoInitialize(NULL);

    HWND hTarget = NULL;

    // 查找 DELMIA V5 窗口
    EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL {
        wchar_t title[256];
        GetWindowTextW(hWnd, title, sizeof(title)/sizeof(wchar_t));
        if (wcsstr(title, L"DELMIA V5")) {
            *(HWND*)lParam = hWnd;
            return FALSE;
        }
        return TRUE;
    }, (LPARAM)&hTarget);

    QAxObject *catia = nullptr;

    if (hTarget) {
        // DELMIA 窗口已存在
        catia = new QAxObject("DELMIA.Application", nullptr);

        if (catia && !catia->isNull()) {
            bool isVisible = catia->property("Visible").toBool();

            if (!isVisible) {
                catia->setProperty("Visible", true);
            }
        }

        // 将窗口提到前台
        SetForegroundWindow(hTarget);
    } else {
        // DELMIA 未运行，创建新实例
        catia = new QAxObject("DELMIA.Application", nullptr);

        if (catia && !catia->isNull()) {
            catia->setProperty("Visible", true);
        }
    }

    // 清理 COM 对象
    if (catia) {
        delete catia;
    }

    emit openResult();
    CoUninitialize();
}

void DelmiaWorker::doCloseSimulation()
{
    qDebug() << "[Delmia] 关闭仿真";

    CoInitialize(NULL);

    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (catia && !catia->isNull()) {
        catia->setProperty("Visible", false);
        delete catia;
    }

    emit closeResult();
    CoUninitialize();
}

void DelmiaWorker::doGetDocumentList()
{
    qDebug() << "[Delmia] 获取文档列表";

    CoInitialize(NULL);

    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        return;
    }

    QList<QStringList> docInfoList;

    QAxObject *documents = catia->querySubObject("Documents");
    QAxObject *windows = catia->querySubObject("Windows");

    if (documents && !documents->isNull() && windows && !windows->isNull()) {
        int docCount = documents->property("Count").toInt();
        int winCount = windows->property("Count").toInt();

        // 收集所有窗口标题
        QStringList windowCaptions;
        for (int j = 1; j <= winCount; ++j) {
            QAxObject *win = windows->querySubObject("Item(int)", j);
            if (win) {
                QString winCaption = win->property("Caption").toString();
                windowCaptions.append(winCaption);
                delete win;
            }
        }

        QString activeDocName;
        QAxObject *activeDoc = catia->querySubObject("ActiveDocument");
        QString activeFullName;
        if (activeDoc && !activeDoc->isNull()) {
            activeDocName = activeDoc->property("Name").toString();
            activeFullName = activeDoc->property("FullName").toString();
            delete activeDoc;
        }

        for (int i = 1; i <= docCount; ++i) {
            QAxObject *doc = documents->querySubObject("Item(int)", i);
            if (!doc) continue;

            QString docName = doc->property("Name").toString();
            QString fullName = doc->property("FullName").toString();

            // 如果是当前活动文档，使用之前获取的 fullName
            if (docName == activeDocName && !activeFullName.isEmpty()) {
                fullName = activeFullName;
            }

            bool isSaved = !fullName.isEmpty() && QFile::exists(fullName);

            bool hasWindow = false;
            for (const QString &caption : windowCaptions) {
                if (caption.contains(docName, Qt::CaseInsensitive)) {
                    hasWindow = true;
                    break;
                }
            }

            if (hasWindow || !isSaved) {
                QStringList info;
                info << docName;
                info << fullName;
                info << QString::number(i);
                info << (isSaved ? "1" : "0");
                info << (docName == activeDocName ? "1" : "0");
                docInfoList.append(info);
            }
            delete doc;
        }

        delete documents;
        delete windows;
    }

    delete catia;
    CoUninitialize();

    emit documentListReady(docInfoList);
}

void DelmiaWorker::doSaveCurrentFile()
{
    qDebug() << "[Delmia] 保存当前文件";

    CoInitialize(NULL);
    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        emit saveFileResult("");
        return;
    }

    QAxObject *activeDocument = catia->querySubObject("ActiveDocument");
    if (!activeDocument || activeDocument->isNull()) {
        delete catia;
        CoUninitialize();
        emit saveFileResult("");
        return;
    }

    // 获取文档路径
    QString filePath = activeDocument->property("FullName").toString();

    // 如果文档已保存过，直接保存
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        activeDocument->dynamicCall("Save()");
        QString savedPath = activeDocument->property("FullName").toString();
        emit saveFileResult(savedPath);
        delete activeDocument;
        delete catia;
        CoUninitialize();
        return;
    }

    // 获取文档的名称
    QString docName = activeDocument->property("Name").toString();
    delete activeDocument;
    delete catia;
    CoUninitialize();

    // 根据文件名后缀判断类型
    QString suffix = QFileInfo(docName).suffix().toLower();
    QString filter;
    QString extension;

    if (suffix == "catpart") {
        filter = "CATPart (*.CATPart)";
        extension = ".CATPart";
    }
    else if (suffix == "catproduct") {
        filter = "CATProduct (*.CATProduct)";
        extension = ".CATProduct";
    }
    else if (suffix == "catprocess") {
        filter = "CATProcess (*.CATProcess)";
        extension = ".CATProcess";
    }
    else {
        filter = "所有文件 (*.*)";
        extension = "";
    }

    // 去掉可能存在的扩展名
    QString baseName = docName;
    int lastDot = baseName.lastIndexOf('.');
    if (lastDot > 0) {
        baseName = baseName.left(lastDot);
    }
    QString defaultName = baseName + extension;

    // 读取上次保存路径
    QSettings settings("HeWenTech", "CATIAAutomationTool");
    QString lastSavePath = settings.value("LastSavePath", "").toString();

    // 构建默认保存路径
    QString defaultPath;
    if (!lastSavePath.isEmpty() && QDir(lastSavePath).exists()) {
        defaultPath = QDir(lastSavePath).filePath(defaultName);
    } else {
        defaultPath = defaultName;
    }

    // 发送信号请求主线程显示文件对话框
    emit requestSaveFileDialog(defaultPath, filter);
}

void DelmiaWorker::doGetActiveDocumentName()
{
    CoInitialize(NULL);
    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    QString docName;

    if (catia && !catia->isNull()) {
        QAxObject *activeDoc = catia->querySubObject("ActiveDocument");
        if (activeDoc && !activeDoc->isNull()) {
            docName = activeDoc->property("Name").toString();
            delete activeDoc;
        }
        delete catia;
    }

    CoUninitialize();
    emit activeDocumentNameResult(docName);
}

void DelmiaWorker::doCloseCurrentFile()
{
    qDebug() << "[Delmia] 关闭当前文件";

    CoInitialize(NULL);
    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        return;
    }

    // 获取要关闭的文档信息
    QAxObject *activeDocument = catia->querySubObject("ActiveDocument");
    if (!activeDocument || activeDocument->isNull()) {
        delete catia;
        CoUninitialize();
        return;
    }

    QString closedPath = activeDocument->property("FullName").toString();
    QString closedName = activeDocument->property("Name").toString();
    bool isClosedSaved = !closedPath.isEmpty() && QFile::exists(closedPath);

    QString displayText;
    if (isClosedSaved) {
        displayText = QDir::toNativeSeparators(closedPath);
    } else {
        displayText = closedName + " (未保存)";
    }

    // 关闭文档
    activeDocument->dynamicCall("Close()");
    delete activeDocument;

    // 获取关闭后的当前活动文档
    QString resultPath;
    QAxObject *newActiveDoc = catia->querySubObject("ActiveDocument");
    if (newActiveDoc && !newActiveDoc->isNull()) {
        QString newPath = newActiveDoc->property("FullName").toString();
        QString newName = newActiveDoc->property("Name").toString();
        bool isSaved = !newPath.isEmpty() && QFile::exists(newPath);

        if (isSaved) {
            resultPath = QDir::toNativeSeparators(newPath);
        } else {
            resultPath = newName + " (未保存)";
        }
        delete newActiveDoc;
    } else {
        // 没有打开的文档了，resultPath 保持为空
        resultPath = "";
    }

    delete catia;
    CoUninitialize();

    // 发送信号更新 UI
    // 如果 resultPath 为空，表示没有打开的文档，lineEdit_2 和 lineEdit_4 都应该清空
    emit closeFileInfo(displayText, resultPath);
}

// ========== 激活指定文档 ==========
void DelmiaWorker::doActivateDocument(int docIndex)
{
    qDebug() << "[Delmia] 激活文档，索引:" << docIndex;

    CoInitialize(NULL);
    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        return;
    }

    QAxObject *documents = catia->querySubObject("Documents");
    if (documents && !documents->isNull()) {
        QAxObject *doc = documents->querySubObject("Item(int)", docIndex);
        if (doc && !doc->isNull()) {
            doc->dynamicCall("Activate()");
            qDebug() << "[Delmia] 已激活文档索引:" << docIndex;
            delete doc;
        }
        delete documents;
    }

    delete catia;
    CoUninitialize();
}

// ========== 带路径保存文件（另存为） ==========
void DelmiaWorker::doSaveFileWithPath(const QString &path)
{
    CoInitialize(NULL);
    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        emit saveFileResult("");
        return;
    }

    QAxObject *activeDoc = catia->querySubObject("ActiveDocument");
    if (activeDoc && !activeDoc->isNull()) {
        // 确保目录存在
        QDir dir(QFileInfo(path).absolutePath());
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        QString nativePath = QDir::toNativeSeparators(path);
        activeDoc->dynamicCall("SaveAs(const QString&)", nativePath);
        QString savedPath = activeDoc->property("FullName").toString();
        emit saveFileResult(savedPath);
        delete activeDoc;
    } else {
        emit saveFileResult("");
    }

    delete catia;
    CoUninitialize();
}


// ========== 按名称激活指定文档 ==========
void DelmiaWorker::doActivateDocumentByName(const QString &docName)
{
    qDebug() << "[Delmia] 按名称激活文档:" << docName;

    CoInitialize(NULL);
    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        return;
    }

    QAxObject *documents = catia->querySubObject("Documents");
    if (documents && !documents->isNull()) {
        QAxObject *doc = documents->querySubObject("Item(const QString&)", docName);
        if (doc && !doc->isNull()) {
            doc->dynamicCall("Activate()");
            qDebug() << "[Delmia] 已激活文档:" << docName;
            delete doc;
        } else {
            qDebug() << "[Delmia] 未找到文档:" << docName;
        }
        delete documents;
    }

    delete catia;
    CoUninitialize();
}

// ========== 按名称关闭指定文档 ==========
void DelmiaWorker::doCloseDocumentByName(const QString &docName)
{
    qDebug() << "[Delmia] 按名称关闭文档:" << docName;

    CoInitialize(NULL);
    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        return;
    }

    QAxObject *documents = catia->querySubObject("Documents");
    if (documents && !documents->isNull()) {
        QAxObject *doc = documents->querySubObject("Item(const QString&)", docName);
        if (doc && !doc->isNull()) {
            doc->dynamicCall("Close()");
            qDebug() << "[Delmia] 已关闭文档:" << docName;
            delete doc;
        } else {
            qDebug() << "[Delmia] 未找到文档:" << docName;
        }
        delete documents;
    }

    delete catia;
    CoUninitialize();
}

void DelmiaWorker::doAssemblePartToProduct(const QString &productDocName, const QString &partDocName)
{
    qDebug() << "[Delmia] 装配 Part 到 Product:" << partDocName << "->" << productDocName;

    CoInitialize(NULL);

    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        emit partAssembled(partDocName, productDocName);
        return;
    }

    QAxObject *documents = catia->querySubObject("Documents");
    if (!documents || documents->isNull()) {
        delete catia;
        CoUninitialize();
        emit partAssembled(partDocName, productDocName);
        return;
    }

    // 通过名称获取已打开的 Product 文档
    QAxObject *productDoc = documents->querySubObject("Item(const QString&)", productDocName);
    if (!productDoc || productDoc->isNull()) {
        qDebug() << "[Delmia] 未找到已打开的 Product 文档:" << productDocName;
        delete documents;
        delete catia;
        CoUninitialize();
        emit partAssembled(partDocName, productDocName);
        return;
    }

    // 获取 Product 的根产品
    QAxObject *rootProduct = productDoc->querySubObject("Product");
    if (rootProduct && !rootProduct->isNull()) {
        // 获取 Products 集合
        QAxObject *products = rootProduct->querySubObject("Products");
        if (products && !products->isNull()) {
            // 通过名称获取已打开的 Part 文档
            QAxObject *partDoc = documents->querySubObject("Item(const QString&)", partDocName);
            if (partDoc && !partDoc->isNull()) {
                // 获取 Part 文档的根产品作为参考
                QAxObject *partRootProduct = partDoc->querySubObject("Product");
                if (partRootProduct && !partRootProduct->isNull()) {
                    IDispatch *partDispatch = partRootProduct->asVariant().value<IDispatch*>();
                    QVariant variant = QVariant::fromValue(partDispatch);
                    QAxObject *newComponent = products->querySubObject("AddComponent(QAxObject*)", variant);
                    if (newComponent && !newComponent->isNull()) {
                        qDebug() << "[Delmia] Part 装配成功:" << partDocName;
                        delete newComponent;
                    } else {
                        qDebug() << "[Delmia] Part 装配失败";
                    }
                    delete partRootProduct;
                }
                delete partDoc;
            }
            delete products;
        }
        delete rootProduct;
    }

    // 保存 Product 文档
    productDoc->dynamicCall("Save()");
    delete productDoc;
    delete documents;
    delete catia;
    CoUninitialize();

    // 装配完成后发送带参数的信号
    emit partAssembled(partDocName, productDocName);
}

void DelmiaWorker::doAssembleProductToParent(const QString &parentDocName, const QString &childDocName)
{
    qDebug() << "[Delmia] 装配 Product 到父 Product:" << childDocName << "->" << parentDocName;

    CoInitialize(NULL);

    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        emit productAssembled(childDocName, parentDocName);
        return;
    }

    QAxObject *documents = catia->querySubObject("Documents");
    if (!documents || documents->isNull()) {
        delete catia;
        CoUninitialize();
        emit productAssembled(childDocName, parentDocName);
        return;
    }

    // 通过名称获取已打开的父 Product 文档
    QAxObject *parentDoc = documents->querySubObject("Item(const QString&)", parentDocName);
    if (!parentDoc || parentDoc->isNull()) {
        qDebug() << "[Delmia] 未找到已打开的父 Product 文档:" << parentDocName;
        delete documents;
        delete catia;
        CoUninitialize();
        emit productAssembled(childDocName, parentDocName);
        return;
    }

    // 获取父 Product 的根产品
    QAxObject *parentRootProduct = parentDoc->querySubObject("Product");
    if (parentRootProduct && !parentRootProduct->isNull()) {
        // 获取 Products 集合
        QAxObject *products = parentRootProduct->querySubObject("Products");
        if (products && !products->isNull()) {
            // 通过名称获取已打开的子 Product 文档
            QAxObject *childDoc = documents->querySubObject("Item(const QString&)", childDocName);
            if (childDoc && !childDoc->isNull()) {
                // 获取子 Product 文档的根产品作为参考
                QAxObject *childRootProduct = childDoc->querySubObject("Product");
                if (childRootProduct && !childRootProduct->isNull()) {
                    IDispatch *childDispatch = childRootProduct->asVariant().value<IDispatch*>();
                    QVariant variant = QVariant::fromValue(childDispatch);
                    QAxObject *newComponent = products->querySubObject("AddComponent(QAxObject*)", variant);
                    if (newComponent && !newComponent->isNull()) {
                        qDebug() << "[Delmia] Product 装配成功:" << childDocName;
                        delete newComponent;
                    } else {
                        qDebug() << "[Delmia] Product 装配失败";
                    }
                    delete childRootProduct;
                }
                delete childDoc;
            }
            delete products;
        }
        delete parentRootProduct;
    }

    // 保存父 Product 文档
    parentDoc->dynamicCall("Save()");
    delete parentDoc;
    delete documents;
    delete catia;
    CoUninitialize();

    // 装配完成后发送带参数的信号
    emit productAssembled(childDocName, parentDocName);
}

void DelmiaWorker::doAddProductToProcess(const QString &processPath, const QString &productPath)
{
    qDebug() << "[Delmia] 添加 Product 到 Process";

    QFileInfo processFileInfo(processPath);
    QFileInfo productFileInfo(productPath);

    QFileInfo productInfo(productPath);
    QFileInfo processInfo(processPath);

    if (!processFileInfo.exists() || !productFileInfo.exists()) {
        emit productAddedToProcess(productInfo.fileName(), processInfo.fileName());
        return;
    }

    CoInitialize(NULL);

    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        emit productAddedToProcess(productInfo.fileName(), processInfo.fileName());
        return;
    }

    QAxObject *documents = catia->querySubObject("Documents");
    if (!documents || documents->isNull()) {
        delete catia;
        CoUninitialize();
        emit productAddedToProcess(productInfo.fileName(), processInfo.fileName());
        return;
    }

    // 先尝试获取已打开的 Process 文档
    QString processDocName = processFileInfo.fileName();
    QAxObject *processDoc = documents->querySubObject("Item(const QString&)", processDocName);

    if (!processDoc || processDoc->isNull()) {
        QString absProcessPath = processFileInfo.absoluteFilePath();
        absProcessPath.replace("/", "\\");
        processDoc = documents->querySubObject("Open(const QString&)", absProcessPath);

        if (!processDoc || processDoc->isNull()) {
            delete documents;
            delete catia;
            CoUninitialize();
            emit productAddedToProcess(productInfo.fileName(), processInfo.fileName());
            return;
        }
    }

    processDoc->dynamicCall("Activate()");

    QAxObject *pprDocument = processDoc->querySubObject("PPRDocument");

    if (pprDocument && !pprDocument->isNull()) {
        QAxObject *products = pprDocument->querySubObject("Products");
        if (products && !products->isNull()) {
            // 先尝试获取已打开的 Product 文档
            QString productDocName = productFileInfo.fileName();
            QAxObject *productDoc = documents->querySubObject("Item(const QString&)", productDocName);

            if (!productDoc || productDoc->isNull()) {
                QString absProductPath = productFileInfo.absoluteFilePath();
                absProductPath.replace("/", "\\");
                productDoc = documents->querySubObject("Open(const QString&)", absProductPath);
            }

            if (productDoc && !productDoc->isNull()) {
                QAxObject *rootProduct = productDoc->querySubObject("Product");
                if (rootProduct && !rootProduct->isNull()) {
                    IDispatch *productDispatch = rootProduct->asVariant().value<IDispatch*>();
                    QVariant variant = QVariant::fromValue(productDispatch);
                    QAxObject *addedProduct = products->querySubObject("Add(QAxObject*)", variant);

                    if (addedProduct && !addedProduct->isNull()) {
                        delete addedProduct;
                    }
                    delete rootProduct;
                }
                delete productDoc;
            }
            delete products;
        }
        delete pprDocument;
    }

    processDoc->dynamicCall("Save()");
    delete processDoc;
    delete documents;
    delete catia;
    CoUninitialize();

    emit productAddedToProcess(productInfo.fileName(), processInfo.fileName());
}

void DelmiaWorker::doAddFourPointsToPart(const QString &partDocName,
                                         double x1, double y1, double z1,
                                         double x2, double y2, double z2,
                                         double x3, double y3, double z3,
                                         double x4, double y4, double z4)
{
    qDebug() << "[Delmia] 添加/修改四个点:" << partDocName;

    CoInitialize(NULL);

    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (!catia || catia->isNull()) {
        delete catia;
        CoUninitialize();
        return;
    }

    QAxObject *documents = catia->querySubObject("Documents");
    if (!documents || documents->isNull()) {
        delete catia;
        CoUninitialize();
        return;
    }

    QAxObject *partDoc = documents->querySubObject("Item(const QString&)", partDocName);
    if (!partDoc || partDoc->isNull()) {
        qDebug() << "[Delmia] 未找到 Part 文档:" << partDocName;
        delete documents;
        delete catia;
        CoUninitialize();
        return;
    }

    QAxObject *part = partDoc->querySubObject("Part");
    if (!part || part->isNull()) {
        delete partDoc;
        delete documents;
        delete catia;
        CoUninitialize();
        return;
    }

    // 四个点的坐标
    QList<QList<double>> newCoords = {
        {x1, y1, z1}, {x2, y2, z2}, {x3, y3, z3}, {x4, y4, z4}
    };
    QStringList pointNames = {"点.1", "点.2", "点.3", "点.4"};

    // 检查点是否存在
    bool allPointsExist = true;
    for (int i = 0; i < 4; ++i) {
        QAxObject *point = part->querySubObject("FindObjectByName(const QString&)", pointNames[i]);
        if (!point || point->isNull()) {
            allPointsExist = false;
            delete point;
            break;
        }
        delete point;
    }

    if (allPointsExist) {
        // 修改现有四个点的坐标
        for (int i = 0; i < 4; ++i) {
            QAxObject *point = part->querySubObject("FindObjectByName(const QString&)", pointNames[i]);
            if (!point || point->isNull()) continue;

            QAxObject *xParam = point->querySubObject("X");
            QAxObject *yParam = point->querySubObject("Y");
            QAxObject *zParam = point->querySubObject("Z");

            if (xParam && yParam && zParam) {
                xParam->dynamicCall("SetValue(double)", newCoords[i][0]);
                yParam->dynamicCall("SetValue(double)", newCoords[i][1]);
                zParam->dynamicCall("SetValue(double)", newCoords[i][2]);
                qDebug() << "[Delmia] 已修改点:" << pointNames[i];
            }

            delete xParam;
            delete yParam;
            delete zParam;
            delete point;
        }
    } else {
        // 创建新的几何图形集和点
        qDebug() << "[Delmia] 创建几何图形集和点";

        QAxObject *hybridShapeFactory = part->querySubObject("HybridShapeFactory");
        QAxObject *hybridBodies = part->querySubObject("HybridBodies");

        if (hybridShapeFactory && hybridBodies) {
            // 查找或创建几何图形集
            QAxObject *geometricalSet = hybridBodies->querySubObject("Item(const QString&)", "FourPoints");
            if (!geometricalSet || geometricalSet->isNull()) {
                geometricalSet = hybridBodies->querySubObject("Add()");
                if (geometricalSet) geometricalSet->setProperty("Name", "FourPoints");
            } else {
                // 清空已有内容
                QAxObject *shapes = geometricalSet->querySubObject("Shapes");
                if (shapes) {
                    int count = shapes->property("Count").toInt();
                    for (int i = count; i >= 1; --i) {
                        QAxObject *shape = shapes->querySubObject("Item(int)", i);
                        if (shape) {
                            geometricalSet->dynamicCall("Remove(QAxObject*)",
                                                        QVariant::fromValue(shape->asVariant().value<IDispatch*>()));
                            delete shape;
                        }
                    }
                    delete shapes;
                }
            }

            // 添加四个点
            if (geometricalSet) {
                for (int i = 0; i < 4; ++i) {
                    QAxObject *point = hybridShapeFactory->querySubObject(
                        "AddNewPointCoord(double, double, double)",
                        newCoords[i][0], newCoords[i][1], newCoords[i][2]);
                    if (point) {
                        geometricalSet->dynamicCall("AppendHybridShape(QAxObject*)",
                                                    QVariant::fromValue(point->asVariant().value<IDispatch*>()));
                        delete point;
                    }
                }
                delete geometricalSet;
            }
        }

        delete hybridShapeFactory;
        delete hybridBodies;
    }

    // 更新并保存
    part->dynamicCall("Update()");
    partDoc->dynamicCall("Save()");

    delete part;
    delete partDoc;
    delete documents;
    delete catia;
    CoUninitialize();

    qDebug() << "[Delmia] 操作完成";
}

// ========== 获取龙门位置（执行宏 + 读取注册表） ==========
void DelmiaWorker::doGetGantryPosition(const QString &gantryName)
{
    qDebug() << "[Delmia] 获取机械装置位置:" << gantryName;

    // ===== 1. 执行宏 =====
    CoInitialize(NULL);
    QAxObject *catia = new QAxObject("DELMIA.Application", nullptr);
    if (catia && !catia->isNull()) {
        catia->dynamicCall("StartCommand(const QString&)", "获取龙门");
        delete catia;
    }
    CoUninitialize();

    // // ===== 2. 等待宏执行完成 =====
    // QThread::msleep(100);

    // ===== 3. 从注册表读取 =====
    QSettings settings("HKEY_CURRENT_USER\\Software\\SoundScan", QSettings::NativeFormat);

    double x = settings.value("GantryX_mm", "0").toString().toDouble();
    double y = settings.value("GantryY_mm", "0").toString().toDouble();

    qDebug() << "[Delmia] 龙门位置: X=" << x << " mm, Y=" << y << " mm";
    emit gantryPositionResult(gantryName, x, y);
}


void DelmiaWorker::doCreateProject(const QString &sourcePartPath,
                                   const QString &projectPath,
                                   const QString &projectName)
{
    qDebug() << "[Delmia] 开始创建项目:" << projectName;

    // 目标文件夹路径
    QString destPath = projectPath + "/" + projectName;

    // 检查目标文件夹是否已存在
    if (QDir(destPath).exists()) {
        emit projectCreated(false, tr("文件夹已存在: %1").arg(destPath));
        return;
    }

    // 创建项目主目录
    if (!QDir().mkpath(destPath)) {
        emit projectCreated(false, tr("无法创建项目目录: %1").arg(destPath));
        return;
    }

    // 复制文件
    QString sourceProcessPath = "C:/超声扫描/仿真/SoundScan/SoundScan.CATProcess";
    QString targetProcessPath = destPath + "/" + projectName + ".CATProcess";
    if (QFile::exists(sourceProcessPath)) {
        QFile::copy(sourceProcessPath, targetProcessPath);
    }

    QString targetPartPath = destPath + "/" + projectName + ".CATPart";
    if (QFile::exists(sourcePartPath)) {
        QFile::copy(sourcePartPath, targetPartPath);
    }

    // 需要新建的文件列表
    struct FileInfo {
        QString fileName;
        QString delmiaType;
        QString extension;
    };

    QList<FileInfo> filesToCreate;
    filesToCreate << FileInfo{ QString("%1_ZONG").arg(projectName), "CATProduct", ".CATProduct" }
                  << FileInfo{ QString("%1_ZB").arg(projectName), "CATPart", ".CATPart" };

    // 创建并保存文件
    for (const FileInfo &fileInfo : filesToCreate) {
        QString fullPath = destPath + "/" + fileInfo.fileName + fileInfo.extension;

        doNewFile(fileInfo.delmiaType, fileInfo.fileName);
        QCoreApplication::processEvents();

        doSaveFileWithPath(fullPath);
        QCoreApplication::processEvents();
    }

    // ========== 发送信号：文件已创建完成 ==========
    QString partZBDocName = projectName + "_ZB.CATPart";
    emit projectFilesCreated(projectName, partZBDocName);
    qDebug() << "[Delmia] 文件创建完成，已发送 projectFilesCreated 信号";

    // 获取文档名称
    QString partDocName = projectName + ".CATPart";
    QString productZONGDocName = projectName + "_ZONG.CATProduct";
    QString processDocName = projectName + ".CATProcess";

    // 打开复制过来的主零件文件
    doOpenFile(targetPartPath);
    QCoreApplication::processEvents();

    // 确保 Product 文档是活动文档
    doActivateDocumentByName(productZONGDocName);
    QCoreApplication::processEvents();

    // 再次确保 Product 文档是活动文档
    doActivateDocumentByName(productZONGDocName);
    QCoreApplication::processEvents();

    // ========== 装配操作 ==========
    doAssemblePartToProduct(productZONGDocName, partDocName);
    QCoreApplication::processEvents();

    doAssemblePartToProduct(productZONGDocName, partZBDocName);
    QCoreApplication::processEvents();

    // 激活 Process 文档
    doActivateDocumentByName(processDocName);
    QCoreApplication::processEvents();

    // 关闭 ZB 文件
    doCloseDocumentByName(partZBDocName);
    QCoreApplication::processEvents();

    // 添加 Product 到 Process
    QString processFullPath = QDir::toNativeSeparators(destPath + "/" + projectName + ".CATProcess");
    QString productZONGPath = QDir::toNativeSeparators(destPath + "/" + projectName + "_ZONG.CATProduct");

    doAddProductToProcess(processFullPath, productZONGPath);
    QCoreApplication::processEvents();

    // 关闭 ZONG 文件
    doCloseDocumentByName(productZONGDocName);
    QCoreApplication::processEvents();

    // 激活 Part 文档
    doActivateDocumentByName(partDocName);
    QCoreApplication::processEvents();

    // 项目创建完成
    QString successMsg = tr("项目创建成功！");

    emit projectCreated(true, successMsg);
    qDebug() << "[Delmia] 项目创建完成:" << projectName;
}

// void DelmiaWorker::doCreateProject(const QString &sourcePartPath,
//                                    const QString &projectPath,
//                                    const QString &projectName)
// {
//     qDebug() << "[Delmia] 开始创建项目:" << projectName;

//     // 目标文件夹路径
//     QString destPath = projectPath + "/" + projectName;

//     // 检查目标文件夹是否已存在
//     if (QDir(destPath).exists()) {
//         emit projectCreated(false, tr("文件夹已存在: %1").arg(destPath));
//         return;
//     }

//     // 创建项目主目录
//     if (!QDir().mkpath(destPath)) {
//         emit projectCreated(false, tr("无法创建项目目录: %1").arg(destPath));
//         return;
//     }

//     // 复制 SoundScan.CATProcess 并重命名
//     QString sourceProcessPath = "C:/超声扫描/仿真/SoundScan/SoundScan.CATProcess";
//     QString targetProcessPath = destPath + "/" + projectName + ".CATProcess";
//     if (QFile::exists(sourceProcessPath)) {
//         QFile::copy(sourceProcessPath, targetProcessPath);
//     }

//     // 复制用户选择的 .CATPart 文件作为项目的主零件
//     QString targetPartPath = destPath + "/" + projectName + ".CATPart";
//     if (QFile::exists(sourcePartPath)) {
//         QFile::copy(sourcePartPath, targetPartPath);
//     }

//     // 需要新建的文件列表
//     struct FileInfo {
//         QString fileName;
//         QString delmiaType;
//         QString extension;
//     };

//     QList<FileInfo> filesToCreate;
//     filesToCreate << FileInfo{ QString("%1_ZONG").arg(projectName), "CATProduct", ".CATProduct" }
//                   << FileInfo{ QString("%1_ZB").arg(projectName), "CATPart", ".CATPart" };

//     // 创建并保存文件
//     for (const FileInfo &fileInfo : filesToCreate) {
//         QString fullPath = destPath + "/" + fileInfo.fileName + fileInfo.extension;

//         doNewFile(fileInfo.delmiaType, fileInfo.fileName);
//         QCoreApplication::processEvents();

//         doSaveFileWithPath(fullPath);
//         QCoreApplication::processEvents();
//     }

//     // ========== 发送信号：文件已创建完成 ==========
//     QString partZBDocName = projectName + "_ZB.CATPart";
//     emit projectFilesCreated(projectName, partZBDocName);
//     qDebug() << "[Delmia] 文件创建完成，已发送 projectFilesCreated 信号";

//     // 获取文档名称
//     QString partDocName = projectName + ".CATPart";
//     QString productZONGDocName = projectName + "_ZONG.CATProduct";
//     QString processDocName = projectName + ".CATProcess";

//     // 打开复制过来的主零件文件
//     doOpenFile(targetPartPath);
//     QCoreApplication::processEvents();
//     Sleep(500);

//     // 确保 Product 文档是活动文档
//     doActivateDocumentByName(productZONGDocName);
//     QCoreApplication::processEvents();
//     Sleep(300);

//     // 再次确保 Product 文档是活动文档
//     doActivateDocumentByName(productZONGDocName);
//     QCoreApplication::processEvents();
//     Sleep(300);

//     // ========== 装配操作 ==========
//     // 装配 Part 到 Product
//     doAssemblePartToProduct(productZONGDocName, partDocName);
//     QCoreApplication::processEvents();
//     Sleep(300);

//     // 装配 ZB Part 到 Product
//     doAssemblePartToProduct(productZONGDocName, partZBDocName);
//     QCoreApplication::processEvents();
//     Sleep(300);

//     // 激活 Process 文档
//     doActivateDocumentByName(processDocName);
//     QCoreApplication::processEvents();
//     Sleep(500);

//     // 关闭 ZB 文件
//     doCloseDocumentByName(partZBDocName);
//     QCoreApplication::processEvents();
//     Sleep(300);

//     // 添加 Product 到 Process
//     QString processFullPath = QDir::toNativeSeparators(destPath + "/" + projectName + ".CATProcess");
//     QString productZONGPath = QDir::toNativeSeparators(destPath + "/" + projectName + "_ZONG.CATProduct");

//     doAddProductToProcess(processFullPath, productZONGPath);
//     QCoreApplication::processEvents();
//     Sleep(500);

//     // 关闭 ZONG 文件
//     doCloseDocumentByName(productZONGDocName);
//     QCoreApplication::processEvents();
//     Sleep(300);

//     // 项目创建完成
//     QString successMsg = tr("项目创建成功！\n\n"
//                             "📁 %1\n"
//                             "   ├─ %2.CATProcess\n"
//                             "   ├─ %2_ZONG.CATProduct\n"
//                             "   ├─ %2.CATPart (已复制)\n"
//                             "   └─ %2_ZB.CATPart")
//                              .arg(QDir::toNativeSeparators(destPath))
//                              .arg(projectName);

//     emit projectCreated(true, successMsg);
//     qDebug() << "[Delmia] 项目创建完成:" << projectName;
// }