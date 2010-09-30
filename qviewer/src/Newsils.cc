#include "Newsils.h"

#include "XForm.h"
#include "TriMesh.h"
#include "DialsAndKnobs.h"
#include "GQInclude.h"

#include <QVector>

namespace Newsils {

TriMesh* themesh;

typedef struct {
    vec f;
    vec e[3];
} ExtraNormals;
QVector<ExtraNormals> extra_normals;

typedef struct {
    float v[3], e[3], f;
} FaceScalars;
QVector<FaceScalars> face_scalars;
    
typedef struct {
    vec p;
    int parent_face;
    int parent_vert;
} SilVert;
typedef struct {
    int a, b;
} SilEdge;
QVector<SilVert> sil_verts;
QVector<SilEdge> sil_edges;

QHash<int, int> verts_to_sil_edges;

void makeExtraNormals()
{
    themesh->need_across_edge();

    extra_normals.resize(themesh->faces.size());
    for (int i = 0; i < extra_normals.size(); i++) {
        const TriMesh::Face& f = themesh->faces[i];
        const vec& v0 = themesh->vertices[f[0]];
        const vec& v1 = themesh->vertices[f[1]];
        const vec& v2 = themesh->vertices[f[2]];        
        vec face_n = (v1 - v0) CROSS (v2 - v0);
        normalize(face_n);
        extra_normals[i].f = face_n;

        for (int k = 0; k < 3; k++) {
            const TriMesh::Face& of = themesh->faces[themesh->across_edge[i][k]];
            const vec& v0 = themesh->vertices[of[0]];
            const vec& v1 = themesh->vertices[of[1]];
            const vec& v2 = themesh->vertices[of[2]]; 
            vec on = (v1 - v0) CROSS (v2 - v0);
            normalize(on);
            vec edge_n = on + face_n;
            normalize(edge_n);
            extra_normals[i].e[k] = edge_n;
        }
    }
}

void initialize(TriMesh* mesh)
{
    themesh = mesh;
    makeExtraNormals();
}

void findFaceScalars(vec viewpos)
{
    face_scalars.resize(extra_normals.size());
    for (int i = 0; i < extra_normals.size(); i++) {
        TriMesh::Face& f = themesh->faces[i];
        FaceScalars& scalars = face_scalars[i]; 
        for (int k = 0; k < 3; k++) {
            vec viewdir = viewpos - themesh->vertices[f[k]];
            normalize(viewdir);
            scalars.v[k] = viewdir DOT themesh->normals[f[k]];
        }
        for (int k = 0; k < 3; k++) {
            const vec& va = themesh->vertices[f[(k+1)%3]];
            const vec& vb = themesh->vertices[f[(k+2)%3]];
            vec edgepos = va + vb;
            edgepos *= 0.5;
            vec viewdir = viewpos - edgepos;
            normalize(viewdir);
            scalars.e[k] = viewdir DOT extra_normals[i].e[k];
        }
        vec centroid = themesh->vertices[f[0]] + themesh->vertices[f[1]] + 
                       themesh->vertices[f[2]];
        centroid *= 1 / 3.0;
        vec viewdir = viewpos - centroid;
        normalize(viewdir);
        scalars.f = viewdir DOT centroid;
    }
}

bool hasZeroCrossing(const FaceScalars& s)
{
    bool all_positive = s.f > 0;
    bool all_negative = s.f < 0;
    for (int i = 0; i < 3; i++) {
        all_positive = all_positive && (s.v[i] > 0) && (s.e[i] > 0);
        all_negative = all_negative && (s.v[i] < 0) && (s.e[i] < 0);
    }
    return !(all_positive || all_negative);
}

void findIsolines(vec viewpos, xform proj)
{
    findFaceScalars(viewpos);
    verts_to_sil_edges.clear();

    int nf = face_scalars.size();

    for (int i = 0; i < nf; i++) {
        if (!hasZeroCrossing(face_scalars[i]))
            continue;

        const FaceScalars& fs = face_scalars[i];
    }
}

void drawIsolines()
{

}

void drawEdgeSilhouettes(vec viewpos)
{
    int nf = extra_normals.size();

    themesh->need_across_edge();

    glBegin(GL_LINES);

    for (int i = 0; i < nf; i++) {
        const TriMesh::Face& f = themesh->faces[i];
        
        for (int k = 0; k < 3; k++) {
            int across_i = themesh->across_edge[i][k];
            if (across_i < i) 
                continue;
            
            // viewdir must be computed to the middle of each edge so that the dot products
            // are consistent between faces.
            vec edge = themesh->vertices[f[(k+1)%3]] + themesh->vertices[f[(k+2)%3]];
            edge *= 0.5;
            vec viewdir = viewpos - edge;
            
            float dot_f = viewdir DOT extra_normals[i].f;
            float dot_g = viewdir DOT extra_normals[across_i].f;

            if ((dot_f > 0 && dot_g < 0) || (dot_f < 0 && dot_g > 0)) {
                glVertex3fv(themesh->vertices[f[(k+1)%3]]);
                glVertex3fv(themesh->vertices[f[(k+2)%3]]);
            }
        }
    }

    glEnd();
}
    
} // namespace






       


