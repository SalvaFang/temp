#-------------------------------------------------
#
# Project created by QtCreator 2018-11-25T21:15:54
#
#-------------------------------------------------

QT       += core gui multimedia svg

greaterThan(QT_MAJOR_VERSION, 6): QT += widgets
CODECFORTR = UTF-8
CODECFORSRC = UTF-8

TARGET = EyeAiming
TEMPLATE = app
QMAKE_CXXFLAGS += -std:c++17 -Zc:__cplusplus -permissive-
# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    calibdlg.cpp \
    caliberror.cpp \
    calibpointdlg.cpp \
    directshowopencvcamera.cpp \
    eyetrackingcalibrator.cpp \
    eyetrackingdialog.cpp \
    gazeoverlaywidget.cpp \
    main.cpp \
    newcalib.cpp \
    opencvimagecallback.cpp \
    sing/DirectShowFrameGrabber.cpp \
    sing/DirectShowGuids.c \
    sing/eye_cameras.cpp \
    sing/eye_model_updater.cpp \
    svd.cpp \
    PupilTracker.cpp \
    mainwindow.cpp \
    showcal.cpp \
    showpoint.cpp \
    ElSe.cpp \
    PupilDetectionMethod.cpp \
    testresultdlg.cpp \
    utils.cpp \
    PuRe.cpp \
    PupilTrackingMethod.cpp \
    PuReST.cpp \
    ExCuSe.cpp \
    FrameGrabber.cpp \
    worker.cpp \
    dataTable.cpp \
    graphPlot.cpp \
    qcustomplot.cpp \
    camctrl/QtCategoryButton1.cpp \
    camctrl/CameraFormBase.cpp \
    camctrl/FormExposure.cpp \
    camctrl/CameraSettingsAdapter.cpp \








HEADERS += \
    calibdlg.h \
    caliberror.h \
    calibpointdlg.h \
    directshowopencvcamera.h \
    eyetrackingcalibrator.h \
    eyetrackingdialog.h \
    gazeoverlaywidget.h \
    newcalib.h \
    opencvimagecallback.h \
    sing/AutoComPtr.h \
    sing/eye_cameras.h \
    sing/eye_model_updater.h \
    sing/fit_ellipse.h \
    sing/high_resolution_timer.hpp \
    sing/pupilFitter.h \
    sing/timer.h \
    sing/ubitrack_util.h \
    svd.h \
    PupilTracker.h \
    mainwindow.h \
    showcal.h \
    testresultdlg.h \
    showpoint.h \
    ElSe.h \
    PupilDetectionMethod.h \
    utils.h \
    inputwidget.h \
    PuRe.h \
    PupilTrackingMethod.h \
    PuReST.h \
    ExCuSe.h \
    FrameGrabber.h \
    worker.h \
    dataTable.h \
    graphPlot.h \
    Pupil.h \
    qcustomplot.h \
    camctrl/QtCategoryButton1.h \
    camctrl/CameraFormBase.h \
    camctrl/FormExposure.h \
    camctrl/CameraSettingsAdapter.h \







FORMS += \
    calibdlg.ui \
    calibpointdlg.ui \
    mainwindow.ui \
    showcal.ui \
    showcal.ui \
    showpoint.ui \
    testresultdlg.ui


#INCLUDEPATH += E:\Printable-3D-Gaze\opencv\build\include
#INCLUDEPATH += E:\Printable-3D-Gaze\opencv\build\include\opencv
#INCLUDEPATH += E:\Printable-3D-Gaze\opencv\build\include\opencv2
#CONFIG(debug, debug|release): {

#        LIBS +=E:\Printable-3D-Gaze\opencv\build\x64\vc15\lib\opencv_world349d.lib
#        }

#        else:CONFIG(release, debug|release): {

#        LIBS += E:\Printable-3D-Gaze\opencv\build\x64\vc15\lib\opencv_world349.lib

#        }

#Debug:DBG_SUFFIX = "d"

#win32 : {
#        CV_SUFFIX=410$${DBG_SUFFIX}
#        OPENCVPATH="E:\3D-Eye-Tracker-master\613\EyeAiming/deps/opencv-4.1.0"
#        LIBS += "-L$${OPENCVPATH}/x64/vc14/lib/"
#} else {
#        OPENCVPATH="E:\3D-Eye-Tracker-master\613\EyeAiming/deps/opencv-4.1.1"
#}
#INCLUDEPATH += $${OPENCVPATH}/include/
#LIBS += -lopencv_world$${CV_SUFFIX}


# 正确的Debug/Release配置
CONFIG(debug, debug|release) {
    DBG_SUFFIX = d
    CONFIG += console  # Debug模式显示控制台
    DEFINES += _DEBUG
    # Debug模式使用调试运行时
    QMAKE_CXXFLAGS_DEBUG += -MDd
} else {
    DBG_SUFFIX =
    # Release模式设置
    # 暂时不禁用assert和调试输出以便排查问题
    # DEFINES += NDEBUG
    # DEFINES += QT_NO_DEBUG_OUTPUT

    # 清除默认的优化设置，避免冲突
    QMAKE_CXXFLAGS_RELEASE -= -O2

    # 使用Release运行时库
    QMAKE_CXXFLAGS_RELEASE += -MD

    # 生成调试信息以便分析崩溃
    QMAKE_CXXFLAGS_RELEASE += -Zi
    QMAKE_LFLAGS_RELEASE += -DEBUG

    # 选择一个：运行时检查或优化（不能同时使用）

    # 方案1：使用运行时检查（有助于找到崩溃原因，但程序较慢）
    # QMAKE_CXXFLAGS_RELEASE += -RTC1

    # 方案2：使用轻度优化（程序较快，但可能难以调试）
    QMAKE_CXXFLAGS_RELEASE += -O1
}

OPENCVPATH="E:\3D-Eye-Tracker-master\613\EyeAiming/deps/opencv-3.2.0"
INCLUDEPATH += $${OPENCVPATH}/include/
INCLUDEPATH += $${OPENCVPATH}/include/opencv/
INCLUDEPATH += $${OPENCVPATH}/include/opencv2/
win32:CV_SUFFIX=320$${DBG_SUFFIX}
unix:CV_SUFFIX=$${DBG_SUFFIX}
win32:contains(QMAKE_HOST.arch, x86_64) {
    LIBS += "-L$${OPENCVPATH}/x64/vc14/lib/"
} else {
    LIBS += "-L$${OPENCVPATH}/x86/vc14/lib/"
}
LIBS += \
    -lopencv_core$${CV_SUFFIX} \
    -lopencv_core$${CV_SUFFIX} \
    -lopencv_calib3d$${CV_SUFFIX} \
    -lopencv_core$${CV_SUFFIX} \
    -lopencv_features2d$${CV_SUFFIX} \
    -lopencv_flann$${CV_SUFFIX} \
    -lopencv_highgui$${CV_SUFFIX} \
    -lopencv_imgcodecs$${CV_SUFFIX} \
    -lopencv_imgproc$${CV_SUFFIX} \
    -lopencv_videoio$${CV_SUFFIX} \
        -lopencv_video$${CV_SUFFIX} \
        -lopencv_aruco$${CV_SUFFIX}

# DirectShow libraries
LIBS += -lstrmiids -lole32 -loleaut32 -luuid -luser32



# 额外依赖库
INCLUDEPATH += E:\3D-Eye-Tracker-master\613\eigen-3.2.10\eigen-3.2.10
INCLUDEPATH += E:\3D-Eye-Tracker-master\613\tbb2018_20180822oss\include

# TBB库配置 - 暂时禁用以排查崩溃问题
# 如果程序能正常启动，说明TBB库有问题
 CONFIG(debug, debug|release) {
     LIBS += E:\3D-Eye-Tracker-master\613\tbb2018_20180822oss\lib\intel64\vc14\tbb_debug.lib
     LIBS += E:\3D-Eye-Tracker-master\613\tbb2018_20180822oss\lib\intel64\vc14\tbbmalloc_debug.lib
 } else {
     LIBS += E:\3D-Eye-Tracker-master\613\tbb2018_20180822oss\lib\intel64\vc14\tbb.lib
     LIBS += E:\3D-Eye-Tracker-master\613\tbb2018_20180822oss\lib\intel64\vc14\tbbmalloc.lib
 }




RESOURCES += \
    icon.qrc \
    professional_style.qrc
RC_FILE=icon.rc

CONFIG += qaxcontainer #导出excel

QMAKE_CFLAGS += /utf-8
QMAKE_CXXFLAGS += /utf-8


INCLUDEPATH += E:\3D-Eye-Tracker-master\613\EyeAiming\sing\


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../Build/singleeyefitter/release/ -lsingleeyefitter
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../Build/singleeyefitter/debug/ -lsingleeyefitter

INCLUDEPATH += $$PWD/../../singleeyefitter
DEPENDPATH += $$PWD/../../singleeyefitter

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../Build/singleeyefitter/release/libsingleeyefitter.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../Build/singleeyefitter/debug/libsingleeyefitter.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../Build/singleeyefitter/release/singleeyefitter.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../Build/singleeyefitter/debug/singleeyefitter.lib


#D:\local\boost_1_62_0\lib64-msvc-14.0\libboost_filesystem-vc140-mt-gd-1_62.lib
#D:\local\boost_1_62_0\lib64-msvc-14.0\libboost_system-vc140-mt-gd-1_62.lib
#D:\local\boost_1_62_0\lib64-msvc-14.0\libboost_timer-vc140-mt-gd-1_62.lib
#D:\local\boost_1_62_0\lib64-msvc-14.0\libboost_thread-vc140-mt-gd-1_62.lib
#D:\local\boost_1_62_0\lib64-msvc-14.0\libboost_date_time-vc140-mt-gd-1_62.lib
#D:\local\boost_1_62_0\lib64-msvc-14.0\libboost_chrono-vc140-mt-gd-1_62.lib
#D:\local\boost_1_62_0\lib64-msvc-14.0\libboost_regex-vc140-mt-gd-1_62.lib
#D:\local\boost_1_62_0\lib64-msvc-14.0\libboost_serialization-vc140-mt-gd-1_62.lib
#D:\local\boost_1_62_0\lib64-msvc-14.0\libboost_atomic-vc140-mt-gd-1_62.lib

#boost
INCLUDEPATH += D:\local\boost_1_62_0\


 CONFIG(debug, debug|release) {
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\ -llibboost_filesystem-vc140-mt-gd-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_system-vc140-mt-gd-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_timer-vc140-mt-gd-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_thread-vc140-mt-gd-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_date_time-vc140-mt-gd-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_chrono-vc140-mt-gd-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_regex-vc140-mt-gd-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_serialization-vc140-mt-gd-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_atomic-vc140-mt-gd-1_62
 } else {
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\ -llibboost_filesystem-vc140-mt-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_system-vc140-mt-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_timer-vc140-mt-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_thread-vc140-mt-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_date_time-vc140-mt-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_chrono-vc140-mt-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_regex-vc140-mt-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_serialization-vc140-mt-1_62
    LIBS += -LD:\local\boost_1_62_0\lib64-msvc-14.0\  -llibboost_atomic-vc140-mt-1_62
 }







#DISTFILES += \
#    sing/DirectShowInterfaces.idl \
#    sing/SConscript



INCLUDEPATH += E:\3D-Eye-Tracker-master\external\spii-3.0.0\include\
#LIBS += -LE:\3D-Eye-Tracker-master\external\spii-3.0.0\lib\ -lspii.lib
LIBS += -LE:\3D-Eye-Tracker-master\external\spii-3.0.0\lib\ -lmeschach

INCLUDEPATH += E:\3D-Eye-Tracker-master\external\Ceres-1.11\include
INCLUDEPATH += E:\3D-Eye-Tracker-master\external\Ceres-1.11\include\ceres\internal\miniglog
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../external/Ceres-1.11/lib/ -lceres
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../external/Ceres-1.11/lib/ -lceresd





win32: LIBS += -L$$PWD/../../external/spii-3.0.0/lib/ -lspii

INCLUDEPATH += $$PWD/../../external/Ceres-1.11/include
DEPENDPATH += $$PWD/../../external/Ceres-1.11/include
