#-------------------------------------------------
#
# Project created by QtCreator 2013-06-16T12:27:54
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sddm
TEMPLATE = app

INCLUDEPATH += . include

SOURCES += src/main.cpp\
    src/mainwindow.cpp \
    src/sddm.cpp \
    src/audio_driver.cpp \
    src/midi.cpp \
    src/config.cpp \
    src/model.cpp \
    src/myxml.cpp \
    src/scene.cpp \
    src/log.cpp \
    src/Sample.cpp \
    src/alsamidi.cpp \
    src/nsmclient.cpp \
    src/app.cpp

HEADERS  += include/mainwindow.h \
    include/sddm.h \
    include/audio_driver.h \
    include/midi.h \
    include/config.h \
    include/model.h \
    include/scene.h \
    include/alsamidi.h \
    include/streamer.h \
    include/myxml.h \
    include/log.h \
    include/nsmclient.h \
    include/nonlib_nsm.h \
    include/app.h

FORMS    += mainwindow.ui

LIBS += -ljack -lsndfile -lasound -lsamplerate -llo

target.path = /usr/local/bin/
INSTALLS += target
