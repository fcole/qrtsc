CONFIG += debug_and_release

CONFIG(release, debug|release) {
	DBGNAME = release
}
else {
	DBGNAME = debug
}
DESTDIR = $${DBGNAME}

TEMPLATE = app

DEFINES += DARWIN
DEFINES += HAVE_VA_COPY
#QMAKE_CXXFLAGS += -fopenmp
CONFIG += x86_64
CONFIG(debug, debug|release) {
    CONFIG -= app_bundle
}
LIBS += -framework CoreFoundation
QMAKE_CXXFLAGS += -fopenmp
QMAKE_LFLAGS += -fopenmp

QT += opengl xml

TARGET = qviewer

INCLUDEPATH += trimesh2/include
INCLUDEPATH += libgq/include
DEPENDPATH += libgq/include

#Input
#libgq
HEADERS += libgq/include/GQ*.h
SOURCES += libgq/libsrc/GQ*.cc 
SOURCES += libgq/libsrc/GLee.c

# zlib
INCLUDEPATH += libgq/zlib
SOURCES += libgq/zlib/*.c
win32 {
	SOURCES += libgq/zlib/win32/*.c
}

# Matio
INCLUDEPATH += libgq/matio
SOURCES += libgq/matio/*.c

# demoutils
HEADERS += demoutils/include/*.h
SOURCES += demoutils/libsrc/*.cc

# qviewer
HEADERS += qviewer/src/*.h
SOURCES += qviewer/src/*.cc

# trimesh2
HEADERS += trimesh2/include/*.h
SOURCES += trimesh2/libsrc/*.cc

# libqglviewer has to be compiled separately because of
# conflicts with the Vec type.
PRE_TARGETDEPS += qglviewer/$${DBGNAME}/libqglviewer.a
DEPENDPATH += qglviewer
INCLUDEPATH += qglviewer 
LIBS += -Lqglviewer/$${DBGNAME} -lqglviewer
DEFINES += QGLVIEWER_STATIC



