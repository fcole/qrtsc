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

CONFIG(link_matlab) {
    macx {
        MATLAB = /Applications/MATLAB_R2009a.app/extern
    }
    DEFINES += GQ_LINK_MATLAB
    INCLUDEPATH += $${MATLAB}/include
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
