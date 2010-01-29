CONFIG += debug_and_release

CONFIG(debug) {
	DBGNAME = debug
}
else {
	DBGNAME = release
}

DEFINES += NO_VECTORIAL_RENDER
DEFINES += QGLVIEWER_STATIC

DESTDIR = lib.$${DBGNAME}

win32 {
	TEMPLATE = vclib
}
else {
	TEMPLATE = lib

macx {
	DEFINES += DARWIN
}
else {
	DEFINES += LINUX
}
}

CONFIG += staticlib
QT += opengl xml

TARGET = qglviewer

DEPENDPATH += .
INCLUDEPATH += .

#Input
HEADERS += *.h
SOURCES += *.cpp
FORMS += *.ui
