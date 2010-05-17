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

# qviewer
HEADERS += qviewer/src/*.h
SOURCES += qviewer/src/*.cc

# qviewer
HEADERS += trimesh2/include/*.h
SOURCES += trimesh2/libsrc/*.cc

# Trimesh2
#PRE_TARGETDEPS += trimesh2/lib.Darwin/libtrimesh.a
#DEPENDPATH += trimesh2/include
#LIBS += -Ltrimesh2/lib.Darwin -ltrimesh

# qglviewer
PRE_TARGETDEPS += qglviewer/$${DBGNAME}/libqglviewer.a
DEPENDPATH += qglviewer
INCLUDEPATH += qglviewer 
LIBS += -Lqglviewer/$${DBGNAME} -lqglviewer
DEFINES += QGLVIEWER_STATIC

