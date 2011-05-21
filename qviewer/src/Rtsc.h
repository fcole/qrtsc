#include "XForm.h"

class TriMesh;

namespace Rtsc {

// Initialize global variables that were previous done in main()
void initialize(TriMesh* mesh);
void setCameraTransform(xform main);
void setLightDir(const vec& lightdir);
void redraw();

// Smooth the mesh
void filter_mesh(int dummy = 0);
// Diffuse the normals across the mesh
void filter_normals(int dummy = 0);
// Diffuse the curvatures across the mesh
void filter_curv(int dummy = 0);
// Diffuse the curvature derivatives across the mesh
void filter_dcurv(int dummy = 0);
// Perform an iteration of subdivision
void subdivide_mesh(int dummy = 0);

}

