QT += core gui opengl multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = video_editor
TEMPLATE = app

DEFINES += __STDC_CONSTANT_MACROS

SOURCES += main.cpp\
			  main_window.cpp \
    av_gl_widget.cpp \
    video_item.cpp \
    audio_presenter.cpp \
    avformat_context_wrapper.cpp \
    avpacket_wrapper.cpp

HEADERS += main_window.h \
    av_gl_widget.h \
    video_item.h \
    audio_presenter.h \
    avformat_context_wrapper.h \
    avpacket_wrapper.h

FORMS += main_window.ui

LIBS += -lavcodec -lavformat -lswscale
