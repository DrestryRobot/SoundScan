QT       += core gui network charts widgets xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets axcontainer opengl openglwidgets

CONFIG += c++17

# Prevent Windows min/max macros from breaking std::min/std::max
DEFINES += NOMINMAX

# Force qmake to emit absolute paths in Makefiles. Fixes
# "dependent '..\..\..\..\..\Qt\...qspinbox.h' does not exist"
# when the build directory is too deep for relative paths.
QMAKE_PROJECT_DEPTH = 0

# Shared NDT code is owned by the phaselink_code subproject (same as 3DScan:
# sources below are compiled from that folder via relative paths).
INCLUDEPATH += $$PWD/phaselink_code

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

# ============ CUDA (3DScan/algorithm.cu) ============
CUDA_DIR = C:/CUDA/v13.3
CUDA_ARCH = sm_86
INCLUDEPATH += $$PWD $$PWD/3DScan $$CUDA_DIR/include $$CUDA_DIR/common/inc
QMAKE_LIBDIR += $$CUDA_DIR/lib/x64
LIBS += -lcudart -lcublas -lcufft
CONFIG(debug, debug|release) {
    CUDA_XCOMPILER = /MDd
} else {
    CUDA_XCOMPILER = /MD
}
# Compile algorithm.cu with nvcc before linking (works for any build dir).
cuda_prebuild.target = $$OUT_PWD/algorithm.obj
cuda_prebuild.commands = cmd /c "\"$$CUDA_DIR/bin/nvcc.exe --machine 64 -arch=$$CUDA_ARCH -c -o $$OUT_PWD/algorithm.obj $$PWD/3DScan/algorithm.cu -Xcompiler $$CUDA_XCOMPILER\""
cuda_prebuild.depends = $$PWD/3DScan/algorithm.cu $$PWD/3DScan/algorithm.h
QMAKE_EXTRA_TARGETS += cuda_prebuild
PRE_TARGETDEPS += $$OUT_PWD/algorithm.obj
OBJECTS += $$OUT_PWD/algorithm.obj
OTHER_FILES += $$PWD/3DScan/algorithm.cu

# SoundScan-only sources
SOURCES += \
    ads_client.cpp \
    ads_read_thread.cpp \
    colormanager.cpp \
    phaselink_code/datadispatch.cpp \
    debugoutput.cpp \
    delmia.cpp \
    delmiaworker.cpp \
    phaselink_code/dialog/viewmodel.cpp \
    main.cpp \
    configwindow.cpp \
    phaselink_code/mainwindow.cpp \
    mainwindow5.cpp \
    3DScan/mainwindow7.cpp \
    3DScan/scandata.cpp \
    3DScan/scan.cpp \
    3DScan/vtkvboactor.cpp \
    tcpserver.cpp \
    udpserver.cpp

# Shared code compiled from phaselink_code (subproject, like 3DScan)
SOURCES += \
    phaselink_code/app.cpp \
    phaselink_code/ndtbase.cpp \
    phaselink_code/UI/scanning.cpp \
    phaselink_code/UI/UT/acg_tcg_widget.cpp \
    phaselink_code/UI/UT/essentialwidget.cpp \
    phaselink_code/UI/UT/gate_widget.cpp \
    phaselink_code/UI/UT/phase_array.cpp \
    phaselink_code/UI/UT/ut_widget.cpp \
    phaselink_code/dialog/addsud_group.cpp \
    phaselink_code/dialog/amplitudpalette.cpp \
    phaselink_code/dialog/axis_utils.cpp \
    phaselink_code/dialog/colordialog.cpp \
    phaselink_code/dialog/dataprocessor.cpp \
    phaselink_code/dialog/listwidget.cpp \
    phaselink_code/dialog/measurewidget.cpp \
    phaselink_code/dialog/packetdatasaver.cpp \
    phaselink_code/dialog/parammanager.cpp \
    phaselink_code/dialog/rulerwidget.cpp \
    phaselink_code/dialog/sider.cpp \
    phaselink_code/dialog/viewwidget.cpp \
    phaselink_code/dialog/viewworker.cpp

SOURCES += \
    $$PWD/phaselink_code/simulation/SimDataPlayer.cpp

HEADERS += \
    ads_client.h \
    ads_read_thread.h \
    colormanager.h \
    debugoutput.h \
    delmia.h \
    delmiaworker.h \
    phaselink_code/dialog/viewmodel.h \
    3DScan/scandata.h \
    phaselink_code/datadispatch.h \
    configwindow.h \
    phaselink_code/mainwindow.h \
    mainwindow5.h \
    3DScan/mainwindow7.h \
    3DScan/scan.h \
    3DScan/algorithm.h \
    3DScan/vtkvboactor.h \
    tcpserver.h \
    udpserver.h

HEADERS += \
    phaselink_code/app.h \
    phaselink_code/ndtbase.h \
    phaselink_code/UI/scanning.h \
    phaselink_code/UI/UT/acg_tcg_widget.h \
    phaselink_code/UI/UT/essentialwidget.h \
    phaselink_code/UI/UT/gate_widget.h \
    phaselink_code/UI/UT/phase_array.h \
    phaselink_code/UI/UT/ut_widget.h \
    phaselink_code/dialog/adddeviceDialog.h \
    phaselink_code/dialog/addsud_group.h \
    phaselink_code/dialog/amplitudpalette.h \
    phaselink_code/dialog/axis_utils.h \
    phaselink_code/dialog/colordialog.h \
    phaselink_code/dialog/dataprocessor.h \
    phaselink_code/dialog/listwidget.h \
    phaselink_code/dialog/measurewidget.h \
    phaselink_code/dialog/packetdatasaver.h \
    phaselink_code/dialog/parammanager.h \
    phaselink_code/dialog/rulerwidget.h \
    phaselink_code/dialog/sider.h \
    phaselink_code/dialog/viewwidget.h \
    phaselink_code/dialog/viewworker.h

HEADERS += \
    $$PWD/phaselink_code/simulation/SimDataPlayer.h

FORMS += \
    phaselink_code/UI/scanning.ui \
    phaselink_code/UI/UT/acg_tcg_widget.ui \
    phaselink_code/UI/UT/essentialwidget.ui \
    phaselink_code/UI/UT/gate_widget.ui \
    phaselink_code/UI/UT/phase_array.ui \
    phaselink_code/UI/UT/ut_widget.ui \
    phaselink_code/dialog/addsud_group.ui \
    phaselink_code/dialog/amplitudpalette.ui \
    phaselink_code/dialog/colordialog.ui \
    phaselink_code/dialog/measurewidget.ui \
    phaselink_code/dialog/rulerwidget.ui \
    phaselink_code/dialog/sider.ui \
    phaselink_code/dialog/viewwidget.ui \
    configwindow.ui \
    phaselink_code/mainwindow.ui \
    mainwindow5.ui \
    3DScan/mainwindow7.ui

RESOURCES += resources.qrc \
    phaselink_code/Qss.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += -lopengl32
LIBS += -lglu32

win32: RC_ICONS +=
