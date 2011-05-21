/*
Rtsc.cc

A port of real-time suggestive contours to Qt.

Authors:
  Szymon Rusinkiewicz, Princeton University
  Doug DeCarlo, Rutgers University

With contributions by:
  Xiaofeng Mi, Rutgers University
  Tilke Judd, MIT

Port modifications by:
  Forrester Cole, MIT

*/


#ifdef WIN32
#define _USE_MATH_DEFINES
#include <cmath>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "TriMesh.h"
#include "TriMesh_algo.h"
#include "XForm.h"
#include "apparentridge.h"
#include "timestamp.h"
#include <algorithm>
#include "DialsAndKnobs.h"
#include "GQInclude.h"
#include "GQShaderManager.h"

using namespace std;

namespace Rtsc {

TriMesh* themesh;

// Toggles for drawing various lines
static dkBool draw_extsil("Lines->Silhouette", false);
static dkBool draw_c("Lines->Occluding Contours", true);
static dkBool draw_sc("Lines->Suggestive Contours", true);
static dkBool draw_sh("Lines->Suggestive Highlights", false);
static dkBool draw_phridges("Lines->Principal Hlt. (R)", false);
static dkBool draw_phvalleys("Lines->Principal Hlt. (V)", false);
static dkBool draw_ridges("Lines->Ridges", false);
static dkBool draw_valleys("Lines->Valleys", false);
static dkBool draw_apparent("Lines->Apparent Ridges", false);
static dkBool draw_K("Lines->K", false);
static dkBool draw_H("Lines->H", false);
static dkBool draw_DwKr("Lines->DwKr", false);
static dkBool draw_bdy("Lines->Boundary", true);
static dkBool draw_isoph("Lines->Isophotes", false);
static dkBool draw_topo("Lines->Topo Lines", false);
static dkInt niso("Lines-># Isophotes", 20);
static dkInt ntopo("Lines-># Topo Lines", 20);
static dkFloat topo_offset("Lines->Topo Offset", 0.0);
static dkBool enable_lines("Lines->Enable Lines", true);

// Toggles for tests we perform
static dkBool draw_hidden("Tests->Draw Hidden Lines", false);
static dkBool test_c("Tests->Trim \"inside\" contours", false);
static dkBool test_sc("Tests->Trim SC", true);
static dkBool test_sh("Tests->Trim SH", true);
static dkBool test_ph("Tests->Trim PH", true);
static dkBool test_rv("Tests->Trim RV", true);
static dkBool test_ar("Tests->Trim AR", true);
static dkFloat sug_thresh("Tests->SC Thresh", 0.01, 0.0, 1, 0.01);
static dkFloat sh_thresh("Tests->SH Thresh", 0.02, 0.0, 1, 0.01);
static dkFloat ph_thresh("Tests->PH Thresh", 0.04, 0.0, 1, 0.01);
static dkFloat rv_thresh("Tests->RV Thresh", 0.1, 0.0, 1, 0.01);
static dkFloat ar_thresh("Tests->AR Thresh", 0.1, 0.0, 1, 0.01);

// Toggles for style
static dkBool use_texture("Style->Use Texture", false);
static dkBool draw_faded("Style->Draw Faded", true);
static dkBool draw_colors("Style->Draw Colors", false);
static dkBool use_hermite("Style->Use Hermite", false);
static dkBool single_pixel_lines("Style->Single Pixel Wide", false);
static dkBool draw_edges("Style->Draw Edges", false);
    
// Mesh colorization
vector<Color> curv_colors, gcurv_colors;
static QStringList mesh_color_types = QStringList() << "White" << "Gray" 
    << "Black" << "Curvature" << "Gaussian C." << "Mesh" << "Depth"
    << "Normals";
static dkStringList color_style("Style->Mesh Color", mesh_color_types);

// Lighting
static QStringList lighting_types = QStringList() << "None" << "Lambertian" 
    << "Lambertian2" << "Hemisphere" << "Toon" << "Toon BW" << "Gooch";    
static dkStringList lighting_style("Style->Lighting", lighting_types);
vec light_direction;
    
// Background color
static QStringList background_types = QStringList() << "White" << "Black" 
    << "Gray";
static dkStringList background_style("Style->Background", background_types);
    
    
// Per-vertex vectors
static dkBool draw_norm("Vectors->Normals", false);
static dkBool draw_curv1("Vectors->Principal Curv. 1", false);
static dkBool draw_curv2("Vectors->Principal Curv. 2", false);
static dkBool draw_asymp("Vectors->Asymptotic", false);
static dkBool draw_w("Vectors->W", false);
static dkBool draw_wperp("Vectors->W Perp", false);

// Other miscellaneous variables
float feature_size;	// Used to make thresholds dimensionless
float currsmooth;	// Used in smoothing
vec currcolor;		// Current line color
xform xf;           // Local copy of the viewing transform
point viewpos;
    
// Per-vertex computed values at each frame
vector<float> ndotv, kr;
vector<float> sctest_num, sctest_den, shtest_num;
vector<float> q1, Dt1q1;
vector<vec2> t1;

// Draw triangle strips.  They are stored as length followed by values.
void draw_tstrips()
{
	const int *t = &themesh->tstrips[0];
	const int *end = t + themesh->tstrips.size();
	while (likely(t < end)) {
		int striplen = *t++;
		glDrawElements(GL_TRIANGLE_STRIP, striplen, GL_UNSIGNED_INT, t);
		t += striplen;
	}
}

// Set the line width. Sometimes we just want single pixel lines.
void set_line_width(float amount)
{
    if (single_pixel_lines)
        glLineWidth(1.0);
    else
        glLineWidth(amount);
}

// Create a texture with a black line of the given width.
void make_texture(float width)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	int texsize = 1024;
	static unsigned char *texture = new unsigned char[texsize*texsize];
	int miplevel = 0;
	while (texsize) {
		for (int i = 0; i < texsize*texsize; i++) {
			float x = (float) (i%texsize) - 0.5f * texsize + 0.5f;
			float y = (float) (i/texsize) - 0.5f * texsize + 0.5f;
			float val = 1;
			if (texsize >= 4)
				if (fabs(x) < width && y > 0.0f)
					val = sqr(max(1.0f - y, 0.0f));
			texture[i] = min(max(int(256.0f * val), 0), 255);
		}
		glTexImage2D(GL_TEXTURE_2D, miplevel, GL_LUMINANCE,
			     texsize, texsize, 0,
			     GL_LUMINANCE, GL_UNSIGNED_BYTE, texture);
		texsize >>= 1;
		miplevel++;
	}

	float bgcolor[] = { 1, 1, 1, 1 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bgcolor);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
#ifdef GL_EXT_texture_filter_anisotropic
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
#endif
}


// Draw contours and suggestive contours using texture mapping
void draw_c_sc_texture(const vector<float> &ndotv,
		       const vector<float> &kr,
		       const vector<float> &sctest_num,
		       const vector<float> &sctest_den)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &themesh->vertices[0][0]);

	static vector<float> texcoords;
	int nv = themesh->vertices.size();
	texcoords.resize(2*nv);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, &texcoords[0]);

	// Remap texture coordinates from [-1..1] to [0..1]
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(0.5, 0.5, 0.0);
	glScalef(0.5, 0.5, 0.0);
	glMatrixMode(GL_MODELVIEW);

	float bgcolor[] = { 1, 1, 1, 1 };
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, bgcolor);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_COLOR, GL_ZERO); // Multiplies texture into FB
	glEnable(GL_TEXTURE_2D);
	glDepthFunc(GL_LEQUAL);


	// First drawing pass for contours
	if (draw_c) {
		// Set up the texture for the contour pass
		static GLuint texcontext_c = 0;
		if (!texcontext_c) {
			glGenTextures(1, &texcontext_c);
			glBindTexture(GL_TEXTURE_2D, texcontext_c);
			make_texture(4.0);
		}
		glBindTexture(GL_TEXTURE_2D, texcontext_c);
		if (draw_colors)
			glColor3f(0.0, 0.6, 0.0);
		else
			glColor3f(0.05, 0.05, 0.05);

		// Compute texture coordinates and draw
		for (int i = 0; i < nv; i++) {
			texcoords[2*i] = ndotv[i];
			texcoords[2*i+1] = 0.5f;
		}
		draw_tstrips();
	}

	// Second drawing pass for suggestive contours.  This should eventually
	// be folded into the previous one with multitexturing.
	if (draw_sc) {
		static GLuint texcontext_sc = 0;
		if (!texcontext_sc) {
			glGenTextures(1, &texcontext_sc);
			glBindTexture(GL_TEXTURE_2D, texcontext_sc);
			make_texture(2.0);
		}
		glBindTexture(GL_TEXTURE_2D, texcontext_sc);
		if (draw_colors)
			glColor3f(0.0, 0.0, 0.8);
		else
			glColor3f(0.05, 0.05, 0.05);


		float feature_size2 = sqr(feature_size);
		for (int i = 0; i < nv; i++) {
			texcoords[2*i] = feature_size * kr[i];
			texcoords[2*i+1] = feature_size2 *
					   sctest_num[i] / sctest_den[i];
		}
		draw_tstrips();
	}

	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}


// Color the mesh by curvatures
void compute_curv_colors()
{
	float cscale = sqr(8.0f * feature_size);

	int nv = themesh->vertices.size();
	curv_colors.resize(nv);
	for (int i = 0; i < nv; i++) {
		float H = 0.5f * (themesh->curv1[i] + themesh->curv2[i]);
		float K = themesh->curv1[i] * themesh->curv2[i];
		float h = 4.0f / 3.0f * fabs(atan2(H*H-K,H*H*sgn(H)));
		float s = M_2_PI * atan((2.0f*H*H-K)*cscale);
		curv_colors[i] = Color::hsv(h,s,1.0);
	}
}


// Similar, but grayscale mapping of mean curvature H
void compute_gcurv_colors()
{
	float cscale = 10.0f * feature_size;

	int nv = themesh->vertices.size();
	gcurv_colors.resize(nv);
	for (int i = 0; i < nv; i++) {
		float H = 0.5f * (themesh->curv1[i] + themesh->curv2[i]);
		float c = (atan(H*cscale) + M_PI_2) / M_PI;
		c = sqrt(c);
		int C = int(min(max(256.0 * c, 0.0), 255.99));
		gcurv_colors[i] = Color(C,C,C);
	}
}

// Draw the basic mesh, which we'll overlay with lines
void draw_base_mesh()
{
    glDisable(GL_LIGHTING);

	int nv = themesh->vertices.size();

	// Enable the vertex array
	glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &themesh->vertices[0][0]);
    glNormalPointer(GL_FLOAT, 0, &themesh->normals[0][0]);

	// Set up for color
    if (color_style == "White") {
        glColor3f(1,1,1);
    } else if (color_style == "Gray") {
        glColor3f(0.65, 0.65, 0.65);
    } else if (color_style == "Black") {
        glColor3f(0.0, 0.0, 0.0);
    } else if (color_style == "Curvature") {
        if (curv_colors.empty())
            compute_curv_colors();
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(3, GL_FLOAT, 0,
            &curv_colors[0][0]);
    } else if (color_style == "Gaussian C.") {
        if (gcurv_colors.empty())
            compute_gcurv_colors();
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(3, GL_FLOAT, 0,
            &gcurv_colors[0][0]);
    } else if (color_style == "Mesh") {
        if (themesh->colors.size()) {
            glEnableClientState(GL_COLOR_ARRAY);
            glColorPointer(3, GL_FLOAT, 0,
                &themesh->colors[0][0]);
        } else {
            glColor3f(1,1,1);
        }
	}

    GQShaderRef shader;
    // Shaders for debugging colors
    if (color_style == "Depth") {
        shader = GQShaderManager::bindProgram("depth");
        shader.setUniform3fv("bsphere_center", themesh->bsphere.center);
        shader.setUniform1f("bsphere_radius", themesh->bsphere.r);
    } else if (color_style == "Normals") {
        shader = GQShaderManager::bindProgram("normals");
    }
    // Actual lighting shaders
    else if (lighting_style == "None") {
        shader = GQShaderManager::bindProgram("nolighting");
    } else if (lighting_style == "Lambertian") {
        shader = GQShaderManager::bindProgram("diffuse");
    } else if (lighting_style == "Lambertian2") {
        shader = GQShaderManager::bindProgram("diffuse2");
    } else if (lighting_style == "Hemisphere") {
        shader = GQShaderManager::bindProgram("hemisphere");
    } else if (lighting_style == "Toon") {
        shader = GQShaderManager::bindProgram("toon");
    } else if (lighting_style == "Toon BW") {
        shader = GQShaderManager::bindProgram("toonbw");
    } else if (lighting_style == "Gooch") {
        shader = GQShaderManager::bindProgram("gooch");
    }   
    shader.setUniform3fv("light_dir_world", light_direction);

	// Draw the mesh, possibly with color and/or lighting
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glPolygonOffset(5.0f, 30.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_CULL_FACE);

    // Draw geometry, no display list
    draw_tstrips();

	// Reset everything
    shader.unbind();    
	glDisableClientState(GL_COLOR_ARRAY);
	//glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE); // Do not remove me, else get dotted lines
    
	// Draw the mesh edges on top, if requested
	set_line_width(1);
	if (draw_edges) {
		glPolygonMode(GL_FRONT, GL_LINE);
		glColor3f(0.5, 1.0, 1.0);
		draw_tstrips();
		glPolygonMode(GL_FRONT, GL_FILL);
	}

	// Draw various per-vertex vectors, if requested
	float line_len = 0.5f * themesh->feature_size();
	if (draw_norm) {
		// Normals
		glColor3f(0.7, 0.7, 0);
		glBegin(GL_LINES);
		for (int i = 0; i < nv; i++) {
			glVertex3fv(themesh->vertices[i]);
			glVertex3fv(themesh->vertices[i] +
				    2.0f * line_len * themesh->normals[i]);
		}
		glEnd();
		glPointSize(3);
		glDrawArrays(GL_POINTS, 0, nv);
	}
	if (draw_curv1) {
		// Maximum-magnitude principal direction
		glColor3f(0.2, 0.7, 0.2);
		glBegin(GL_LINES);
		for (int i = 0; i < nv; i++) {
			glVertex3fv(themesh->vertices[i] -
				    line_len * themesh->pdir1[i]);
			glVertex3fv(themesh->vertices[i] +
				    line_len * themesh->pdir1[i]);
		}
		glEnd();
	}
	if (draw_curv2) {
		// Minimum-magnitude principal direction
		glColor3f(0.7, 0.2, 0.2);
		glBegin(GL_LINES);
		for (int i = 0; i < nv; i++) {
			glVertex3fv(themesh->vertices[i] -
				    line_len * themesh->pdir2[i]);
			glVertex3fv(themesh->vertices[i] +
				    line_len * themesh->pdir2[i]);
		}
		glEnd();
	}
	if (draw_asymp) {
		// Asymptotic directions, scaled by sqrt(-K)
		float ascale2 = sqr(5.0f * line_len * feature_size);
		glColor3f(1, 0.5, 0);
		glBegin(GL_LINES);
		for (int i = 0; i < nv; i++) {
			const float &k1 = themesh->curv1[i];
			const float &k2 = themesh->curv2[i];
			float scale2 = -k1 * k2 * ascale2;
			if (scale2 <= 0.0f)
				continue;
			vec ax = sqrt(scale2 * k2 / (k2-k1)) *
				 themesh->pdir1[i];
			vec ay = sqrt(scale2 * k1 / (k1-k2)) *
				 themesh->pdir2[i];
			glVertex3fv(themesh->vertices[i] + ax + ay);
			glVertex3fv(themesh->vertices[i] - ax - ay);
			glVertex3fv(themesh->vertices[i] + ax - ay);
			glVertex3fv(themesh->vertices[i] - ax + ay);
		}
		glEnd();
	}
	if (draw_w) {
		// Projected view direction
		glColor3f(0, 0, 1);
		glBegin(GL_LINES);
		for (int i = 0; i < nv; i++) {
			vec w = viewpos - themesh->vertices[i];
			w -= themesh->normals[i] * (w DOT themesh->normals[i]);
			normalize(w);
			glVertex3fv(themesh->vertices[i]);
			glVertex3fv(themesh->vertices[i] + line_len * w);
		}
		glEnd();
	}
	if (draw_wperp) {
		// Perpendicular to projected view direction
		glColor3f(0, 0, 1);
		glBegin(GL_LINES);
		for (int i = 0; i < nv; i++) {
			vec w = viewpos - themesh->vertices[i];
			w -= themesh->normals[i] * (w DOT themesh->normals[i]);
			vec wperp = themesh->normals[i] CROSS w;
			normalize(wperp);
			glVertex3fv(themesh->vertices[i]);
			glVertex3fv(themesh->vertices[i] + line_len * wperp);
		}
		glEnd();
	}

	glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}


// Compute per-vertex n dot l, n dot v, radial curvature, and
// derivative of curvature for the current view
void compute_perview(vector<float> &ndotv, vector<float> &kr,
		     vector<float> &sctest_num, vector<float> &sctest_den,
		     vector<float> &shtest_num, vector<float> &q1,
		     vector<vec2> &t1, vector<float> &Dt1q1,
		     bool extra_sin2theta = false)
{
	if (draw_apparent)
		themesh->need_adjacentfaces();

	int nv = themesh->vertices.size();

	float scthresh = sug_thresh / sqr(feature_size);
	float shthresh = sh_thresh / sqr(feature_size);
	bool need_DwKr = (draw_sc || draw_sh || draw_DwKr);

	ndotv.resize(nv);
	kr.resize(nv);
	if (draw_apparent) {
		q1.resize(nv);
		t1.resize(nv);
		Dt1q1.resize(nv);
	}
	if (need_DwKr) {
		sctest_num.resize(nv);
		sctest_den.resize(nv);
		if (draw_sh)
			shtest_num.resize(nv);
	}

	// Compute quantities at each vertex
#pragma omp parallel for
	for (int i = 0; i < nv; i++) {
		// Compute n DOT v
		vec viewdir = viewpos - themesh->vertices[i];
		float rlv = 1.0f / len(viewdir);
		viewdir *= rlv;
		ndotv[i] = viewdir DOT themesh->normals[i];

		float u = viewdir DOT themesh->pdir1[i], u2 = u*u;
		float v = viewdir DOT themesh->pdir2[i], v2 = v*v;

		// Note:  this is actually Kr * sin^2 theta
		kr[i] = themesh->curv1[i] * u2 + themesh->curv2[i] * v2;

		if (draw_apparent) {
			float csc2theta = 1.0f / (u2 + v2);
			Rtsc::compute_viewdep_curv(themesh, i, ndotv[i],
				u2*csc2theta, u*v*csc2theta, v2*csc2theta,
				q1[i], t1[i]);
		}
		if (!need_DwKr)
			continue;

		// Use DwKr * sin(theta) / cos(theta) for cutoff test
		sctest_num[i] = u2 * (     u*themesh->dcurv[i][0] +
				      3.0f*v*themesh->dcurv[i][1]) +
				v2 * (3.0f*u*themesh->dcurv[i][2] +
					   v*themesh->dcurv[i][3]);
		float csc2theta = 1.0f / (u2 + v2);
		sctest_num[i] *= csc2theta;
		float tr = (themesh->curv2[i] - themesh->curv1[i]) *
			   u * v * csc2theta;
		sctest_num[i] -= 2.0f * ndotv[i] * sqr(tr);
		if (extra_sin2theta)
			sctest_num[i] *= u2 + v2;

		sctest_den[i] = ndotv[i];

		if (draw_sh) {
			shtest_num[i] = -sctest_num[i];
			shtest_num[i] -= shthresh * sctest_den[i];
		}
		sctest_num[i] -= scthresh * sctest_den[i];
	}
	if (draw_apparent) {
#pragma omp parallel for
		for (int i = 0; i < nv; i++)
			Rtsc::compute_Dt1q1(themesh, i, ndotv[i], q1, t1, Dt1q1[i]);
	}
}


// Compute gradient of (kr * sin^2 theta) at vertex i
static inline vec gradkr(int i)
{
	vec viewdir = viewpos - themesh->vertices[i];
	float rlen_viewdir = 1.0f / len(viewdir);
	viewdir *= rlen_viewdir;

	float ndotv = viewdir DOT themesh->normals[i];
	float sintheta = sqrt(1.0f - sqr(ndotv));
	float csctheta = 1.0f / sintheta;
	float u = (viewdir DOT themesh->pdir1[i]) * csctheta;
	float v = (viewdir DOT themesh->pdir2[i]) * csctheta;
	float kr = themesh->curv1[i] * u*u + themesh->curv2[i] * v*v;
	float tr = u*v * (themesh->curv2[i] - themesh->curv1[i]);
	float kt = themesh->curv1[i] * (1.0f - u*u) +
		   themesh->curv2[i] * (1.0f - v*v);
	vec w     = u * themesh->pdir1[i] + v * themesh->pdir2[i];
	vec wperp = u * themesh->pdir2[i] - v * themesh->pdir1[i];
	const Vec<4> &C = themesh->dcurv[i];

	vec g = themesh->pdir1[i] * (u*u*C[0] + 2.0f*u*v*C[1] + v*v*C[2]) +
		themesh->pdir2[i] * (u*u*C[1] + 2.0f*u*v*C[2] + v*v*C[3]) -
		2.0f * csctheta * tr * (rlen_viewdir * wperp +
					ndotv * (tr * w + kt * wperp));
	g *= (1.0f - sqr(ndotv));
	g -= 2.0f * kr * sintheta * ndotv * (kr * w + tr * wperp);
	return g;
}


// Find a zero crossing between val0 and val1 by linear interpolation
// Returns 0 if zero crossing is at val0, 1 if at val1, etc.
static inline float find_zero_linear(float val0, float val1)
{
	return val0 / (val0 - val1);
}


// Find a zero crossing using Hermite interpolation
float find_zero_hermite(int v0, int v1, float val0, float val1,
			const vec &grad0, const vec &grad1)
{
	if (unlikely(val0 == val1))
		return 0.5f;

	// Find derivatives along edge (of interpolation parameter in [0,1]
	// which means that e01 doesn't get normalized)
	vec e01 = themesh->vertices[v1] - themesh->vertices[v0];
	float d0 = e01 DOT grad0, d1 = e01 DOT grad1;

	// This next line would reduce val to linear interpolation
	//d0 = d1 = (val1 - val0);

	// Use hermite interpolation:
	//   val(s) = h1(s)*val0 + h2(s)*val1 + h3(s)*d0 + h4(s)*d1
	// where
	//  h1(s) = 2*s^3 - 3*s^2 + 1
	//  h2(s) = 3*s^2 - 2*s^3
	//  h3(s) = s^3 - 2*s^2 + s
	//  h4(s) = s^3 - s^2
	//
	//  val(s)  = [2(val0-val1) +d0+d1]*s^3 +
	//            [3(val1-val0)-2d0-d1]*s^2 + d0*s + val0
	// where
	//
	//  val(0) = val0; val(1) = val1; val'(0) = d0; val'(1) = d1
	//

	// Coeffs of cubic a*s^3 + b*s^2 + c*s + d
	float a = 2 * (val0 - val1) + d0 + d1;
	float b = 3 * (val1 - val0) - 2 * d0 - d1;
	float c = d0, d = val0;

	// -- Find a root by bisection
	// (as Newton can wander out of desired interval)

	// Start with entire [0,1] interval
	float sl = 0.0f, sr = 1.0f, valsl = val0, valsr = val1;

	// Check if we're in a (somewhat uncommon) 3-root situation, and pick
	// the middle root if it happens (given we aren't drawing curvy lines,
	// seems the best approach..)
	//
	// Find extrema of derivative (a -> 3a; b -> 2b, c -> c),
	// and check if they're both in [0,1] and have different signs
	float disc = 4 * b - 12 * a * c;
	if (disc > 0 && a != 0) {
		disc = sqrt(disc);
		float r1 = (-2 * b + disc) / (6 * a);
		float r2 = (-2 * b - disc) / (6 * a);
		if (r1 >= 0 && r1 <= 1 && r2 >= 0 && r2 <= 1) {
			float vr1 = (((a * r1 + b) * r1 + c) * r1) + d;
			float vr2 = (((a * r2 + b) * r2 + c) * r2) + d;
			// When extrema have different signs inside an
			// interval with endpoints with different signs,
			// the middle root is in between the two extrema
			if (vr1 < 0.0f && vr2 >= 0.0f ||
			    vr1 > 0.0f && vr2 <= 0.0f) {
				// 3 roots
				if (r1 < r2) {
					sl = r1;
					valsl = vr1;
					sr = r2;
					valsr = vr2;
				} else {
					sl = r2;
					valsl = vr2;
					sr = r1;
					valsr = vr1;
				}
			}
		}
	}

	// Bisection method (constant number of interations)
	for (int iter = 0; iter < 10; iter++) {
		float sbi = (sl + sr) / 2.0f;
		float valsbi = (((a * sbi + b) * sbi) + c) * sbi + d;

		// Keep the half which has different signs
		if (valsl < 0.0f && valsbi >= 0.0f ||
		    valsl > 0.0f && valsbi <= 0.0f) {
			sr = sbi;
			valsr = valsbi;
		} else {
			sl = sbi;
			valsl = valsbi;
		}
	}

	return 0.5f * (sl + sr);
}


// Draw part of a zero-crossing curve on one triangle face, but only if
// "test_num/test_den" is positive.  v0,v1,v2 are the indices of the 3
// vertices, "val" are the values of the scalar field whose zero
// crossings we are finding, and "test_*" are the values we are testing
// to make sure they are positive.  This function assumes that val0 has
// opposite sign from val1 and val2 - the following function is the
// general one that figures out which one actually has the different sign.
void draw_face_isoline2(int v0, int v1, int v2,
			const vector<float> &val,
			const vector<float> &test_num,
			const vector<float> &test_den,
			bool do_hermite, bool do_test, float fade)
{
	// How far along each edge?
	float w10 = do_hermite ?
		find_zero_hermite(v0, v1, val[v0], val[v1],
				  gradkr(v0), gradkr(v1)) :
		find_zero_linear(val[v0], val[v1]);
	float w01 = 1.0f - w10;
	float w20 = do_hermite ?
		find_zero_hermite(v0, v2, val[v0], val[v2],
				  gradkr(v0), gradkr(v2)) :
		find_zero_linear(val[v0], val[v2]);
	float w02 = 1.0f - w20;

	// Points along edges
	point p1 = w01 * themesh->vertices[v0] + w10 * themesh->vertices[v1];
	point p2 = w02 * themesh->vertices[v0] + w20 * themesh->vertices[v2];

	float test_num1 = 1.0f, test_num2 = 1.0f;
	float test_den1 = 1.0f, test_den2 = 1.0f;
	float z1 = 0.0f, z2 = 0.0f;
	bool valid1 = true;
	if (do_test) {
		// Interpolate to find value of test at p1, p2
		test_num1 = w01 * test_num[v0] + w10 * test_num[v1];
		test_num2 = w02 * test_num[v0] + w20 * test_num[v2];
		if (!test_den.empty()) {
			test_den1 = w01 * test_den[v0] + w10 * test_den[v1];
			test_den2 = w02 * test_den[v0] + w20 * test_den[v2];
		}
		// First point is valid iff num1/den1 is positive,
		// i.e. the num and den have the same sign
		valid1 = ((test_num1 >= 0.0f) == (test_den1 >= 0.0f));
		// There are two possible zero crossings of the test,
		// corresponding to zeros of the num and den
		if ((test_num1 >= 0.0f) != (test_num2 >= 0.0f))
			z1 = test_num1 / (test_num1 - test_num2);
		if ((test_den1 >= 0.0f) != (test_den2 >= 0.0f))
			z2 = test_den1 / (test_den1 - test_den2);
		// Sort and order the zero crossings
		if (z1 == 0.0f)
			z1 = z2, z2 = 0.0f;
		else if (z2 < z1)
			swap(z1, z2);
	}

	// If the beginning of the segment was not valid, and
	// no zero crossings, then whole segment invalid
	if (!valid1 && !z1 && !z2)
		return;

	// Draw the valid piece(s)
	int npts = 0;
	if (valid1) {
		glColor4f(currcolor[0], currcolor[1], currcolor[2],
			  test_num1 / (test_den1 * fade + test_num1));
		glVertex3fv(p1);
		npts++;
	}
	if (z1) {
		float num = (1.0f - z1) * test_num1 + z1 * test_num2;
		float den = (1.0f - z1) * test_den1 + z1 * test_den2;
		glColor4f(currcolor[0], currcolor[1], currcolor[2],
			  num / (den * fade + num));
		glVertex3fv((1.0f - z1) * p1 + z1 * p2);
		npts++;
	}
	if (z2) {
		float num = (1.0f - z2) * test_num1 + z2 * test_num2;
		float den = (1.0f - z2) * test_den1 + z2 * test_den2;
		glColor4f(currcolor[0], currcolor[1], currcolor[2],
			  num / (den * fade + num));
		glVertex3fv((1.0f - z2) * p1 + z2 * p2);
		npts++;
	}
	if (npts != 2) {
		glColor4f(currcolor[0], currcolor[1], currcolor[2],
			  test_num2 / (test_den2 * fade + test_num2));
		glVertex3fv(p2);
	}
}


// See above.  This is the driver function that figures out which of
// v0, v1, v2 has a different sign from the others.
void draw_face_isoline(int v0, int v1, int v2,
		       const vector<float> &val,
		       const vector<float> &test_num,
		       const vector<float> &test_den,
		       const vector<float> &ndotv,
		       bool do_bfcull, bool do_hermite,
		       bool do_test, float fade)
{
	// Backface culling
	if (likely(do_bfcull && ndotv[v0] <= 0.0f &&
		   ndotv[v1] <= 0.0f && ndotv[v2] <= 0.0f))
		return;

	// Quick reject if derivs are negative
	if (do_test) {
		if (test_den.empty()) {
			if (test_num[v0] <= 0.0f &&
			    test_num[v1] <= 0.0f &&
			    test_num[v2] <= 0.0f)
				return;
		} else {
			if (test_num[v0] <= 0.0f && test_den[v0] >= 0.0f &&
			    test_num[v1] <= 0.0f && test_den[v1] >= 0.0f &&
			    test_num[v2] <= 0.0f && test_den[v2] >= 0.0f)
				return;
			if (test_num[v0] >= 0.0f && test_den[v0] <= 0.0f &&
			    test_num[v1] >= 0.0f && test_den[v1] <= 0.0f &&
			    test_num[v2] >= 0.0f && test_den[v2] <= 0.0f)
				return;
		}
	}

	// Figure out which val has different sign, and draw
	if (val[v0] < 0.0f && val[v1] >= 0.0f && val[v2] >= 0.0f ||
	    val[v0] > 0.0f && val[v1] <= 0.0f && val[v2] <= 0.0f)
		draw_face_isoline2(v0, v1, v2,
				   val, test_num, test_den,
				   do_hermite, do_test, fade);
	else if (val[v1] < 0.0f && val[v2] >= 0.0f && val[v0] >= 0.0f ||
		 val[v1] > 0.0f && val[v2] <= 0.0f && val[v0] <= 0.0f)
		draw_face_isoline2(v1, v2, v0,
				   val, test_num, test_den,
				   do_hermite, do_test, fade);
	else if (val[v2] < 0.0f && val[v0] >= 0.0f && val[v1] >= 0.0f ||
		 val[v2] > 0.0f && val[v0] <= 0.0f && val[v1] <= 0.0f)
		draw_face_isoline2(v2, v0, v1,
				   val, test_num, test_den,
				   do_hermite, do_test, fade);
}


// Takes a scalar field and renders the zero crossings, but only where
// test_num/test_den is greater than 0.
void draw_isolines(const vector<float> &val,
		   const vector<float> &test_num,
		   const vector<float> &test_den,
		   const vector<float> &ndotv,
		   bool do_bfcull, bool do_hermite,
		   bool do_test, float fade)
{
	const int *t = &themesh->tstrips[0];
	const int *stripend = t;
	const int *end = t + themesh->tstrips.size();

	// Walk through triangle strips
	while (1) {
		if (unlikely(t >= stripend)) {
			if (unlikely(t >= end))
				return;
			// New strip: each strip is stored as
			// length followed by indices
			stripend = t + 1 + *t;
			// Skip over length plus first two indices of
			// first face
			t += 3;
		}
		// Draw a line if, among the values in this triangle,
		// at least one is positive and one is negative
		const float &v0 = val[*t], &v1 = val[*(t-1)], &v2 = val[*(t-2)];
		if (unlikely((v0 > 0.0f || v1 > 0.0f || v2 > 0.0f) &&
			     (v0 < 0.0f || v1 < 0.0f || v2 < 0.0f)))
			draw_face_isoline(*(t-2), *(t-1), *t,
					  val, test_num, test_den, ndotv,
					  do_bfcull, do_hermite, do_test, fade);
		t++;
	}
}


// Draw part of a ridge/valley curve on one triangle face.  v0,v1,v2
// are the indices of the 3 vertices; this function assumes that the
// curve connects points on the edges v0-v1 and v1-v2
// (or connects point on v0-v1 to center if to_center is true)
void draw_segment_ridge(int v0, int v1, int v2,
			float emax0, float emax1, float emax2,
			float kmax0, float kmax1, float kmax2,
			float thresh, bool to_center)
{
	// Interpolate to find ridge/valley line segment endpoints
	// in this triangle and the curvatures there
	float w10 = fabs(emax0) / (fabs(emax0) + fabs(emax1));
	float w01 = 1.0f - w10;
	point p01 = w01 * themesh->vertices[v0] + w10 * themesh->vertices[v1];
	float k01 = fabs(w01 * kmax0 + w10 * kmax1);

	point p12;
	float k12;
	if (to_center) {
		// Connect first point to center of triangle
		p12 = (themesh->vertices[v0] +
		       themesh->vertices[v1] +
		       themesh->vertices[v2]) / 3.0f;
		k12 = fabs(kmax0 + kmax1 + kmax2) / 3.0f;
	} else {
		// Connect first point to second one (on next edge)
		float w21 = fabs(emax1) / (fabs(emax1) + fabs(emax2));
		float w12 = 1.0f - w21;
		p12 = w12 * themesh->vertices[v1] + w21 * themesh->vertices[v2];
		k12 = fabs(w12 * kmax1 + w21 * kmax2);
	}

	// Don't draw below threshold
	k01 -= thresh;
	if (k01 < 0.0f)
		k01 = 0.0f;
	k12 -= thresh;
	if (k12 < 0.0f)
		k12 = 0.0f;

	// Skip lines that you can't see...
	if (k01 == 0.0f && k12 == 0.0f)
		return;

	// Fade lines
	if (draw_faded) {
		k01 /= (k01 + thresh);
		k12 /= (k12 + thresh);
	} else {
		k01 = k12 = 1.0f;
	}

	// Draw the line segment
	glColor4f(currcolor[0], currcolor[1], currcolor[2], k01);
	glVertex3fv(p01);
	glColor4f(currcolor[0], currcolor[1], currcolor[2], k12);
	glVertex3fv(p12);
}


// Draw ridges or valleys (depending on do_ridge) in a triangle v0,v1,v2
// - uses ndotv for backface culling (enabled with do_bfcull)
// - do_test checks for curvature maxima/minina for ridges/valleys
//   (when off, it draws positive minima and negative maxima)
// Note: this computes ridges/valleys every time, instead of once at the
//   start (given they aren't view dependent, this is wasteful)
// Algorithm based on formulas of Ohtake et al., 2004.
void draw_face_ridges(int v0, int v1, int v2,
		      bool do_ridge,
		      const vector<float> &ndotv,
		      bool do_bfcull, bool do_test, float thresh)
{
	// Backface culling
	if (likely(do_bfcull &&
		   ndotv[v0] <= 0.0f && ndotv[v1] <= 0.0f && ndotv[v2] <= 0.0f))
		return;

	// Check if ridge possible at vertices just based on curvatures
	if (do_ridge) {
		if ((themesh->curv1[v0] <= 0.0f) ||
		    (themesh->curv1[v1] <= 0.0f) ||
		    (themesh->curv1[v2] <= 0.0f))
			return;
	} else {
		if ((themesh->curv1[v0] >= 0.0f) ||
		    (themesh->curv1[v1] >= 0.0f) ||
		    (themesh->curv1[v2] >= 0.0f))
			return;
	}

	// Sign of curvature on ridge/valley
	float rv_sign = do_ridge ? 1.0f : -1.0f;

	// The "tmax" are the principal directions of maximal curvature,
	// flipped to point in the direction in which the curvature
	// is increasing (decreasing for valleys).  Note that this
	// is a bit different from the notation in Ohtake et al.,
	// but the tests below are equivalent.
	const float &emax0 = themesh->dcurv[v0][0];
	const float &emax1 = themesh->dcurv[v1][0];
	const float &emax2 = themesh->dcurv[v2][0];
	vec tmax0 = rv_sign * themesh->dcurv[v0][0] * themesh->pdir1[v0];
	vec tmax1 = rv_sign * themesh->dcurv[v1][0] * themesh->pdir1[v1];
	vec tmax2 = rv_sign * themesh->dcurv[v2][0] * themesh->pdir1[v2];

	// We have a "zero crossing" if the tmaxes along an edge
	// point in opposite directions
	bool z01 = ((tmax0 DOT tmax1) <= 0.0f);
	bool z12 = ((tmax1 DOT tmax2) <= 0.0f);
	bool z20 = ((tmax2 DOT tmax0) <= 0.0f);

	if (z01 + z12 + z20 < 2)
		return;

	if (do_test) {
		const point &p0 = themesh->vertices[v0],
			    &p1 = themesh->vertices[v1],
			    &p2 = themesh->vertices[v2];

		// Check whether we have the correct flavor of extremum:
		// Is the curvature increasing along the edge?
		z01 = z01 && ((tmax0 DOT (p1 - p0)) >= 0.0f ||
			      (tmax1 DOT (p1 - p0)) <= 0.0f);
		z12 = z12 && ((tmax1 DOT (p2 - p1)) >= 0.0f ||
			      (tmax2 DOT (p2 - p1)) <= 0.0f);
		z20 = z20 && ((tmax2 DOT (p0 - p2)) >= 0.0f ||
			      (tmax0 DOT (p0 - p2)) <= 0.0f);

		if (z01 + z12 + z20 < 2)
			return;
	}

	// Draw line segment
	const float &kmax0 = themesh->curv1[v0];
	const float &kmax1 = themesh->curv1[v1];
	const float &kmax2 = themesh->curv1[v2];
	if (!z01) {
		draw_segment_ridge(v1, v2, v0,
				   emax1, emax2, emax0,
				   kmax1, kmax2, kmax0,
				   thresh, false);
	} else if (!z12) {
		draw_segment_ridge(v2, v0, v1,
				   emax2, emax0, emax1,
				   kmax2, kmax0, kmax1,
				   thresh, false);
	} else if (!z20) {
		draw_segment_ridge(v0, v1, v2,
				   emax0, emax1, emax2,
				   kmax0, kmax1, kmax2,
				   thresh, false);
	} else {
		// All three edges have crossings -- connect all to center
		draw_segment_ridge(v1, v2, v0,
				   emax1, emax2, emax0,
				   kmax1, kmax2, kmax0,
				   thresh, true);
		draw_segment_ridge(v2, v0, v1,
				   emax2, emax0, emax1,
				   kmax2, kmax0, kmax1,
				   thresh, true);
		draw_segment_ridge(v0, v1, v2,
				   emax0, emax1, emax2,
				   kmax0, kmax1, kmax2,
				   thresh, true);
	}
}


// Draw the ridges (valleys) of the mesh
void draw_mesh_ridges(bool do_ridge, const vector<float> &ndotv,
		      bool do_bfcull, bool do_test, float thresh)
{
	const int *t = &themesh->tstrips[0];
	const int *stripend = t;
	const int *end = t + themesh->tstrips.size();

	// Walk through triangle strips
	while (1) {
		if (unlikely(t >= stripend)) {
			if (unlikely(t >= end))
				return;
			// New strip: each strip is stored as
			// length followed by indices
			stripend = t + 1 + *t;
			// Skip over length plus first two indices of
			// first face
			t += 3;
		}

		draw_face_ridges(*(t-2), *(t-1), *t,
				 do_ridge, ndotv, do_bfcull, do_test, thresh);
		t++;
	}
}


// Draw principal highlights on a face
void draw_face_ph(int v0, int v1, int v2, bool do_ridge,
		  const vector<float> &ndotv, bool do_bfcull,
		  bool do_test, float thresh)
{
	// Backface culling
	if (likely(do_bfcull &&
		   ndotv[v0] <= 0.0f && ndotv[v1] <= 0.0f && ndotv[v2] <= 0.0f))
		return;

	// Orient principal directions based on the largest principal curvature
	float k0 = themesh->curv1[v0];
	float k1 = themesh->curv1[v1];
	float k2 = themesh->curv1[v2];
	if (do_test && do_ridge && min(min(k0,k1),k2) < 0.0f)
		return;
	if (do_test && !do_ridge && max(max(k0,k1),k2) > 0.0f)
		return;

	vec d0 = themesh->pdir1[v0];
	vec d1 = themesh->pdir1[v1];
	vec d2 = themesh->pdir1[v2];
	float kmax = fabs(k0);
        // dref is the e1 vector with the largest |k1|
	vec dref = d0;
	if (fabs(k1) > kmax)
		kmax = fabs(k1), dref = d1;
	if (fabs(k2) > kmax)
		kmax = fabs(k2), dref = d2;
        
        // Flip all the e1 to agree with dref
	if ((d0 DOT dref) < 0.0f) d0 = -d0;
	if ((d1 DOT dref) < 0.0f) d1 = -d1;
	if ((d2 DOT dref) < 0.0f) d2 = -d2;

        // If directions have flipped (more than 45 degrees), then give up
        if ((d0 DOT dref) < M_SQRT1_2 ||
            (d1 DOT dref) < M_SQRT1_2 ||
            (d2 DOT dref) < M_SQRT1_2)
          return;

	// Compute view directions, dot products @ each vertex
	vec viewdir0 = viewpos - themesh->vertices[v0];
	vec viewdir1 = viewpos - themesh->vertices[v1];
	vec viewdir2 = viewpos - themesh->vertices[v2];

        // Normalize these for cos(theta) later...
        normalize(viewdir0);
        normalize(viewdir1);
        normalize(viewdir2);

        // e1 DOT w sin(theta) 
        // -- which is zero when looking down e2
	float dot0 = viewdir0 DOT d0;
	float dot1 = viewdir1 DOT d1;
	float dot2 = viewdir2 DOT d2;

	// We have a "zero crossing" if the dot products along an edge
	// have opposite signs
	int z01 = (dot0*dot1 <= 0.0f);
	int z12 = (dot1*dot2 <= 0.0f);
	int z20 = (dot2*dot0 <= 0.0f);

	if (z01 + z12 + z20 < 2)
		return;

	// Draw line segment
	float test0 = (sqr(themesh->curv1[v0]) - sqr(themesh->curv2[v0])) *
                      viewdir0 DOT themesh->normals[v0];
	float test1 = (sqr(themesh->curv1[v1]) - sqr(themesh->curv2[v1])) *
                      viewdir0 DOT themesh->normals[v1];
	float test2 = (sqr(themesh->curv1[v2]) - sqr(themesh->curv2[v2])) *
                      viewdir0 DOT themesh->normals[v2];

	if (!z01) {
		draw_segment_ridge(v1, v2, v0,
				   dot1, dot2, dot0,
				   test1, test2, test0,
				   thresh, false);
	} else if (!z12) {
		draw_segment_ridge(v2, v0, v1,
				   dot2, dot0, dot1,
				   test2, test0, test1,
				   thresh, false);
	} else if (!z20) {
		draw_segment_ridge(v0, v1, v2,
				   dot0, dot1, dot2,
				   test0, test1, test2,
				   thresh, false);
	}
}


// Draw principal highlights
void draw_mesh_ph(bool do_ridge, const vector<float> &ndotv, bool do_bfcull,
		  bool do_test, float thresh)
{
	const int *t = &themesh->tstrips[0];
	const int *stripend = t;
	const int *end = t + themesh->tstrips.size();

	// Walk through triangle strips
	while (1) {
		if (unlikely(t >= stripend)) {
			if (unlikely(t >= end))
				return;
			// New strip: each strip is stored as
			// length followed by indices
			stripend = t + 1 + *t;
			// Skip over length plus first two indices of
			// first face
			t += 3;
		}

		draw_face_ph(*(t-2), *(t-1), *t, do_ridge,
			     ndotv, do_bfcull, do_test, thresh);
		t++;
	}
}


// Draw exterior silhouette of the mesh: this just draws
// thick contours, which are partially hidden by the mesh.
// Note: this needs to happen *before* draw_base_mesh...
void draw_silhouette(const vector<float> &ndotv)
{
	glDepthMask(GL_FALSE);

	currcolor = vec(0.0, 0.0, 0.0);
	set_line_width(6);
	glBegin(GL_LINES);
	draw_isolines(ndotv, vector<float>(), vector<float>(), ndotv,
		      false, false, false, 0.0f);
	glEnd();

	// Wide lines are gappy, so fill them in
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPointSize(6);
	glBegin(GL_POINTS);
	draw_isolines(ndotv, vector<float>(), vector<float>(), ndotv,
		      false, false, false, 0.0f);
	glEnd();

	glDisable(GL_POINT_SMOOTH);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}


// Draw the boundaries on the mesh
void draw_boundaries(bool do_hidden)
{
	themesh->need_faces();
	themesh->need_across_edge();
	if (do_hidden) {
		glColor3f(0.6, 0.6, 0.6);
		set_line_width(1.5);
	} else {
		glColor3f(0.05, 0.05, 0.05);
		set_line_width(2.5);
	}
	glBegin(GL_LINES);
	for (int i = 0; i < themesh->faces.size(); i++) {
		for (int j = 0; j < 3; j++) {
			if (themesh->across_edge[i][j] >= 0)
				continue;
			int v1 = themesh->faces[i][(j+1)%3];
			int v2 = themesh->faces[i][(j+2)%3];
			glVertex3fv(themesh->vertices[v1]);
			glVertex3fv(themesh->vertices[v2]);
		}
	}
	glEnd();
}


// Draw lines of n.l = const.
void draw_isophotes(const vector<float> &ndotv)
{
	// Compute N dot L
	int nv = themesh->vertices.size();
	static vector<float> ndotl;
	ndotl.resize(nv);
	for (int i = 0; i < nv; i++)
		ndotl[i] = themesh->normals[i] DOT light_direction;

	if (draw_colors)
		currcolor = vec(0.4, 0.8, 0.4);
	else
		currcolor = vec(0.6, 0.6, 0.6);
	glColor3fv(currcolor);

	float dt = 1.0f / niso;
	for (int it = 0; it < niso; it++) {
		if (it == 0) {
			set_line_width(2);
		} else {
			set_line_width(1);
			for (int i = 0; i < nv; i++)
				ndotl[i] -= dt;
		}
		glBegin(GL_LINES);
		draw_isolines(ndotl, vector<float>(), vector<float>(),
			      ndotv, true, false, false, 0.0f);
		glEnd();
	}

        // Draw negative isophotes (useful when light is not at camera)
	if (draw_colors)
		currcolor = vec(0.6, 0.9, 0.6);
	else
		currcolor = vec(0.7, 0.7, 0.7);
	glColor3fv(currcolor);

	for (int i = 0; i < nv; i++)
		ndotl[i] += dt * (niso-1);
	for (int it = 1; it < niso; it++) {
		set_line_width(1.0);
		for (int i = 0; i < nv; i++)
			ndotl[i] += dt;
		glBegin(GL_LINES);
		draw_isolines(ndotl, vector<float>(), vector<float>(),
			      ndotv, true, false, false, 0.0f);
		glEnd();
	}
}


// Draw lines of constant depth
void draw_topolines(const vector<float> &ndotv)
{
	// Camera direction and scale
	vec camdir(xf[2], xf[6], xf[10]);
	float depth_scale = 0.5f / themesh->bsphere.r * ntopo;
	float depth_offset = 0.5f * ntopo - topo_offset;

	// Compute depth
	static vector<float> depth;
	int nv = themesh->vertices.size();
	depth.resize(nv);
	for (int i = 0; i < nv; i++) {
		depth[i] = ((themesh->vertices[i] - themesh->bsphere.center)
			     DOT camdir) * depth_scale + depth_offset;
	}

	// Draw the topo lines
	set_line_width(1);
	glColor3f(0.5, 0.5, 0.5);
	for (int it = 0; it < ntopo; it++) {
		glBegin(GL_LINES);
		draw_isolines(depth, vector<float>(), vector<float>(),
			      ndotv, true, false, false, 0.0f);
		glEnd();
		for (int i = 0; i < nv; i++)
			depth[i] -= 1.0f;
	}
}


// Draw K=0, H=0, and DwKr=thresh lines
void draw_misc(const vector<float> &ndotv, const vector<float> &DwKr,
	       bool do_hidden)
{
	if (do_hidden) {
		currcolor = vec(1, 0.5, 0.5);
		set_line_width(1);
	} else {
		currcolor = vec(1, 0, 0);
		set_line_width(2);
	}

	int nv = themesh->vertices.size();
	if (draw_K) {
		vector<float> K(nv);
		for (int i = 0; i < nv; i++)
			K[i] = themesh->curv1[i] * themesh->curv2[i];
		glBegin(GL_LINES);
		draw_isolines(K, vector<float>(), vector<float>(), ndotv,
			      !do_hidden, false, false, 0.0f);
		glEnd();
	}
	if (draw_H) {
		vector<float> H(nv);
		for (int i = 0; i < nv; i++)
			H[i] = 0.5f * (themesh->curv1[i] + themesh->curv2[i]);
		glBegin(GL_LINES);
		draw_isolines(H, vector<float>(), vector<float>(), ndotv,
			      !do_hidden, false, false, 0.0f);
		glEnd();
	}
	if (draw_DwKr) {
		glBegin(GL_LINES);
		draw_isolines(DwKr, vector<float>(), vector<float>(), ndotv,
			      !do_hidden, false, false, 0.0f);
		glEnd();
	}
}
    


void draw_lines()
{
    
	int nv = themesh->vertices.size();

    bool draw_light_lines = color_style == "Gray" || lighting_style != "None";
    
	// First rendering pass (in light gray) if drawing hidden lines
	if (draw_hidden) {
		glDisable(GL_DEPTH_TEST);
        
		// K=0, H=0, DwKr=thresh
		draw_misc(ndotv, sctest_num, true);
        
		// Apparent ridges
		if (draw_apparent) {
			if (draw_colors) {
				currcolor = vec(0.8, 0.8, 0.4);
			} else {
				if (draw_light_lines)
					currcolor = vec(0.75, 0.75, 0.75);
				else
					currcolor = vec(0.55, 0.55, 0.55);
			}
			if (draw_colors)
                set_line_width(2);
			glBegin(GL_LINES);
			Rtsc::draw_mesh_app_ridges(ndotv, q1, t1, Dt1q1, true,
                                 test_ar, ar_thresh / sqr(feature_size));
			glEnd();
		}
        
		// Ridges and valleys
		currcolor = vec(0.55, 0.55, 0.55);
		if (draw_ridges) {
			if (draw_colors)
				currcolor = vec(0.72, 0.6, 0.72);
			set_line_width(1);
			glBegin(GL_LINES);
			draw_mesh_ridges(true, ndotv, false, test_rv,
                             rv_thresh / feature_size);
			glEnd();
		}
		if (draw_valleys) {
			if (draw_colors)
				currcolor = vec(0.8, 0.72, 0.68);
			set_line_width(1);
			glBegin(GL_LINES);
			draw_mesh_ridges(false, ndotv, false, test_rv,
                             rv_thresh / feature_size);
			glEnd();
		}
        
		// Principal highlights
		if (draw_phridges || draw_phvalleys) {
			if (draw_colors) {
				currcolor = vec(0.5,0,0);
			} else {
				if (draw_light_lines)
					currcolor = vec(0.75, 0.75, 0.75);
				else
					currcolor = vec(0.55, 0.55, 0.55);
			}
			set_line_width(2);
			glBegin(GL_LINES);
			float thresh = ph_thresh / sqr(feature_size);
			if (draw_phridges)
				draw_mesh_ph(true, ndotv, false, test_ph, thresh);
			if (draw_phvalleys)
				draw_mesh_ph(false, ndotv, false, test_ph, thresh);
			glEnd();
		}
        
		// Suggestive highlights
		if (draw_sh) {
			if (draw_colors) {
				currcolor = vec(0.5,0,0);
			} else {
				if (draw_light_lines)
					currcolor = vec(0.75, 0.75, 0.75);
				else
					currcolor = vec(0.55,0.55,0.55);
			}
			float fade = draw_faded ? 0.03f / sqr(feature_size) : 0.0f;
			set_line_width(2.5);
			glBegin(GL_LINES);
			draw_isolines(kr, shtest_num, sctest_den, ndotv,
                          false, use_hermite, test_sh, fade);
			glEnd();
		}
        
		// Suggestive contours and contours
		if (draw_sc) {
			float fade = (draw_faded && test_sc) ?
            0.03f / sqr(feature_size) : 0.0f;
			if (draw_colors)
				currcolor = vec(0.5, 0.5, 1.0);
			set_line_width(1.5);
			glBegin(GL_LINES);
			draw_isolines(kr, sctest_num, sctest_den, ndotv,
                          false, use_hermite, test_sc, fade);
			glEnd();
		}
        
		if (draw_c) {
			if (draw_colors)
				currcolor = vec(0.4, 0.8, 0.4);
			set_line_width(1.5);
			glBegin(GL_LINES);
			draw_isolines(ndotv, kr, vector<float>(), ndotv,
                          false, false, test_c, 0.0f);
			glEnd();
		}
        
		// Boundaries
		if (draw_bdy)
			draw_boundaries(true);
        
		glEnable(GL_DEPTH_TEST);
	}
    
    
	// The main rendering pass
    
	// Isophotes
	if (draw_isoph)
		draw_isophotes(ndotv);
    
	// Topo lines
	if (draw_topo)
		draw_topolines(ndotv);
    
	// K=0, H=0, DwKr=thresh
	draw_misc(ndotv, sctest_num, false);
    
	// Apparent ridges
	currcolor = vec(0.0, 0.0, 0.0);
	if (draw_apparent) {
		if (draw_colors)
			currcolor = vec(0.4, 0.4, 0);
		set_line_width(2.5);
		glBegin(GL_LINES);
		draw_mesh_app_ridges(ndotv, q1, t1, Dt1q1, true,
                             test_ar, ar_thresh / sqr(feature_size));
		glEnd();
	}
    
	// Ridges and valleys
	currcolor = vec(0.0, 0.0, 0.0);
	float rvfade = draw_faded ? rv_thresh / feature_size : 0.0f;
	if (draw_ridges) {
		if (draw_colors)
			currcolor = vec(0.3, 0.0, 0.3);
		set_line_width(2);
		glBegin(GL_LINES);
		draw_mesh_ridges(true, ndotv, true, test_rv,
                         rv_thresh / feature_size);
		glEnd();
	}
	if (draw_valleys) {
		if (draw_colors)
			currcolor = vec(0.5, 0.3, 0.2);
		set_line_width(2);
		glBegin(GL_LINES);
		draw_mesh_ridges(false, ndotv, true, test_rv,
                         rv_thresh / feature_size);
		glEnd();
	}
    
	// Principal highlights
	if (draw_phridges || draw_phvalleys) {
		if (draw_colors) {
			currcolor = vec(0.5, 0, 0);
		} else {
			if (draw_light_lines)
				currcolor = vec(1, 1, 1);
			else
				currcolor = vec(0, 0, 0);
		}
		set_line_width(2);
		glBegin(GL_LINES);
		float thresh = ph_thresh / sqr(feature_size);
		if (draw_phridges)
			draw_mesh_ph(true, ndotv, true, test_ph, thresh);
		if (draw_phvalleys)
			draw_mesh_ph(false, ndotv, true, test_ph, thresh);
		glEnd();
		currcolor = vec(0.0, 0.0, 0.0);
	}
    
	// Suggestive highlights
    if (draw_sh) {
		if (draw_colors) {
			currcolor = vec(0.5,0,0);
		} else {
			if (draw_light_lines)
				currcolor = vec(1.0, 1.0, 1.0);
			else
				currcolor = vec(0.3,0.3,0.3);
		}
		float fade = draw_faded ? 0.03f / sqr(feature_size) : 0.0f;
		set_line_width(2.5);
		glBegin(GL_LINES);
		draw_isolines(kr, shtest_num, sctest_den, ndotv,
                      true, use_hermite, test_sh, fade);
		glEnd();
		currcolor = vec(0.0, 0.0, 0.0);
    }
    
	// Kr = 0 loops
	if (draw_sc && !test_sc && !draw_hidden) {
		if (draw_colors)
			currcolor = vec(0.5, 0.5, 1.0);
		else
			currcolor = vec(0.6, 0.6, 0.6);
		set_line_width(1.5);
		glBegin(GL_LINES);
		draw_isolines(kr, sctest_num, sctest_den, ndotv,
                      true, use_hermite, false, 0.0f);
		glEnd();
		currcolor = vec(0.0, 0.0, 0.0);
	}
    
	// Suggestive contours and contours
	if (draw_sc && !use_texture) {
		float fade = draw_faded ? 0.03f / sqr(feature_size) : 0.0f;
		if (draw_colors)
			currcolor = vec(0.0, 0.0, 0.8);
		set_line_width(2.5);
		glBegin(GL_LINES);
		draw_isolines(kr, sctest_num, sctest_den, ndotv,
                      true, use_hermite, true, fade);
		glEnd();
	}
	if (draw_c && !use_texture) {
		if (draw_colors)
			currcolor = vec(0.0, 0.6, 0.0);
		set_line_width(2.5);
		glBegin(GL_LINES);
		draw_isolines(ndotv, kr, vector<float>(), ndotv,
                      false, false, true, 0.0f);
		glEnd();
	}
	if ((draw_sc || draw_c) && use_texture)
		draw_c_sc_texture(ndotv, kr, sctest_num, sctest_den);
    
    
    
	// Boundaries
	if (draw_bdy)
		draw_boundaries(false);
}
    
// Draw the mesh, possibly including a bunch of lines
void draw_everything()
{
	compute_perview(ndotv, kr, sctest_num, sctest_den, shtest_num,
                    q1, t1, Dt1q1, use_texture);
    
	// Enable antialiased lines
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Exterior silhouette
	if (draw_extsil && enable_lines)
		draw_silhouette(ndotv);

	// The mesh itself, possibly colored and/or lit
	glDisable(GL_BLEND);
	draw_base_mesh();
	glEnable(GL_BLEND);

	if (enable_lines) {
        draw_lines();
    }

	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POINT_SMOOTH);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}


void setCameraTransform(xform main)
{
    xf = main;
    viewpos = inv(xf) * point(0,0,0);
}
    
void setLightDir(const vec& lightdir)
{
    light_direction = lightdir;
}

// Draw the scene
void redraw()
{
    if (background_style == "Black") {
        glClearColor(0.0,0.0,0.0,0.0);
    } else if (background_style == "Gray") {
        glClearColor(0.5,0.5,0.5,0.0);
    } else {
        glClearColor(1.0,1.0,1.0,0.0);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_everything();
}

// Smooth the mesh
void filter_mesh(int dummy = 0)
{
	printf("\r");  fflush(stdout);
	smooth_mesh(themesh, currsmooth);

	themesh->pointareas.clear();
	themesh->normals.clear();
	themesh->curv1.clear();
	themesh->dcurv.clear();
	themesh->need_normals();
	themesh->need_curvatures();
	themesh->need_dcurv();
	curv_colors.clear();
	gcurv_colors.clear();
	currsmooth *= 1.1f;
}


// Diffuse the normals across the mesh
void filter_normals(int dummy = 0)
{
	printf("\r");  fflush(stdout);
	diffuse_normals(themesh, currsmooth);
	themesh->curv1.clear();
	themesh->dcurv.clear();
	themesh->need_curvatures();
	themesh->need_dcurv();
	curv_colors.clear();
	gcurv_colors.clear();
	currsmooth *= 1.1f;
}


// Diffuse the curvatures across the mesh
void filter_curv(int dummy = 0)
{
	printf("\r");  fflush(stdout);
	diffuse_curv(themesh, currsmooth);
	themesh->dcurv.clear();
	themesh->need_dcurv();
	curv_colors.clear();
	gcurv_colors.clear();
	currsmooth *= 1.1f;
}


// Diffuse the curvature derivatives across the mesh
void filter_dcurv(int dummy = 0)
{
	printf("\r");  fflush(stdout);
	diffuse_dcurv(themesh, currsmooth);
	curv_colors.clear();
	gcurv_colors.clear();
	currsmooth *= 1.1f;
}


// Perform an iteration of subdivision
void subdivide_mesh(int dummy = 0)
{
	printf("\r");  fflush(stdout);
	subdiv(themesh);

	themesh->need_tstrips();
	themesh->need_normals();
	themesh->need_pointareas();
	themesh->need_curvatures();
	themesh->need_dcurv();
	curv_colors.clear();
	gcurv_colors.clear();
}


// Compute a "feature size" for the mesh: computed as 1% of
// the reciprocal of the 10-th percentile curvature
void compute_feature_size()
{
	int nv = themesh->curv1.size();
	int nsamp = min(nv, 500);

	vector<float> samples;
	samples.reserve(nsamp * 2);

	for (int i = 0; i < nsamp; i++) {
		// Quick 'n dirty portable random number generator
		static unsigned randq = 0;
		randq = unsigned(1664525) * randq + unsigned(1013904223);

		int ind = randq % nv;
		samples.push_back(fabs(themesh->curv1[ind]));
		samples.push_back(fabs(themesh->curv2[ind]));
	}

	const float frac = 0.1f;
	const float mult = 0.01f;
	themesh->need_bsphere();
	float max_feature_size = 0.05f * themesh->bsphere.r;

	int which = int(frac * samples.size());
	nth_element(samples.begin(), samples.begin() + which, samples.end());

	feature_size = min(mult / samples[which], max_feature_size);
}

void initialize(TriMesh* mesh)
{
	themesh = mesh;

	themesh->need_tstrips();
	themesh->need_bsphere();
	themesh->need_normals();
	themesh->need_curvatures();
	themesh->need_dcurv();
	compute_feature_size();
	currsmooth = 0.5f * themesh->feature_size();
}
    
} // namespace Rtsc


