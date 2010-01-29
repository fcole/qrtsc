/*****************************************************************************\

GQInclude.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Common include headers for libgq and dependent libraries. The main purpose
is to make sure the OpenGL headers get included properly.

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _GQ_INCLUDE_H_
#define _GQ_INCLUDE_H_

#ifdef DARWIN
#    include <GLee.h>
#    include <glu.h>
#endif
#ifdef LINUX
#    include <GLee.h>
#    include <GL/glu.h>
#endif
#ifdef WIN32
#    define NOMINMAX
#	 include <windows.h>
#    include <GLee.h>
#    include <GL/glu.h>
#endif

#include <Vec.h>
#include <XForm.h>
#include <QtGlobal>

typedef int             int32;
typedef char            int8;
typedef unsigned int    uint32; // This could break on 64 bit, I guess.
typedef unsigned char   uint8;
typedef unsigned int    uint;
typedef Vec<4,int>	    vec4i;
typedef Vec<3,int>	    vec3i;
typedef Vec<2,int>	    vec2i;
typedef Vec<4,float>    vec4f;
typedef Vec<3,float>    vec3f;
typedef Vec<2,float>    vec2f;

#ifndef likely
#  if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#    define likely(x) (x)
#    define unlikely(x) (x)
#  else
#    define likely(x)   (__builtin_expect((x), 1))
#    define unlikely(x) (__builtin_expect((x), 0))
#  endif
#endif

template <class T>
static inline XForm<T> transpose(const XForm<T> &xf)
{
    return XForm<T>(xf[0], xf[4], xf[8],  xf[12],
                    xf[1], xf[5], xf[9],  xf[13],
                    xf[2], xf[6], xf[10], xf[14],
                    xf[3], xf[7], xf[11], xf[15]);
}

inline void reportGLError()
{
    GLint error = glGetError();
    if (error != 0)
    {
        qCritical("GL Error: %s\n", gluErrorString(error));
        qFatal("GL Error: %s\n", gluErrorString(error));
    }
}

#ifdef WIN32
#include <float.h>
#define isnan _isnan
#else
#include <cmath>
using std::isnan;
#endif

#endif // _GQ_INCLUDE_H_
