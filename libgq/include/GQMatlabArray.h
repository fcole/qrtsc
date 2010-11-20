/*****************************************************************************\

GQMatlabArray.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2010 Forrester Cole

A basic wrapper for reading a single MATLAB format array, for example a 3D
texture. Uses matio for reading.

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _GQ_MATLAB_ARRAY_H_
#define _GQ_MATLAB_ARRAY_H_

#include <QString>

class GQMatlabArray
{
public:
    GQMatlabArray();
    ~GQMatlabArray()      { clear(); }

    int width()const      { return _width; }
    int height()const      { return _height; }
    int depth()const      { return _depth; }

    unsigned char* dataUint8() const;
    unsigned int* dataUint32() const;
    float* dataFloat() const;
    double* dataDouble() const;
    const char* data() const { return _data; }
    int glType() const { return _gl_type; }

    void convertToFloat();
    void normalize();

    void clear();

    bool load(const QString& filename);

private:
    int _width;
    int _height;
    int _depth;
    int _gl_type;
    int _type_size;
    char* _data;
};

#endif //_GQ_MATLAB_ARRAY_H_




