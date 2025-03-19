#-------------------------------------------------
#
# Project created by QtCreator 2015-02-26T21:14:11
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FontTool
TEMPLATE = app


SOURCES += main.cpp\
        maindlg.cpp \
    fontsampler.cpp \
    selectcharacters.cpp

HEADERS  += maindlg.h \
    freetype_font_sampler.h \
    fontsampler.h \
    Font.h \
    selectcharacters.h

FORMS    += maindlg.ui \
    selectcharacters.ui
