QT += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = video_editor
TEMPLATE = app

DEFINES += __STDC_CONSTANT_MACROS

SOURCES += main.cpp\
			  main_window.cpp \
    av_gl_widget.cpp

HEADERS += main_window.h \
    av_gl_widget.h

FORMS += main_window.ui

LIBS += -lavcodec -lavformat -lswscale
