/*
Szymon Rusinkiewicz
Princeton University

TriMesh_normals.cc
Compute per-vertex normals for TriMeshes

For meshes, uses average of per-face normals, weighted according to:
  Max, N.
  "Weights for Computing Vertex Normals from Facet Normals,"
  Journal of Graphics Tools, Vol. 4, No. 2, 1999.

For raw point clouds, fits plane to k nearest neighbors.

Modified by Forrester Cole (fcole@cs.princeton.edu) to include computation
of per-vertex texture coordinates.
*/

#include <stdio.h>
#include "TriMesh.h"
#include "KDtree.h"
#include "lineqn.h"
#include <set>
using std::set;


// Helper class for finding k-nearest-neighbors: returns true iff
// a point is not in the given set of points
class NotInSet : public KDtree::CompatFunc {
private:
	const float *plist;
	const set<int> &s;
public:
	NotInSet(const float *plist_, const set<int> &s_) :
			plist(plist_), s(s_)
		{}
	virtual bool operator () (const float *p) const
	{
		int ind = p - plist; 
		return (s.find(ind) == s.end());
	}
};


// Compute per-vertex normals
void TriMesh::need_normals()
{
	// Nothing to do if we already have normals
	int nv = vertices.size();
	if (int(normals.size()) == nv)
		return;

	dprintf("Computing normals... ");
	normals.clear();
	normals.resize(nv);

	// TODO: direct handling of grids
	if (!tstrips.empty()) {
		// Compute from tstrips
		const int *t = &tstrips[0], *end = t + tstrips.size();
		while (likely(t < end)) {
			int striplen = *t - 2;
			t += 3;
			bool flip = false;
			for (int i = 0; i < striplen; i++, t++, flip = !flip) {
				const point &p0 = vertices[*(t-2)];
				const point &p1 = vertices[*(t-1)];
				const point &p2 = vertices[* t   ];
				vec a = p0-p1, b = p1-p2, c = p2-p0;
				float l2a = len2(a), l2b = len2(b), l2c = len2(c);
				vec facenormal = flip ? (b CROSS a) : (a CROSS b);
				normals[*(t-2)] += facenormal * (1.0f / (l2a * l2c));
				normals[*(t-1)] += facenormal * (1.0f / (l2b * l2a));
				normals[* t   ] += facenormal * (1.0f / (l2c * l2b));
			}
		}
	} else if (need_faces(), !faces.empty()) {
		// Compute from faces
		int nf = faces.size();
#pragma omp parallel for
		for (int i = 0; i < nf; i++) {
			const point &p0 = vertices[faces[i][0]];
			const point &p1 = vertices[faces[i][1]];
			const point &p2 = vertices[faces[i][2]];
			vec a = p0-p1, b = p1-p2, c = p2-p0;
			float l2a = len2(a), l2b = len2(b), l2c = len2(c);
			vec facenormal = a CROSS b;
			normals[faces[i][0]] += facenormal * (1.0f / (l2a * l2c));
			normals[faces[i][1]] += facenormal * (1.0f / (l2b * l2a));
			normals[faces[i][2]] += facenormal * (1.0f / (l2c * l2b));
		}
	} else {
		// Find normals of a point cloud
		const int k = 12;
		const vec ref(0, 0, 1);
		const float *v0 = &vertices[0][0];
		KDtree *kd = new KDtree(v0, nv);
#pragma omp parallel for
		for (int i = 0; i < nv; i++) {
			const float *vi = &vertices[i][0];
			set<int> s;
			s.insert(vi - v0);
			for (int j = 0; j < k; j++) {
				NotInSet ns(v0, s);
				const float *match =
					kd->closest_to_pt(vi, 0.0f, &ns);
				if (!match)
					break;
				s.insert(match - v0);
			}
			if (s.size() < 4) {
				dprintf("Warning: not enough points for vertex %d\n", i);
				normals[i] = ref;
				continue;
			}
			// Compute covariance
			float C[3][3] = { {0,0,0}, {0,0,0}, {0,0,0} };
			for (set<int>::iterator it = s.begin(); it != s.end(); it++) {
				int ind = *it / 3;
				if (ind == i)
					continue;
				vec d = vertices[ind] - vertices[i];
				for (int l = 0; l < 3; l++)
					for (int m = 0; m < 3; m++)
						C[l][m] += d[l] * d[m];
			}
			float e[3];
			eigdc<float,3>(C, e);
			normals[i] = vec(C[0][0], C[1][0], C[2][0]);
			if ((normals[i] DOT ref) < 0.0f)
				normals[i] = -normals[i];
		}
		delete kd;
	}

	// Make them all unit-length
#pragma omp parallel for
	for (int i = 0; i < nv; i++)
		normalize(normals[i]);

	dprintf("Done.\n");
}

void TriMesh::need_uv_dirs()
{
    if (texcoords.empty() || 
        (need_faces(), faces.empty()) ||
        (need_normals(), normals.empty()))
        return;

    int nv = vertices.size();
	udirs.clear();
	vdirs.clear();
	udirs.resize(nv);
	vdirs.resize(nv);
	
    // Compute from faces
    int nf = faces.size();
#pragma omp parallel for
    for (int i = 0; i < nf; i++) {
        const point &p0 = vertices[faces[i][0]];
        const point &p1 = vertices[faces[i][1]];
        const point &p2 = vertices[faces[i][2]];

        const vec2 &uv0 = texcoords[faces[i][0]];
        const vec2 &uv1 = texcoords[faces[i][1]];
        const vec2 &uv2 = texcoords[faces[i][2]];

        vec a = p0-p1, b = p1-p2, c = p2-p0;
        float l2a = len2(a), l2b = len2(b), l2c = len2(c);

        // Get a local frame for the face and compute the gradient
        // of u and v in that frame. 
        vec s = -a;
        float b_s = sqrt(l2a);
        s /= b_s;
        vec t = c - s * (c DOT s);
        normalize(t);
        float c_s = c DOT s;
        float c_t = c DOT t;

        float fgs_u = (uv1[0] - uv0[0]) / b_s;
        float fgt_u = ((uv2[0] - uv0[0]) - fgs_u * c_s) / c_t;
        vec fg_u = s * fgs_u + t * fgt_u;

        float fgs_v = (uv1[1] - uv0[1]) / b_s;
        float fgt_v = ((uv2[1] - uv0[1]) - fgs_v * c_s) / c_t;
        vec fg_v = s * fgs_v + t * fgt_v;

        udirs[faces[i][0]] += fg_u * (1.0f / (l2a * l2c));
        udirs[faces[i][1]] += fg_u * (1.0f / (l2b * l2a));
        udirs[faces[i][2]] += fg_u * (1.0f / (l2c * l2b));
        vdirs[faces[i][0]] += fg_v * (1.0f / (l2a * l2c));
        vdirs[faces[i][1]] += fg_v * (1.0f / (l2b * l2a));
        vdirs[faces[i][2]] += fg_v * (1.0f / (l2c * l2b));
    }

    // Project and renormalize so normal, u, and v dirs are
    // all orthogonal.

#pragma omp parallel for
	for (int i = 0; i < nv; i++) {
        udirs[i] -= normals[i] * (udirs[i] DOT normals[i]);
        normalize(udirs[i]);
		/*vdirs[i] -= normals[i] * (vdirs[i] DOT normals[i]);
        normalize(vdirs[i]);*/
        vec v = normals[i] CROSS udirs[i];
        if ((v DOT vdirs[i]) < 0)
            v *= -1;
        vdirs[i] = v;
    }
}

