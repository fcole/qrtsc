/*****************************************************************************\

Scene.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

qviewer is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef SCENE_H_
#define SCENE_H_

#include <QDomElement>
#include <QDir>

#include "GQVertexBufferSet.h"
#include "GQShaderManager.h"
#include "Stats.h"
#include "XForm.h"

class GLViewer;
class DialsAndKnobs;
class TriMesh;
class dkFloat;
class dkEnum;

class Scene
{
public:
    Scene();

    void clear();

	bool load( const QString& filename );
    bool load( const QDomElement& root, const QDir& path );
    bool save( const QString& filename, const GLViewer* viewer,
               const DialsAndKnobs* dials_and_knobs );
    bool save( QDomDocument& doc, QDomElement& root, const QDir& path );

    void recordStats(Stats& stats);

    void drawScene(); 

    void boundingSphere(vec& center, float& radius);

    void setCameraTransform( const xform& xf );
    void setLightDir(const vec& lightdir);

    const TriMesh* trimesh() const { return _trimesh; }
    const QDomElement& viewerState() { return _viewer_state; }
    const QDomElement& dialsAndKnobsState() { return _dials_and_knobs_state; }

    static QString fileExtension() { return QString("qrt"); }

protected:
    void setupMesh();

protected:
    TriMesh*            _trimesh;
    QString             _trimesh_filename;

    xform               _camera_transform;

    QDomElement         _viewer_state;
    QDomElement         _dials_and_knobs_state;
};

#endif // SCENE_H_
