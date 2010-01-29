CONFIG += debug_and_release

CONFIG(debug) {
	DBGNAME = debug
}
else {
	DBGNAME = release
}

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

TARGET = gq

DEPENDPATH += include
INCLUDEPATH += include
INCLUDEPATH += ../trimesh2/include

#Input
HEADERS += include/GQ*.h
SOURCES += libsrc/GQ*.cc
SOURCES += libsrc/GLee.c
