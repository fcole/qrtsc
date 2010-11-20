/*****************************************************************************\

GQMatlabArray.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2010 Forrester Cole

A basic wrapper for reading a single MATLAB format array, for example a 3D
texture. Uses matio for reading.

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "GQMatlabArray.h"
#include "GQInclude.h"

#include "matio.h"
#include "float.h"

GQMatlabArray::GQMatlabArray()
{
    _data = 0;
}

void GQMatlabArray::clear()
{
    delete _data;
    _data = 0;
}
    
unsigned char* GQMatlabArray::dataUint8() const 
{ 
    return (_gl_type == GL_UNSIGNED_BYTE) ? 
        reinterpret_cast<unsigned char*>(_data) :
        0;
}
    
unsigned int* GQMatlabArray::dataUint32() const 
{ 
    return (_gl_type == GL_UNSIGNED_INT) ? 
        reinterpret_cast<unsigned int*>(_data) :
        0;
}

float* GQMatlabArray::dataFloat() const
{ 
    return (_gl_type == GL_FLOAT) ? 
        reinterpret_cast<float*>(_data) :
        0;
}

double* GQMatlabArray::dataDouble() const 
{ 
    return (_gl_type == GL_DOUBLE) ? 
        reinterpret_cast<double*>(_data) :
        0;
}

bool GQMatlabArray::load(const QString& filename)
{
    mat_t* pmat = Mat_Open(qPrintable(filename), MAT_ACC_RDONLY);
    if (pmat == NULL)
    {
        qWarning("GQMatlabArray::load: could not open %s",
                qPrintable(filename));
        return false;
    }

    matvar_t* pa = Mat_VarReadNext(pmat);
    if (pa == NULL)
    {
        qWarning("GQMatlabArray::load: error reading %s",
                qPrintable(filename));
        return false;
    }

    if (Mat_Close(pmat) != 0)
    {
        qWarning("GQMatlabArray::load: error closing %s",
                qPrintable(filename));
        return false;
    }

    int num_dims = pa->rank;
    if (num_dims < 2 || num_dims > 3)
    {
        qWarning("GQMatlabArray::load: unsupported number of dimensions %d",
                (int)num_dims);
        Mat_VarFree(pa);
        return false;
    }

    _width = pa->dims[0];
    _height = pa->dims[1];
    if (num_dims > 2)
        _depth = pa->dims[2];
    else
        _depth = 1;

    matio_classes class_id = pa->class_type;
    switch (class_id)
    {
        case MAT_C_UINT8  : _gl_type = GL_UNSIGNED_BYTE; 
                              _type_size = 1; break;
        case MAT_C_UINT32 : _gl_type = GL_UNSIGNED_INT; 
                              _type_size = sizeof(unsigned int); 
                              break;
        case MAT_C_SINGLE : _gl_type = GL_FLOAT; 
                             _type_size = sizeof(float); 
                             break;
        case MAT_C_DOUBLE : _gl_type = GL_DOUBLE; 
                              _type_size = sizeof(double);
                              break;

        default:
            qWarning("GQMatlabArray::load: unsupported data type");
            Mat_VarFree(pa);
            return false;
    }
    
    try {
    	_data = new char[_type_size*_width*_height*_depth];
    } catch (std::bad_alloc&) {
        qWarning("GQMatlabArray::load: failed to allocate storage space (out of memory?)");
        Mat_VarFree(pa);
        return false;
    }

    memcpy(_data, pa->data, _type_size*_width*_height*_depth);
    Mat_VarFree(pa);

    return true;
}

void GQMatlabArray::convertToFloat()
{
    if (_gl_type == GL_FLOAT)
        return;

    float* new_data = new float[_width*_height*_depth];

    if (_gl_type == GL_DOUBLE)
    {
        double* old_data = dataDouble();
        for (int i = 0; i < _width*_height*_depth; i++)
            new_data[i] = float(old_data[i]);
    }
    else if (_gl_type == GL_UNSIGNED_INT)
    {
        unsigned int* old_data = dataUint32();
        for (int i = 0; i < _width*_height*_depth; i++)
            new_data[i] = float(old_data[i]);
    }
    else if (_gl_type == GL_UNSIGNED_BYTE)
    {
        unsigned char* old_data = dataUint8();
        for (int i = 0; i < _width*_height*_depth; i++)
            new_data[i] = float(old_data[i]);
    }

    _gl_type = GL_FLOAT;
    _type_size = sizeof(float);

    delete _data;
    _data = reinterpret_cast<char*>(new_data);
}

void GQMatlabArray::normalize()
{
    if (_gl_type == GL_DOUBLE)
    {
        double min = FLT_MAX;
        double max = -FLT_MAX;
        double* p = dataDouble();
        for (int i = 0; i < _width*_height*_depth; i++) 
        {
            if (p[i] < min)
                min = p[i];
            if (p[i] > max)
                max = p[i];
        }
        for (int i = 0; i < _width*_height*_depth; i++) 
            p[i] = (p[i] - min) / (max - min);
    }
    else if (_gl_type == GL_FLOAT)
    {
        float min = FLT_MAX;
        float max = -FLT_MAX;
        float* p = dataFloat();
        for (int i = 0; i < _width*_height*_depth; i++) 
        {
            if (p[i] < min)
                min = p[i];
            if (p[i] > max)
                max = p[i];
        }
        for (int i = 0; i < _width*_height*_depth; i++) 
            p[i] = (p[i] - min) / (max - min);
    }
}

