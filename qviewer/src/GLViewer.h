/*****************************************************************************\

GLViewer.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

OpenGL widget for drawing. Handles camera movement and mouse actions.
Inherits the QGLViewer class by Gilles Debunne.

dpix is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef GLVIEWER_H_
#define GLVIEWER_H_

#include "GQInclude.h"
#include "GQFramebufferObject.h"

#include <qglviewer.h>

class Scene;

class GLViewer : public QGLViewer
{
    Q_OBJECT

public:
    GLViewer( QWidget* parent = 0 );

    void setScene( Scene* npr_scene );
    void finishInit();
    void resetView();

    void saveScreenshot(QString filename);

    void setDisplayTimers(bool display) { _display_timers = display; }
    
public slots:
    void on_actionCamera_Perspective_toggled(bool checked);
    /*virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);*/
    
protected:
    virtual void draw();
    virtual void postDraw();
    virtual void resizeGL( int width, int height );

    void drawDualViewport();
    
private:
    bool   _inited;
    bool   _visible;
    bool   _display_timers;
    bool   _save_hdr_screen;
    QString _hdr_screen_filename;
    GQFramebufferObject _hdr_fbo;

    Scene* _scene;
    qglviewer::ManipulatedCameraFrame* _main_camera_frame;
    qglviewer::ManipulatedCameraFrame* _off_camera_frame;
};

#endif /*GLVIEWER_H_*/
