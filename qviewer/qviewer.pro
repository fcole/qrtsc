CONFIG += debug_and_release

CONFIG(release, debug|release) {
	DBGNAME = release
}
else {
	DBGNAME = debug
}
DESTDIR = $${DBGNAME}

win32 {
    TEMPLATE = vcapp
    UNAME = Win32
}
else {
    TEMPLATE = app
    macx {
        DEFINES += DARWIN
        UNAME = Darwin
        CONFIG(debug, debug|release) {
            CONFIG -= app_bundle
        }
        LIBS += -framework CoreFoundation
        QMAKE_CXXFLAGS += -fopenmp
        QMAKE_LFLAGS += -fopenmp
    }
    else {
        DEFINES += LINUX
        UNAME = Linux
    }
}

TRIMESH = trimesh

QT += opengl xml script
TARGET = qrtsc

PRE_TARGETDEPS += ../libgq/$${DBGNAME}/libgq.a
DEPENDPATH += ../libgq/include
INCLUDEPATH += ../libgq/include
LIBS += -L../libgq/$${DBGNAME} -lgq

PRE_TARGETDEPS += ../demoutils/$${DBGNAME}/libdemoutils.a
DEPENDPATH += ../demoutils/include
INCLUDEPATH += ../demoutils/include
LIBS += -L../demoutils/$${DBGNAME} -ldemoutils

PRE_TARGETDEPS += ../trimesh2/$${DBGNAME}/libtrimesh.a
DEPENDPATH += ../trimesh2/include
INCLUDEPATH += ../trimesh2/include
LIBS += -L../trimesh2/$${DBGNAME} -l$${TRIMESH}

PRE_TARGETDEPS += ../qglviewer/$${DBGNAME}/libqglviewer.a
DEPENDPATH += ../qglviewer
INCLUDEPATH += ../qglviewer 
LIBS += -L../qglviewer/$${DBGNAME} -lqglviewer
DEFINES += QGLVIEWER_STATIC

# Input
HEADERS += src/*.h
SOURCES += src/*.cc

