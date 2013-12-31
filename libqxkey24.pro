QT -= gui

TARGET = qxkey24
TEMPLATE = lib

DEFINES += LIBQXKEY24_LIBRARY

SOURCES += qxkey24.cpp

HEADERS += qxkey24.h

unix {
    LIBS += -Bstatic -lpiehid -Bdynamic

    target.path = /usr/lib
    INSTALLS += target

    header_files.files = $$HEADERS
    header_files.path = /usr/include
    INSTALLS += header_files
}
