/*****************************************************************************\

GLViewer.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

dpix is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "GLViewer.h"
#include <XForm.h>
#include <assert.h>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDir>
#include <QDebug>
#include <QMouseEvent>
#include "Scene.h"
#include "DialsAndKnobs.h"
#include "Stats.h"

static dkBool dual_viewport("Camera->Dual Viewport", false, DK_MENU);
static dkBool camera_perspective("Camera->Perspective", true, DK_MENU);

GLViewer::GLViewer(QWidget* parent) : QGLViewer( parent )
{ 
    _inited = false;
    _visible = false;
    _display_timers = false;
    _scene = NULL;
    _save_hdr_screen = false;

    camera()->frame()->setWheelSensitivity(-1.0);

    _main_camera_frame = camera()->frame();
    _off_camera_frame = new qglviewer::ManipulatedCameraFrame;
    
    connect(&camera_perspective, SIGNAL(valueChanged(bool)),
            this, SLOT(on_actionCamera_Perspective_toggled(bool)));
    connect(_off_camera_frame, SIGNAL(manipulated()), this, SLOT(updateGL()));
    connect(_off_camera_frame, SIGNAL(spun()), this, SLOT(updateGL()));
    
    connect(&dual_viewport, SIGNAL(valueChanged(bool)), this, SLOT(updateGL()));
    connect(&camera_perspective, SIGNAL(valueChanged(bool)), this, SLOT(updateGL()));
}

void GLViewer::resetView()
{
    vec center;
    float radius;

    if (_scene)
    {
        _scene->boundingSphere(center, radius);

        setSceneRadius( radius );
        setSceneCenter( qglviewer::Vec( center[0], center[1], center[2] ) );
        camera()->setFieldOfView(3.1415926f / 6.0f);
        camera()->setZNearCoefficient(0.01f);
        camera()->setOrientation(0,0);
        showEntireScene();
        xform cam_xf = xform(_main_camera_frame->matrix());
        _scene->setCameraTransform(cam_xf);
        _off_camera_frame->setPosition(_main_camera_frame->position());
        _off_camera_frame->setOrientation(_main_camera_frame->orientation());
    }
}

void GLViewer::setScene( Scene* scene )
{ 
    bool was_inited = _inited;

    // Temporarily clear initialized flag to stop drawing
    // during initFromDOMElement.
    _inited = false;
    _scene = scene;

    if (!_scene->viewerState().isNull())
        initFromDOMElement(_scene->viewerState());
    else
        resetView();

    _inited = was_inited;
}

void GLViewer::finishInit()
{
    resetView();

    _inited = true;
}

void GLViewer::resizeGL( int width, int height )
{
    if (width < 0 || height < 0) {
        _visible = false;
        return;
    }

    _visible = (width * height != 0);
    
    QGLViewer::resizeGL( width, height);
}

static bool in_draw_function = false;

void GLViewer::draw()
{  
    if (in_draw_function)
        return; // recursive draw

    if (!(_visible && _inited && _scene))
        return;

    if (GQShaderManager::status() == GQ_SHADERS_NOT_LOADED)
        GQShaderManager::initialize();
    
    in_draw_function = true;

    Stats& perf = Stats::instance();
    if (_display_timers)
    {
        perf.reset();
    }
    DialsAndKnobs::incrementFrameCounter();
    
    if (_save_hdr_screen) {
        _hdr_fbo.initFullScreen(1,GQ_ATTACH_DEPTH);
        _hdr_fbo.bind();
    }

    xform main_cam_xf = inv(xform(_main_camera_frame->matrix()));
    xform off_cam_xf = inv(xform(_off_camera_frame->matrix()));
    
    if (dual_viewport) {
        qglviewer::ManipulatedCameraFrame* cur_frame = camera()->frame();
        camera()->setFrame(_off_camera_frame);
        camera()->loadProjectionMatrix();
        camera()->loadModelViewMatrix();
        camera()->setFrame(cur_frame);
        /*glMatrixMode(GL_MODELVIEW);
        glLoadMatrixd(off_cam_xf);*/
    } else {
        _off_camera_frame->setPosition(_main_camera_frame->position());
        _off_camera_frame->setOrientation(_main_camera_frame->orientation());
    }
    
    _scene->setCameraTransform(main_cam_xf);
    _scene->drawScene();
    
    if (_display_timers)
    {
        perf.updateView();
    }

    in_draw_function = false;
}

void GLViewer::postDraw()
{
    QGLViewer::postDraw();
    if (dual_viewport) {
        drawDualViewport();
    }
    if (_save_hdr_screen && _hdr_fbo.isBound()) {
        _hdr_fbo.unbind();
        _hdr_fbo.saveColorTextureToFile(0, _hdr_screen_filename);
        _save_hdr_screen = false;
    }
}

void GLViewer::drawDualViewport()
{
    glPushAttrib(GL_VIEWPORT_BIT | GL_SCISSOR_BIT);
    glEnable(GL_SCISSOR_TEST);
    
    const int border = 3;
    const int offset = 15;
    
    int box_width = width() / 4;
    int box_height = height() / 4;
    
    int box_right = width() - offset;
    int box_top = height() - offset;
    int box_left = box_right - box_width;
    int box_bottom = box_top - box_height;
    
    // This guy needs to be flipped in Y to match Qt's event coordinates.
    _dual_viewport_rect = QRect(box_left,height()-box_bottom-box_height,box_width,box_height);

    // Clear to make a border for the dual box. 
    glScissor(box_left-border,box_bottom-border,box_width+2*border,box_height+2*border);
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glViewport(box_left,box_bottom,box_width,box_height);
    glScissor(box_left,box_bottom,box_width,box_height);
    
    xform cam_xf = inv(xform(_main_camera_frame->matrix()));
    
    qglviewer::ManipulatedCameraFrame* cur_frame = camera()->frame();
    camera()->setFrame(_main_camera_frame);
    camera()->loadProjectionMatrix();
    camera()->loadModelViewMatrix();
    camera()->setFrame(cur_frame);
    
//    glMatrixMode(GL_MODELVIEW);
//    glLoadMatrixd(cam_xf);
    
    _scene->setCameraTransform(cam_xf);
    _scene->drawScene();

    glPopAttrib();
}

void GLViewer::saveScreenshot(QString filename) 
{
    if (filename.endsWith("pfm")) {
        _hdr_screen_filename = filename;
        _save_hdr_screen = true;
        updateGL(); 
    } else {
        saveSnapshot(filename, true);
    }
}

void GLViewer::on_actionCamera_Perspective_toggled(bool checked)
{
    if (checked) {
        camera()->setType(qglviewer::Camera::PERSPECTIVE);
    } else {
        camera()->setType(qglviewer::Camera::ORTHOGRAPHIC);
    }
    updateGL();
}

QPoint GLViewer::convertDualViewportCoords(const QPoint& p)
{
    return (p - _dual_viewport_rect.topLeft()) *
    ((float)width() / (float)_dual_viewport_rect.width());
}

void GLViewer::mousePressEvent(QMouseEvent* event)
{
    if (dual_viewport) {
        if (_dual_viewport_rect.contains(event->pos())) {
            QMouseEvent new_e = QMouseEvent(event->type(), 
                 convertDualViewportCoords(event->pos()), event->button(), 
                 event->buttons(), event->modifiers());
            camera()->setFrame(_main_camera_frame);
            QGLViewer::mousePressEvent(&new_e);
        } else {
            camera()->setFrame(_off_camera_frame);
            QGLViewer::mousePressEvent(event);
        }
    } else {
        camera()->setFrame(_main_camera_frame);
        QGLViewer::mousePressEvent(event);
    }
}
    
void GLViewer::mouseMoveEvent(QMouseEvent* event)
{
    if (dual_viewport) {
        if (camera()->frame() == _main_camera_frame) {
            QMouseEvent new_e = QMouseEvent(event->type(), 
                convertDualViewportCoords(event->pos()), event->button(), 
                event->buttons(), event->modifiers());
            QGLViewer::mouseMoveEvent(&new_e);
            return;
        }
    }
    
    QGLViewer::mouseMoveEvent(event);
}

void GLViewer::mouseReleaseEvent(QMouseEvent* event)
{
    if (dual_viewport) {
        if (camera()->frame() == _main_camera_frame) {
            QMouseEvent new_e = QMouseEvent(event->type(), 
                                            convertDualViewportCoords(event->pos()), event->button(), 
                                            event->buttons(), event->modifiers());
            QGLViewer::mouseReleaseEvent(&new_e);
            return;
        }
    }
    
    QGLViewer::mouseReleaseEvent(event);
}

void GLViewer::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space) {
        if (dual_viewport) {
            _off_camera_frame->setPosition(_main_camera_frame->position());
            _off_camera_frame->setOrientation(_main_camera_frame->orientation());
        } else {
            resetView();
        }
        updateGL();
    } else {
        QGLViewer::keyPressEvent(event);
    }
}


