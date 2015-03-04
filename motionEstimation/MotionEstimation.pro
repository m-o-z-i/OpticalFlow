TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11 -fPIC -g -fexpensive-optimizations -D_GNULINUX -O3
SOURCES += \
    MotionEstimation.cpp \
    line/MyLine.cpp

HEADERS += \
    line/MyLine.h

unix:!macx: LIBS += -lopencv_core \
                    -lopencv_imgproc \
                    -lopencv_highgui \
                    -lopencv_calib3d \
                    -lopencv_contrib \
                    -lopencv_features2d \
                    -lopencv_objdetect \
                    -lopencv_video

