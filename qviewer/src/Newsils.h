#include "XForm.h"
#include "Vec.h"

class TriMesh;

namespace Newsils {

void initialize(TriMesh* mesh);

void findIsolines(vec viewpos, xform proj);
void drawIsolines();
    
void drawEdgeSilhouettes(vec viewpos);

}

