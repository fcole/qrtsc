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

#include <QFileInfo>
#include <QStringList>

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
    _vertex_buffer_set.clear();
    _viewer_state.clear();
}


bool Scene::load( const QString& filename )
{
    if (filename.endsWith("qvs"))
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

    GQShaderRef shader = GQShaderManager::bindProgram("polygon_render");
    setupLighting(shader);

    drawMesh(shader);
}

void Scene::setupMesh()
{
    _trimesh->need_bsphere();
    _trimesh->need_normals();
    _trimesh->need_tstrips();
    _trimesh->need_faces();

    _vertex_buffer_set.clear();
    _vertex_buffer_set.add(GQ_VERTEX, _trimesh->vertices);
    _vertex_buffer_set.add(GQ_NORMAL, _trimesh->normals);
}

void Scene::drawMesh(GQShaderRef& shader)
{
    Q_UNUSED(shader);

    assert(_trimesh->tstrips.size() > 0);

    _vertex_buffer_set.bind();

    // Trimesh stores triangle strips as length followed by indices.
	const int *t = &_trimesh->tstrips[0];
	const int *end = t + _trimesh->tstrips.size();
    // "likely" is a hint to the branch predictor for certain versions of 
    // gcc.
	while (likely(t < end)) {
		int striplen = *t++;
		glDrawElements(GL_TRIANGLE_STRIP, striplen, GL_UNSIGNED_INT, t);
		t += striplen;
	}

    _vertex_buffer_set.unbind();
}

static QStringList lightPresetNames = (QStringList() << "Headlight" <<
"North" << "North-Northeast" << "Northeast" << "East-Northeast" << 
"East" << "East-Southeast" << "Southeast" << "South-Southeast" <<
"South" << "South-Southwest" << "Southwest" << "West-Southwest" << 
"West" << "West-Northwest" << "Northwest" << "North-Northwest" );
static dkStringList light_preset("Light Direction", lightPresetNames);
static dkFloat light_depth("Light Depth", 1.0f, 0.0f, 2.0f, 0.1f);

void Scene::setupLighting(GQShaderRef& shader)
{
    const QString& preset = light_preset.value();
    vec camera_light;

    if (preset == "Headlight")
        camera_light = vec(0.0f, 0.0f, 1.0f);
    else if (preset == "North")
        camera_light = vec(0.0f, 1.0f, light_depth);
    else if (preset == "North-Northeast")
        camera_light = vec(0.374f, 1.0f, light_depth);
    else if (preset == "Northeast")
        camera_light = vec(1.0f, 1.0f, light_depth);
    else if (preset == "East-Northeast")
        camera_light = vec(1.0f, 0.374f, light_depth);
    else if (preset == "East")
        camera_light = vec(1.0f, 0.0f, light_depth);
    else if (preset == "East-Southeast")
        camera_light = vec(1.0f, -0.374f, light_depth);
    else if (preset == "Southeast")
        camera_light = vec(1.0f, -1.0f, light_depth);
    else if (preset == "South-Southeast")
        camera_light = vec(0.374f, -1.0f, light_depth);
    else if (preset == "South")
        camera_light = vec(0.0f, -1.0f, light_depth);
    else if (preset == "South-Southwest")
        camera_light = vec(-0.374f, -1.0f, light_depth);
    else if (preset == "Southwest")
        camera_light = vec(-1.0f, -1.0f, light_depth);
    else if (preset == "West-Southwest")
        camera_light = vec(-1.0f, -0.374f, light_depth);
    else if (preset == "West")
        camera_light = vec(-1.0f, 0.0f, light_depth);
    else if (preset == "West-Northwest")
        camera_light = vec(-1.0f, 0.374f, light_depth);
    else if (preset == "Northwest")
        camera_light = vec(-1.0f, 1.0f, light_depth);
    else if (preset == "North-Northwest")
        camera_light = vec(-0.374f, 1.0f, light_depth);
    
    normalize(camera_light);
    xform mv_xf = rot_only( _camera_transform );
    vec world_light = mv_xf * camera_light;

    shader.setUniform3fv("light_dir_world", world_light);
}

void Scene::recordStats(GQStats& stats)
{
    stats.beginConstantGroup("Mesh");
    stats.setConstant("Num Vertices", _trimesh->vertices.size());
    stats.setConstant("Num Faces", _trimesh->faces.size());
    stats.endConstantGroup();
}