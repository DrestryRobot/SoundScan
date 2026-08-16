QT       += core gui network charts widgets xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets axcontainer opengl openglwidgets

CONFIG += c++17
CONFIG += force_debug_info

# Prevent Windows min/max macros from breaking std::min/std::max
DEFINES += NOMINMAX

# Force qmake to emit absolute paths in Makefiles. Fixes
# "dependent '..\..\..\..\..\Qt\...qspinbox.h' does not exist"
# when the build directory is too deep for relative paths.
QMAKE_PROJECT_DEPTH = 0

# Shared NDT code lives in the Phaselink subproject (same as 3dscan:
# sources below are compiled from that folder via relative paths).
INCLUDEPATH += $$PWD/Phaselink

# TwinCAT ADS (Beckhoff)
INCLUDEPATH += "C:\TwinCAT\AdsApi\TcAdsDll\Include"
LIBS += -L"C:\TwinCAT\AdsApi\TcAdsDll\x64\lib" -lTcAdsDll

# Phaselink SDK
INCLUDEPATH += "C:\Phaselink\include"
LIBS += -L"C:\Phaselink\lib" -lclient

# LibKuka3D
INCLUDEPATH += "C:\LibKuka3D\include"
LIBS += -L"C:\LibKuka3D\lib" -lLibKuka3D

# Python 3.13
INCLUDEPATH += C:/Users/23714/AppData/Local/Programs/Python/Python313/include
LIBS += -LC:/Users/23714/AppData/Local/Programs/Python/Python313/libs -lpython313

# VTK 9.6
INCLUDEPATH += $$"C:/Program Files (x86)/VTK/include/vtk-9.6"
LIBS += -L$$"C:/Program Files (x86)/VTK/lib"

LIBS += -lvtkGUISupportQt-9.6 \
        -lvtkRenderingQt-9.6 \
        -lvtkRenderingOpenGL2-9.6 \
        -lvtkRenderingCore-9.6 \
        -lvtkRenderingAnnotation-9.6 \
        -lvtkRenderingFreeType-9.6 \
        -lvtkRenderingVolume-9.6 \
        -lvtkRenderingLabel-9.6 \
        -lvtkRenderingVolumeOpenGL2-9.6 \
        -lvtkInteractionStyle-9.6 \
        -lvtkInteractionWidgets-9.6 \
        -lvtkCommonCore-9.6 \
        -lvtkCommonMath-9.6 \
        -lvtkCommonDataModel-9.6 \
        -lvtkCommonExecutionModel-9.6 \
        -lvtkCommonTransforms-9.6 \
        -lvtkFiltersCore-9.6 \
        -lvtkFiltersSources-9.6 \
        -lvtkFiltersGeneral-9.6 \
        -lvtkFiltersModeling-9.6 \
        -lvtkFiltersGeometry-9.6 \
        -lvtkIOImage-9.6 \
        -lvtkIOPLY-9.6 \
        -lvtkImagingGeneral-9.6 \
        -lvtkImagingCore-9.6 \
        -lvtkImagingHybrid-9.6 \
        -lvtkImagingSources-9.6 \
        -lvtkViewsQt-9.6 \
        -lvtksys-9.6

# ============ CUDA (3dscan/algorithm.cu) ============
CUDA_DIR = C:/CUDA/v13.3
CUDA_ARCH = sm_86
INCLUDEPATH += $$PWD $$PWD/3dscan $$CUDA_DIR/include $$CUDA_DIR/common/inc
QMAKE_LIBDIR += $$CUDA_DIR/lib/x64
LIBS += -lcudart -lcublas -lcufft
CONFIG(debug, debug|release) {
    CUDA_XCOMPILER = /MDd
} else {
    CUDA_XCOMPILER = /MD
}
# Compile algorithm.cu with nvcc before linking (works for any build dir).
cuda_prebuild.target = $$OUT_PWD/algorithm.obj
cuda_prebuild.commands = cmd /c "\"$$CUDA_DIR/bin/nvcc.exe --machine 64 -arch=$$CUDA_ARCH -c -o $$OUT_PWD/algorithm.obj $$PWD/3dscan/algorithm.cu -Xcompiler $$CUDA_XCOMPILER\""
cuda_prebuild.depends = $$PWD/3dscan/algorithm.cu $$PWD/3dscan/algorithm.h
QMAKE_EXTRA_TARGETS += cuda_prebuild
PRE_TARGETDEPS += $$OUT_PWD/algorithm.obj
OBJECTS += $$OUT_PWD/algorithm.obj
OTHER_FILES += $$PWD/3dscan/algorithm.cu

# SoundScan-only sources
SOURCES += \
    ads_client.cpp \
    ads_read_thread.cpp \
    colormanager.cpp \
    Phaselink/datadispatch.cpp \
    debugoutput.cpp \
    delmiaworker.cpp \
    Phaselink/dialog/viewmodel.cpp \
    main.cpp \
    mainwindow1.cpp \
    Phaselink/mainwindow.cpp \
    mainwindow2.cpp \
    3dscan/mainwindow3.cpp \
    3dscan/datapanel.cpp \
    3dscan/scandata.cpp \
    3dscan/scan.cpp \
    3dscan/vtkvboactor.cpp \
    tcpserver.cpp \
    udpserver.cpp

# Shared code compiled from Phaselink (subproject, like 3dscan)
SOURCES += \
    Phaselink/app.cpp \
    Phaselink/ndtbase.cpp \
    Phaselink/UI/scanning.cpp \
    Phaselink/UI/UT/acg_tcg_widget.cpp \
    Phaselink/UI/UT/essentialwidget.cpp \
    Phaselink/UI/UT/gate_widget.cpp \
    Phaselink/UI/UT/phase_array.cpp \
    Phaselink/UI/UT/ut_widget.cpp \
    Phaselink/dialog/addsud_group.cpp \
    Phaselink/dialog/amplitudpalette.cpp \
    Phaselink/dialog/axis_utils.cpp \
    Phaselink/dialog/colordialog.cpp \
    Phaselink/dialog/dataprocessor.cpp \
    Phaselink/dialog/listwidget.cpp \
    Phaselink/dialog/measurewidget.cpp \
    Phaselink/dialog/packetdatasaver.cpp \
    Phaselink/dialog/parammanager.cpp \
    Phaselink/dialog/rulerwidget.cpp \
    Phaselink/dialog/sider.cpp \
    Phaselink/dialog/viewwidget.cpp \
    Phaselink/dialog/viewworker.cpp

SOURCES += \
    $$PWD/Phaselink/simulation/SimDataPlayer.cpp

HEADERS += \
    ads_client.h \
    ads_read_thread.h \
    colormanager.h \
    debugoutput.h \
    delmiaworker.h \
    Phaselink/dialog/viewmodel.h \
    3dscan/scandata.h \
    Phaselink/datadispatch.h \
    mainwindow1.h \
    Phaselink/mainwindow.h \
    mainwindow2.h \
    3dscan/mainwindow3.h \
    3dscan/datapanel.h \
    3dscan/scan.h \
    3dscan/algorithm.h \
    3dscan/vtkvboactor.h \
    tcpserver.h \
    udpserver.h

HEADERS += \
    Phaselink/app.h \
    Phaselink/ndtbase.h \
    Phaselink/UI/scanning.h \
    Phaselink/UI/UT/acg_tcg_widget.h \
    Phaselink/UI/UT/essentialwidget.h \
    Phaselink/UI/UT/gate_widget.h \
    Phaselink/UI/UT/phase_array.h \
    Phaselink/UI/UT/ut_widget.h \
    Phaselink/dialog/adddeviceDialog.h \
    Phaselink/dialog/addsud_group.h \
    Phaselink/dialog/amplitudpalette.h \
    Phaselink/dialog/axis_utils.h \
    Phaselink/dialog/colordialog.h \
    Phaselink/dialog/dataprocessor.h \
    Phaselink/dialog/listwidget.h \
    Phaselink/dialog/measurewidget.h \
    Phaselink/dialog/packetdatasaver.h \
    Phaselink/dialog/parammanager.h \
    Phaselink/dialog/rulerwidget.h \
    Phaselink/dialog/sider.h \
    Phaselink/dialog/viewwidget.h \
    Phaselink/dialog/viewworker.h

HEADERS += \
    $$PWD/Phaselink/simulation/SimDataPlayer.h

FORMS += \
    Phaselink/UI/scanning.ui \
    Phaselink/UI/UT/acg_tcg_widget.ui \
    Phaselink/UI/UT/essentialwidget.ui \
    Phaselink/UI/UT/gate_widget.ui \
    Phaselink/UI/UT/phase_array.ui \
    Phaselink/UI/UT/ut_widget.ui \
    Phaselink/dialog/addsud_group.ui \
    Phaselink/dialog/amplitudpalette.ui \
    Phaselink/dialog/colordialog.ui \
    Phaselink/dialog/measurewidget.ui \
    Phaselink/dialog/rulerwidget.ui \
    Phaselink/dialog/sider.ui \
    Phaselink/dialog/viewwidget.ui \
    mainwindow1.ui \
    Phaselink/mainwindow.ui \
    mainwindow2.ui \
    3dscan/mainwindow3.ui

RESOURCES += resources.qrc \
    Phaselink/Qss.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += -lopengl32
LIBS += -lglu32

win32: RC_ICONS +=
