#-------------------------------------------------
#
# Project created by QtCreator 2015-02-06T20:24:54
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qtSimulator
TEMPLATE = app


SOURCES += main.cpp \
    maindlg.cpp \
    fonts/roboto128.c \
    ../include/pte.c

HEADERS  += \
    maindlg.h \
    ../include/pte.h

FORMS += \
    maindlg.ui
