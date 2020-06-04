#-------------------------------------------------
#
# Project created by QtCreator 2017-02-26T14:13:24
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = hd_demo
TEMPLATE = app

#QMAKE_CXXFLAGS += -Wall -o2 -std=c++11

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += /usr/local/include/opencv
INCLUDEPATH += /home/cubox/work2/mutlcam_mbdas/kernel-headers
INCLUDEPATH += /home/cubox/work2/mutlcam_mbdas/include
#INCLUDEPATH += /home/cubox/work2/HD_Face_kwon/include
#INCLUDEPATH += /home/cubox/work2/HD_Face_kwon/kernel-headers

LIBS += -L/usr/local/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_videoio -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_flann
LIBS += -L/home/cubox/work2/mutlcam_mbdas/lib -lnxvidrc -lm $(OPENCVLIB)

#LIBS += -L/home/cubox/work/HD_Face_kwon/lib -lion -lnxdsp -lnxv4l2 -lnxvip -lv4l2-nexell -lnxvmem -lnxvpu -lyuv2rgb_neon

LIBS += /home/cubox/work2/mutlcam_mbdas/libstatic/libion.a
LIBS += /home/cubox/work2/mutlcam_mbdas/libstatic/libv4l2-nexell.a
LIBS += /home/cubox/work2/mutlcam_mbdas/libstatic/libnxvmem.a
LIBS += /home/cubox/work2/mutlcam_mbdas/libstatic/libnxvpu.a
LIBS += /home/cubox/work2/mutlcam_mbdas/libstatic/libyuv2rgb_neon.a

SOURCES += main.cpp \
        mainwindow.cpp \
        faceeventthread.cpp \
        serialComthread.cpp \
        log_util.cpp \
	MLX90640_API.cpp \ 
	MLX90640_LINUX_I2C_Driver.cpp \
	melesixthread.cpp


HEADERS  += mainwindow.h \
         faceeventthread.h \
         yuv2rgb.neon.h \
         serialComthread.h \
         log_util.h \
	MLX90640_API.h \
	MLX90640_I2C_Driver.h \
	melesixthread.h

FORMS    += mainwindow.ui
