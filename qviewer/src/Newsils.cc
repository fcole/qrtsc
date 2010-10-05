#include "Newsils.h"

#include "XForm.h"
#include "TriMesh.h"
#include "DialsAndKnobs.h"
#include "GQInclude.h"

#include <QVector>

static dkBool draw_extra_normals("Vectors->Extra Normals", false);
static dkFloat normal_interp_weight("Dual Silhouettes->Normal Interp", 0, 0, 1, 0.1);

namespace Newsils {

TriMesh* themesh;

typedef struct {
    vec f;
    vec e[3];
} ExtraNormals;
QVector<ExtraNormals> extra_normals;
QVector<ExtraNormals> extra_normals_interp;

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
    themesh->need_normals();

    extra_normals.resize(themesh->faces.size());
    extra_normals_interp.resize(themesh->faces.size());
    for (int i = 0; i < extra_normals.size(); i++) {
        const TriMesh::Face& f = themesh->faces[i];
        const vec& v0 = themesh->vertices[f[0]];
        const vec& v1 = themesh->vertices[f[1]];
        const vec& v2 = themesh->vertices[f[2]];        
        vec face_n = (v1 - v0) CROSS (v2 - v0);
        normalize(face_n);
        extra_normals[i].f = face_n;

        for (int k = 0; k < 3; k++) {
            if (themesh->across_edge[i][k] < 0) {
                extra_normals[i].e[k] = face_n;
                continue;
            }

            const TriMesh::Face& of = 
                themesh->faces[themesh->across_edge[i][k]];
            const vec& v0 = themesh->vertices[of[0]];
            const vec& v1 = themesh->vertices[of[1]];
            const vec& v2 = themesh->vertices[of[2]]; 
            vec on = (v1 - v0) CROSS (v2 - v0);
            normalize(on);
            vec edge_n = on + face_n;
            normalize(edge_n);
            extra_normals[i].e[k] = edge_n;
        }

        vec n = themesh->normals[f[0]] + themesh->normals[f[1]] + 
                themesh->normals[f[2]];
        normalize(n);
        extra_normals_interp[i].f = n;

        for (int k = 0; k < 3; k++) {
            const vec& n0 = themesh->normals[f[(k+1)%3]];
            const vec& n1 = themesh->normals[f[(k+2)%3]];

            vec en = n0 + n1;
            normalize(en);

            extra_normals_interp[i].e[k] = en;
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
    static QVector<ExtraNormals> normals;
    normals.resize(extra_normals.size());
    
    if (normal_interp_weight == 0)
        normals = extra_normals;
    else if (normal_interp_weight == 1)
        normals = extra_normals_interp;
    else {
        float w = normal_interp_weight;
        for (int i = 0; i < normals.size(); i++) {
            normals[i].f = ((1-w) * extra_normals[i].f) + (w * extra_normals_interp[i].f);
            for (int k = 0; k < 3; k++) {
                normals[i].e[k] = ((1-w) * extra_normals[i].e[k]) + (w * extra_normals_interp[i].e[k]);
            }
        }
    }

    face_scalars.resize(normals.size());
    for (int i = 0; i < normals.size(); i++) {
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
            scalars.e[k] = viewdir DOT normals[i].e[k];
        }
        vec centroid = themesh->vertices[f[0]] + themesh->vertices[f[1]] + 
                       themesh->vertices[f[2]];
        centroid *= 1 / 3.0;
        vec viewdir = viewpos - centroid;
        normalize(viewdir);
        scalars.f = viewdir DOT normals[i].f;
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
            
            // viewdir must be computed to the middle of each edge so 
            //that the dot products are consistent between faces.
            vec edge = themesh->vertices[f[(k+1)%3]] + 
                       themesh->vertices[f[(k+2)%3]];
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

void drawExtraNormals()
{
    // Normals
    int nf = themesh->faces.size();

    vec pos_color(0.2, 0.0, 0.7);
    vec neg_color(0.7, 0.0, 0.2);
    glBegin(GL_LINES);
    float line_len = 0.5f * themesh->feature_size();
    for (int i = 0; i < nf; i++) {
        const ExtraNormals& ex = extra_normals[i];
        const FaceScalars& s = face_scalars[i];
        const TriMesh::Face& f = themesh->faces[i];

        vec c;
        for (int k = 0; k < 3; k++) {
            const vec& v = themesh->vertices[f[k]];
            c = c + v;
            if (s.v[k] > 0)
                glColor3fv(pos_color);
            else
                glColor3fv(neg_color);
            glVertex3fv(v);
            glVertex3fv(v + 2.0f * line_len * s.v[k] * 
                        themesh->normals[f[k]]);
        }
        for (int k = 0; k < 3; k++) {
            const vec& va = themesh->vertices[f[(k+1)%3]];
            const vec& vb = themesh->vertices[f[(k+2)%3]];
            vec edge = va + vb;
            edge *= 0.5;
            if (s.e[k] > 0)
                glColor3fv(pos_color);
            else
                glColor3fv(neg_color);
            glVertex3fv(edge);
            glVertex3fv(edge + 2.0f * line_len * s.e[k] * ex.e[k]);
        }
        c *= 1 / 3.0;
            if (s.f > 0)
                glColor3fv(pos_color);
            else
                glColor3fv(neg_color);
        glVertex3fv(c);
        glVertex3fv(c + 2.0f * line_len * s.f * ex.f);

    }
    glEnd();
}

// Edge is defined to go between a-b and a-c.
void drawLerpEdge(const vec& a, const vec& b, const vec& c,
                  float u, float v, float w)
{
    float alpha = u / (u - v);
    float beta = u / (u - w);

    vec p1 = ((1-alpha) * a) + (alpha * b);
    vec p2 = ((1-beta) * a) + (beta * c);

    glVertex3fv(p1);
    glVertex3fv(p2);
}

void drawFaceSilhouette(const vec& a, const vec& b, const vec& c,
                        float u, float v, float w)
{
    if (u < 0.0f && v >= 0.0f && w >= 0.0f ||
	    u > 0.0f && v <= 0.0f && w <= 0.0f)
        drawLerpEdge(a,b,c,u,v,w);
	else if (v < 0.0f && w >= 0.0f && u >= 0.0f ||
		 v > 0.0f && w <= 0.0f && u <= 0.0f)
        drawLerpEdge(b,c,a,v,w,u);
	else if (w < 0.0f && u >= 0.0f && v >= 0.0f ||
		 w > 0.0f && u <= 0.0f && v <= 0.0f)
        drawLerpEdge(c,a,b,w,u,v);
}

void drawDualSilhouettes(vec viewpos)
{
    findFaceScalars(viewpos);

    int nf = extra_normals.size();

    themesh->need_across_edge();

    glBegin(GL_LINES);

    for (int i = 0; i < nf; i++) {
        const TriMesh::Face& f = themesh->faces[i];
        const FaceScalars& s = face_scalars[i];
        
        vec verts[3];
        vec edges[3];
        for (int k = 0; k < 3; k++) {
            verts[k] = themesh->vertices[f[k]];
            const vec& va = themesh->vertices[f[(k+1)%3]];
            const vec& vb = themesh->vertices[f[(k+2)%3]];
            edges[k] = va + vb;
            edges[k] *= 0.5;
        }
        vec c = verts[0] + verts[1] + verts[2];
        c *= 1 / 3.0;

        drawFaceSilhouette(verts[0],edges[2],edges[1],s.v[0],s.e[2],s.e[1]);
        drawFaceSilhouette(verts[1],edges[0],edges[2],s.v[1],s.e[0],s.e[2]);
        drawFaceSilhouette(verts[2],edges[1],edges[0],s.v[2],s.e[1],s.e[0]);
        drawFaceSilhouette(edges[0],edges[1],c,s.e[0],s.e[1],s.f);
        drawFaceSilhouette(edges[1],edges[2],c,s.e[1],s.e[2],s.f);
        drawFaceSilhouette(edges[2],edges[0],c,s.e[2],s.e[0],s.f);
    }

    glEnd();

    if (draw_extra_normals) {
        drawExtraNormals();
    }
}
    
} // namespace






       


