#ifndef SCENE_H_
#define SCENE_H_

#include <QDomElement>
#include <QDir>

#include "GQVertexBufferSet.h"
#include "GQShaderManager.h"
#include "GQStats.h"
#include "XForm.h"

class GLViewer;
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
    bool save( const QString& filename, const GLViewer* viewer );
    bool save( QDomDocument& doc, QDomElement& root, const QDir& path );

    void recordStats(GQStats& stats);

    void drawScene(); 

    void boundingSphere(vec& center, float& radius);

    void setCameraTransform( const xform& xf ) { _camera_transform = xf; }

    const TriMesh* trimesh() const { return _trimesh; }
    const QDomElement& viewerState() { return _viewer_state; }

protected:
    void setupMesh();

    void setupLighting(GQShaderRef& shader);
    void drawMesh(GQShaderRef& shader);

protected:
    TriMesh*            _trimesh;
    GQVertexBufferSet   _vertex_buffer_set;
    QString             _trimesh_filename;

    xform               _camera_transform;
    vec                 _light_direction;

    QDomElement         _viewer_state;
};

#endif // SCENE_H_