#include "3DScan/scan.h"
#include "3DScan/scandata.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <cstdio>

static void msgHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    fprintf(stderr, "[QT] %s\n", msg.toLocal8Bit().constData());
    fflush(stderr);
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    qInstallMessageHandler(msgHandler);
    if (argc < 3) { fprintf(stderr, "usage: harness <csv> <out.txt>\n"); return 2; }
    QString csv = QString::fromLocal8Bit(argv[1]);
    FILE* out = fopen(argv[2], "w");
    if (!out) { fprintf(stderr, "cannot open output\n"); return 2; }

    Scan scan;
    scan.loadCSVAndBuild(csv);
    const std::vector<ScanFrame>& frames = scan.loadedFrames();
    for (const ScanFrame& f : frames) {
        for (int i = 0; i < f.worldCount; i++) {
            fprintf(out, "%d %d %d %.3f %.3f %.3f\n",
                    f.gridK[i], f.gridJ[i], f.passIndex,
                    f.worldXYZ[i*3+0], f.worldXYZ[i*3+1], f.worldXYZ[i*3+2]);
        }
    }
    fclose(out);
    return 0;
}
