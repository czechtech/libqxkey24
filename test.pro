CONFIG  += debug
QT      -= gui

TEMPLATE = app
CONFIG  += console

SOURCES   += test.cpp
unix:LIBS += -lqxkey24

TARGET   = test 
