CONFIG += debug_and_release

CONFIG(debug) {
	DBGNAME = debug
}
else {
	DBGNAME = release
}

DESTDIR = bin.$${DBGNAME}

win32 {
    TEMPLATE = vcapp
    UNAME = Win32
    CONFIG(debug) {
        TRIMESH = trimeshd
    } else {
        TRIMESH = trimesh
        DPIX = dpix
        CONFIG(cuda) {
            CUDPPLIB = cudpp32
        }
    }
    
}
else {
    TEMPLATE = app
    TRIMESH = trimesh
    macx {
        DEFINES += DARWIN
        UNAME = Darwin
        CONFIG(debug, debug|release) {
            CONFIG -= app_bundle
        }
        LIBS += -framework CoreFoundation
    }
    else {
        DEFINES += LINUX
        UNAME = Linux
    }
}

QT += opengl xml
TARGET = qviewer

PRE_TARGETDEPS += ../libgq/lib.$${DBGNAME}/libgq.a
DEPENDPATH += ../libgq/include
INCLUDEPATH += ../libgq/include
LIBS += -L../libgq/lib.$${DBGNAME} -lgq

PRE_TARGETDEPS += ../trimesh2/lib.$${UNAME}/libtrimesh.a
DEPENDPATH += ../trimesh2/include
INCLUDEPATH += ../trimesh2/include
LIBS += -L../trimesh2/lib.$${UNAME} -l$${TRIMESH}

PRE_TARGETDEPS += ../qglviewer/lib.$${DBGNAME}/libqglviewer.a
DEPENDPATH += ../qglviewer
INCLUDEPATH += ../qglviewer 
LIBS += -L../qglviewer/lib.$${DBGNAME} -lqglviewer
DEFINES += QGLVIEWER_STATIC

# Input
HEADERS += src/*.h
SOURCES += src/*.cc

