CONFIG += debug_and_release

CONFIG(release, debug|release) {
	DBGNAME = release
}
else {
	DBGNAME = debug
}
DESTDIR = $${DBGNAME}

win32 {
	TEMPLATE = vclib
    DEFINES += _CRT_SECURE_NO_WARNINGS
}
else {
	TEMPLATE = lib
    DEFINES += HAVE_VA_COPY

    QMAKE_CXXFLAGS += -fopenmp
	macx {
		DEFINES += DARWIN
	}
	else {
		DEFINES += LINUX
	}
}

CONFIG += staticlib
QT += opengl xml
unix {
    # strange fix for compile bug in demoutils/Console.
    QT += script
}
TARGET = gq

DEPENDPATH += include
INCLUDEPATH += include

#Input
HEADERS += include/GQ*.h
SOURCES += libsrc/GQ*.cc
SOURCES += libsrc/GLee.c

# Trimesh2
INCLUDEPATH += ../trimesh2/include

# Zlib
INCLUDEPATH += zlib
SOURCES += zlib/*.c
win32 {
	SOURCES += zlib/win32/*.c
}

# Matio
INCLUDEPATH += matio
SOURCES += matio/*.c
# Matio calls zlib functions with the z_ prefix, so turn that on here.
#DEFINES += Z_PREFIX
