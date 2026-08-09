QT       += core gui network charts widgets xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets axcontainer opengl

CONFIG += c++17

# Force qmake to emit absolute paths in Makefiles. Fixes
# "dependent '..\..\..\..\..\Qt\...qspinbox.h' does not exist"
# when the build directory is too deep for relative paths.
QMAKE_PROJECT_DEPTH = 0

TRANSLATIONS += $$PWD/phaselink_CN.ts $$PWD/phaselink_EN.ts

# 指定Beckhoff的头文件和库目录
INCLUDEPATH += "C:\TwinCAT\AdsApi\TcAdsDll\Include"
LIBS += -L"C:\TwinCAT\AdsApi\TcAdsDll\x64\lib" -lTcAdsDll

# 指定Phaselink的头文件和库目录
INCLUDEPATH += "C:\Phaselink\include"
LIBS += -L"C:\Phaselink\lib" -lclient

# INCLUDEPATH += "C:\Phaselink\debug\include"
# LIBS += -L"C:\Phaselink\debug\lib" -lclient

# 指定LibKuka3D的头文件和库目录
INCLUDEPATH += "C:\LibKuka3D\include"
LIBS += -L"C:\LibKuka3D\lib" -lLibKuka3D

# INCLUDEPATH += "C:\LibKuka3D\debug\include"
# LIBS += -L"C:\LibKuka3D\debug\lib" -lLibKuka3D

# 指定Python的头文件和库目录
INCLUDEPATH += C:/Users/23714/AppData/Local/Programs/Python/Python313/include
LIBS += -LC:/Users/23714/AppData/Local/Programs/Python/Python313/libs -lpython313

# 链接 VTK 头文件和库目录
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
        -lvtkCommonDataModel-9.6 \
        -lvtkCommonExecutionModel-9.6 \
        -lvtkCommonTransforms-9.6 \
        -lvtkFiltersCore-9.6 \
        -lvtkFiltersSources-9.6 \
        -lvtkFiltersGeneral-9.6 \
        -lvtkFiltersModeling-9.6 \
        -lvtkFiltersGeometry-9.6 \
        -lvtkIOImage-9.6 \
        -lvtkImagingGeneral-9.6 \
        -lvtkImagingCore-9.6 \
        -lvtkImagingHybrid-9.6 \
        -lvtkImagingSources-9.6 \
        -lvtkViewsQt-9.6 \
        -lvtksys-9.6

SOURCES += \
    UI/UT/acg_tcg_widget.cpp \
    UI/UT/essentialwidget.cpp \
    UI/UT/gate_widget.cpp \
    UI/UT/phase_array.cpp \
    UI/UT/ut_widget.cpp \
    UI/scanning.cpp \
    ads_client.cpp \
    ads_read_thread.cpp \
    app.cpp \
    colormanager.cpp \
    debugoutput.cpp \
    delmia.cpp \
    delmiaworker.cpp \
    dialog/addsud_group.cpp \
    dialog/amplitudpalette.cpp \
    dialog/axis_utils.cpp \
    dialog/colordialog.cpp \
    dialog/dataprocessor.cpp \
    dialog/listwidget.cpp \
    dialog/measurewidget.cpp \
    dialog/packetdatasaver.cpp \
    dialog/parammanager.cpp \
    dialog/rulerwidget.cpp \
    dialog/sider.cpp \
    dialog/viewmodel.cpp \
    dialog/viewmodel.cpp \
    dialog/viewwidget.cpp \
    dialog/viewworker.cpp \
    main.cpp \
    datadispatch.cpp \
    mainwindow.cpp \
    mainwindow3.cpp \
    mainwindow5.cpp \
    mainwindow7.cpp \
    ndtbase.cpp \
    tcpserver.cpp \
    udpserver.cpp

HEADERS += \
    UI/UT/acg_tcg_widget.h \
    UI/UT/essentialwidget.h \
    UI/UT/gate_widget.h \
    UI/UT/phase_array.h \
    UI/UT/ut_widget.h \
    UI/scanning.h \
    ads_client.h \
    ads_read_thread.h \
    app.h \
    colormanager.h \
    debugoutput.h \
    delmia.h \
    delmiaworker.h \
    dialog/adddeviceDialog.h \
    dialog/addsud_group.h \
    dialog/amplitudpalette.h \
    dialog/axis_utils.h \
    dialog/colordialog.h \
    dialog/dataprocessor.h \
    dialog/listwidget.h \
    dialog/measurewidget.h \
    dialog/packetdatasaver.h \
    dialog/parammanager.h \
    dialog/rulerwidget.h \
    dialog/sider.h \
    dialog/viewmodel.h \
    dialog/viewwidget.h \
    dialog/viewworker.h \
    datadispatch.h \
    mainwindow.h \
    mainwindow3.h \
    mainwindow5.h \
    mainwindow7.h \
    ndtbase.h \
    tcpserver.h \
    udpserver.h

FORMS += \
    UI/UT/acg_tcg_widget.ui \
    UI/UT/essentialwidget.ui \
    UI/UT/gate_widget.ui \
    UI/UT/phase_array.ui \
    UI/UT/ut_widget.ui \
    UI/scanning.ui \
    dialog/addsud_group.ui \
    dialog/amplitudpalette.ui \
    dialog/colordialog.ui \
    dialog/measurewidget.ui \
    dialog/rulerwidget.ui \
    dialog/sider.ui \
    dialog/viewwidget.ui \
    mainwindow.ui \
    mainwindow3.ui \
    mainwindow5.ui \
    mainwindow7.ui

RESOURCES += resources.qrc \
    Qss.qrc \
    Qss.qrc

TRANSLATIONS += \
    phaselink_CN.ts \
    phaselink_EN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    phaselink_CN.qm \
    phaselink_EN.qm \


# Windows 下链接 OpenGL 库
# win32: LIBS += -lopengl32 -lglu32

# 或者使用这种方式
LIBS += -lopengl32
LIBS += -lglu32

# 部署配置
win32: RC_ICONS +=
