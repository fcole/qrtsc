/*****************************************************************************\

GQDraw.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Utility drawing calls that wrap OpenGL calls.

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _GQ_DRAW_H
#define _GQ_DRAW_H

#include "GQVertexBufferSet.h"

namespace GQDraw
{
	void drawElements(const GQVertexBufferSet& vb, int gl_mode, int offset, 
					  int length, const int* indices = 0);
};

#endif