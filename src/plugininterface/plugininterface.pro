######################################################################
# Automatically generated by qmake (2.01a) Mon Jun 11 05:34:39 2007
######################################################################

TEMPLATE = lib
TARGET = plugininterface
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = .
win32:DESTDIR = ..

# Input
HEADERS += addressparser.h proxy.h tcpsocket.h filewriter.h filewriterthread.h socketexceptions.h basicsettingsmanager.h
SOURCES += addressparser.cpp proxy.cpp tcpsocket.cpp filewriter.cpp filewriterthread.cpp socketexceptions.cpp basicsettingsmanager.cpp
CONFIG	+= release threads
QT	+= network
INCLUDEPATH += ../

unix:themain.path=/usr/lib
unix:themain.files=libplugininterface.so*
win32:themain.path=../
win32:themain.files=release/plugininterface.dll release/libplugininterface.a
win32:LIBS += -L../ -lexceptions
INSTALLS += themain
