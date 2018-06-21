TEMPLATE = app

QT += qml quick gui opengl

CONFIG += c++11
DEFINES += QT_OPENGL_ES_2
    #VIDEOVIV_DEBUG \

INCLUDEPATH += \
    $$PWD/VideoRenderPAL \
    $$PWD/VideoRenderAVB \
    /opt/rootfs-TH/usr/include/    \
    /opt/rootfs-TH/usr/include/GLES2/    \
    /opt/rootfs-TH/usr/src/linux/include/    \
    /opt/rootfs-TH/usr/src/kernel/include/
DEPENDPATH += \
    $$PWD/VideoRenderPAL \
    $$PWD/VideoRenderAVB \

SOURCES += main.cpp \
    VideoManager.cpp \
    VideoRenderPAL/VideoRenderPAL.cpp \
    VideoRenderAVB/VideoRenderAVB.cpp \
    VideoRenderPAL/v4l2_capture.cpp \
    VideoRenderPAL/datathread.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    VideoManager.h \
    VideoRenderPAL/VideoRenderPAL.h \
    VideoRenderAVB/VideoRenderAVB.h \
    VideoRenderPAL/glext_header.h \
    VideoRenderPAL/linux_header.h \
    VideoRenderPAL/v4l2_capture.h \
    VideoRenderPAL/video_defines.h \
    VideoRenderPAL/datathread.h

LIBS+=/opt/rootfs-TH/usr/lib/libGLESv2.so
