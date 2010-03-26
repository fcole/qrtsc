/*****************************************************************************\

GQTexture.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "GQTexture.h"
#include "GQInclude.h"
#include <assert.h>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include "GQMatlabArray.h"

GQTexture::GQTexture()
{
    _id = -1;
}

GQTexture::~GQTexture()
{
    clear();
}

void GQTexture::clear()
{
    if (_id > 0)
    {
        glDeleteTextures(1, (GLuint*)(&_id) );
    }
    _id = -1;
}
        
bool GQTexture2D::genTexture(int width, int height, int internal_format, 
                             int format, int type, const void *data)
{
    glGenTextures(1, (GLuint*)(&_id));
    glBindTexture(_target, _id);
    
    glTexImage2D(_target, 0, internal_format, width, height, 0, 
                 format, type, data);

    int wrapMode = _target == GL_TEXTURE_2D ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    int filterMode = _target == GL_TEXTURE_2D ? GL_LINEAR : GL_NEAREST;
    
    glTexParameteri(_target, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(_target, GL_TEXTURE_WRAP_T, wrapMode);
    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, filterMode);
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, filterMode);

    int error = glGetError();
    if (error)
    {
        qWarning("\nGQTexture2D::createGLTexture : GL error: %s\n", 
                gluErrorString(error));
        return false;
    } 

    return true;
}
        
bool GQTexture2D::load( const QString& filename )
{
    if (filename.endsWith(".pfm"))
    {
        GQFloatImage image;
        if (!image.load(filename))
            return false;
        
        return create(image, GL_TEXTURE_2D);
    }
    else
    {
        GQImage image;
        if (!image.load(filename))
            return false;
        
        return create(image, GL_TEXTURE_2D);
    }
}

bool GQTexture2D::create( const GQImage& image, int target )
{
    GLenum format;
    if (image.chan() == 1) 
        format = GL_ALPHA;
    else if (image.chan() == 2)
        format = GL_LUMINANCE_ALPHA;
    else if (image.chan() == 3) 
        format = GL_RGB;
    else if (image.chan() == 4)
        format = GL_RGBA;
    else {
        qCritical("GQTexture2D::create: unsupported number of channels %d",
                image.chan());
        return false;
    }

    return create(image.width(), image.height(), format, format, 
                  GL_UNSIGNED_BYTE, image.raster(), target);
}

bool GQTexture2D::create( const GQFloatImage& image, int target )
{
    GLenum format;
    if (image.chan() == 1)
        format = GL_ALPHA;
    else if (image.chan() == 2)
        format = GL_LUMINANCE_ALPHA;
    else if (image.chan() == 3) 
        format = GL_RGB;
    else if (image.chan() == 4)
        format = GL_RGBA;
    else {
        qCritical("GQTexture2D::create: unsupported number of channels %d",
                image.chan());
        return false;
    }

    GLenum internal_format = GL_RGBA32F_ARB;

    return create(image.width(), image.height(), internal_format, format, 
                  GL_FLOAT, image.raster(), target);
}

bool GQTexture2D::create(int width, int height, int internal_format, int format, 
                         int type, const void *data, int target)
{
    _target = target;
    _width = width;
    _height = height;
    
    return genTexture(width, height, internal_format, format, type, data);
}

bool GQTexture2D::bind() const
{
    glBindTexture(_target, _id);
    return true;
}

void GQTexture2D::unbind() const
{
    glBindTexture(_target, 0);
}

void GQTexture2D::enable() const
{
    glEnable(_target);
}

void GQTexture2D::disable() const
{
    glDisable(_target);
}

int GQTexture2D::target() const
{
    return _target;
}


bool GQTexture3D::load( const QString& filename )
{
    if (filename.endsWith(".mat"))
    {
        GQMatlabArray array;
        bool ret = array.load(filename);
        if (ret)
        {
            if (array.glType() == GL_DOUBLE)
                array.convertToFloat();

            array.normalize();

            GLenum format = GL_LUMINANCE;
            GLenum internal_format = GL_LUMINANCE;
            if (array.glType() == GL_FLOAT)
                internal_format = GL_LUMINANCE32F_ARB;

            return create(array.width(), array.height(), array.depth(),
                            internal_format, format, 
                            array.glType(), false, array.data());
        }
    }

    return false;
}

bool GQTexture3D::create(int width, int height, int depth, 
                         int internal_format, int format, 
                         int type, bool use_mipmaps, 
                         const void *data)
{
    _width = width;
    _height = height;
    _depth = depth;
    
    return genTexture(internal_format, format, type, 
                      use_mipmaps, data);
}

bool GQTexture3D::genTexture(int internal_format, int format, 
                             int type, bool use_mipmaps,
                             const void *data)
{
    int target = GL_TEXTURE_3D;
    int wrapMode = GL_REPEAT;
    int filterMagMode = GL_LINEAR;
    int filterMinMode = GL_LINEAR;

    glGenTextures(1, (GLuint*)(&_id));
    glBindTexture(target, _id);
    
    Q_UNUSED(use_mipmaps);
#if 0
    if (use_mipmaps)
    {
        int error = gluBuild3DMipmaps(target, internal_format, 
                           _width, _height, _depth, format, 
                           type, data);
        if (error)
        {
            qWarning("\nGQTexture3D::createGLTexture : gluBuild3DMipmaps error: %s\n", 
                    gluErrorString(error));
            return false;
        } 
        filterMinMode = GL_LINEAR_MIPMAP_LINEAR;
    }
    else
    {
        glTexImage3D(target, 0, internal_format, _width, _height, _depth, 0, 
                 format, type, data);
    }
#else
	Q_UNUSED(use_mipmaps);
	glTexImage3D(target, 0, internal_format, _width, _height, _depth, 0, 
                 format, type, data);
#endif
    
    glTexParameteri(target, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, wrapMode);
    glTexParameteri(target, GL_TEXTURE_WRAP_R, wrapMode);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filterMagMode);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filterMinMode);

    int error = glGetError();
    if (error)
    {
        qWarning("\nGQTexture3D::createGLTexture : GL error: %s\n", 
                gluErrorString(error));
        return false;
    } 

    return true;
}


bool GQTexture3D::bind() const
{
    glBindTexture(GL_TEXTURE_3D, _id);
    return true;
}

void GQTexture3D::unbind() const
{
    glBindTexture(GL_TEXTURE_3D, 0);
}

void GQTexture3D::enable() const
{
    glEnable(GL_TEXTURE_3D);
}

void GQTexture3D::disable() const
{
    glDisable(GL_TEXTURE_3D);
}

