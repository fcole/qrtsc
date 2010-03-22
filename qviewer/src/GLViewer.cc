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
#include "Scene.h"
#include "DialsAndKnobs.h"

GLViewer::GLViewer(QWidget* parent) : QGLViewer( parent )
{ 
    _inited = false;
    _visible = false;
    _display_timers = false;
    _scene = NULL;

    camera()->frame()->setWheelSensitivity(-1.0);
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
        xform cam_xf = xform(camera()->frame()->matrix());
        _scene->setCameraTransform(cam_xf);
        showEntireScene();
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

    GQStats& perf = GQStats::instance();
    if (_display_timers)
    {
        perf.reset();
    }
    DialsAndKnobs::instance()->incrementFrameCounter();

    xform cam_xf = xform(camera()->frame()->matrix());
    _scene->setCameraTransform(cam_xf);

    _scene->drawScene();

    if (_display_timers)
    {
        perf.updateView();
    }

    in_draw_function = false;
}


