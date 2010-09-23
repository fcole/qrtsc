/*****************************************************************************\

Scene.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

dpix is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "Scene.h"
#include "GLViewer.h"
#include "TriMesh.h"
#include "DialsAndKnobs.h"
#include "Rtsc.h"

#include <QFileInfo>
#include <QStringList>

#include "GQDraw.h"
using namespace GQDraw;

#include <assert.h>

const int CURRENT_VERSION = 1;

Scene::Scene()
{
    _trimesh = NULL;
}

void Scene::clear()
{
    delete _trimesh;
    _trimesh = NULL;
    _viewer_state.clear();
    _dials_and_knobs_state.clear();
}


bool Scene::load( const QString& filename )
{
    if (filename.endsWith(fileExtension()))
    {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning("Could not open %s", qPrintable(filename));
            return false;
        }

        QDomDocument doc("scene");
        QString parse_errors;
        if (!doc.setContent(&file, &parse_errors))
        {
            qWarning("Parse errors: %s", qPrintable(parse_errors));
            return false;
        }

        file.close();

        QDomElement root = doc.documentElement();
        QDir path = QFileInfo(filename).absoluteDir();

        return load(root, path); 
    }
    else
    {
        _trimesh = TriMesh::read(qPrintable(filename));
        _trimesh_filename = filename;
        if (!_trimesh)
        {
            clear();
            return false;
        }
        _viewer_state.clear();
        _dials_and_knobs_state.clear();
		setupMesh();

        return true;
    }
}

bool Scene::load( const QDomElement& root, const QDir& path )
{
    int version = root.attribute("version").toInt();
    if (version != CURRENT_VERSION)
    {
        qWarning("Scene::load: file version out of date (%d, current is %d)", 
            version, CURRENT_VERSION);
        return false;
    }

	QDomElement model = root.firstChildElement("model");
    if (model.isNull())
    {
        qWarning("Scene::load: no model node found.\n");
        return false;
    }

    QString relative_filename = model.attribute("filename");
    QString abs_filename = path.absoluteFilePath(relative_filename);

    _trimesh = TriMesh::read(qPrintable(abs_filename));
    _trimesh_filename = abs_filename;
    if (!_trimesh)
    {
        qWarning("Scene::load: could not load %s\n", 
            qPrintable(abs_filename));
        return false;
    }

    _viewer_state = root.firstChildElement("viewerstate");
    if (_viewer_state.isNull())
    {
        qWarning("Scene::load: no viewerstate node found.\n");
        clear();
        return false;
    }

    _dials_and_knobs_state = root.firstChildElement("dials_and_knobs");
    if (_dials_and_knobs_state.isNull())
    {
        qWarning("Scene::load: no dials_and_knobs node found.\n");
        return false;
    }
	setupMesh();

    return true;
}

bool Scene::save(const QString& filename, const GLViewer* viewer,
                 const DialsAndKnobs* dials_and_knobs)
{
    QDomDocument doc("scene");
    QDomElement root = doc.createElement("scene");
    doc.appendChild(root);

    _viewer_state = viewer->domElement("viewerstate", doc);
    _dials_and_knobs_state = 
        dials_and_knobs->domElement("dials_and_knobs", doc);

    QDir path = QFileInfo(filename).absoluteDir();

    bool ret = save(doc, root, path);
    if (!ret)
        return false;

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning("Scene::save - Could not save %s", qPrintable(filename));
        return false;
    }

    file.write(doc.toByteArray());

    file.close();

    return true;
}

bool Scene::save( QDomDocument& doc, QDomElement& root, const QDir& path )
{
    root.setAttribute("version", CURRENT_VERSION);

	QDomElement model = doc.createElement("model");
    model.setAttribute("filename", path.relativeFilePath(_trimesh_filename));
    root.appendChild(model);

    root.appendChild(_viewer_state);
    root.appendChild(_dials_and_knobs_state);

    return true;
}

void Scene::boundingSphere(vec& center, float& radius)
{
    center = _trimesh->bsphere.center;
    radius = _trimesh->bsphere.r;
}

void Scene::drawScene()
{
    if (GQShaderManager::status() != GQ_SHADERS_OK)
        return;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixd(_camera_transform);
    
    Rtsc::redraw();
    
    glPopMatrix();
}

void Scene::setupMesh()
{
    Rtsc::initialize(_trimesh);
}

void Scene::setCameraTransform( const xform& xf )
{
    _camera_transform = xf;
    Rtsc::setCameraTransform(xf);
}

void Scene::recordStats(Stats& stats)
{
    stats.beginConstantGroup("Mesh");
    stats.setConstant("Num Vertices", _trimesh->vertices.size());
    stats.setConstant("Num Faces", _trimesh->faces.size());
    stats.endConstantGroup();
}
