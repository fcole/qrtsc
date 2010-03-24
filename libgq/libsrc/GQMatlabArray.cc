#include "GQMatlabArray.h"
#include "GQInclude.h"

#ifdef GQ_LINK_MATLAB
#   include "matrix.h"
#   include "mat.h"
#else
#   include "float.h"
#endif

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
#ifdef GQ_LINK_MATLAB
    MATFile* pmat = matOpen(qPrintable(filename), "r");
    if (pmat == NULL)
    {
        qWarning("GQMatlabArray::load: could not open %s",
                qPrintable(filename));
        return false;
    }

    const char* name;
    mxArray* pa = matGetNextVariable(pmat, &name);
    if (pa == NULL)
    {
        qWarning("GQMatlabArray::load: error reading %s",
                qPrintable(filename));
        return false;
    }

    mwSize num_dims = mxGetNumberOfDimensions(pa);
    if (num_dims < 2 || num_dims > 3)
    {
        qWarning("GQMatlabArray::load: unsupported number of dimensions %d",
                (int)num_dims);
        mxDestroyArray(pa);
        return false;
    }

    const mwSize* dims = mxGetDimensions(pa);
    _width = dims[0];
    _height = dims[1];
    if (num_dims > 2)
        _depth = dims[2];
    else
        _depth = 1;

    mxClassID class_id = mxGetClassID(pa);
    switch (class_id)
    {
        case mxUINT8_CLASS  : _gl_type = GL_UNSIGNED_BYTE; 
                              _type_size = 1; break;
        case mxUINT32_CLASS : _gl_type = GL_UNSIGNED_INT; 
                              _type_size = sizeof(unsigned int); 
                              break;
        case mxSINGLE_CLASS : _gl_type = GL_FLOAT; 
                             _type_size = sizeof(float); 
                             break;
        case mxDOUBLE_CLASS : _gl_type = GL_DOUBLE; 
                              _type_size = sizeof(double);
                              break;
        default:
            qWarning("GQMatlabArray::load: unsupported data type");
            mxDestroyArray(pa);
            return false;
    }
    
	_data = new char[_type_size*_width*_height*_depth];
    memcpy(_data, mxGetData(pa), _type_size*_width*_height*_depth);

    mxDestroyArray(pa);
    if (matClose(pmat) != 0)
    {
        qWarning("GQMatlabArray::load: error closing %s",
                qPrintable(filename));
        return false;
    }

    return true;
#else
    Q_UNUSED(filename);
	return false;
#endif
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

