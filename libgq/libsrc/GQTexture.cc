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


GQTexture2D::GQTexture2D()
{
    _id = -1;
}
        
bool GQTexture2D::genTexture(int width, int height, int internalFormat, 
                             int format, int type, const void *data)
{
    glGenTextures(1, (GLuint*)(&_id));
    glBindTexture(_target, _id);
    
    glTexImage2D(_target, 0, internalFormat, width, height, 0, format, type, data);

    int wrapMode = _target == GL_TEXTURE_2D ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    int filterMode = _target == GL_TEXTURE_2D ? GL_LINEAR : GL_NEAREST;
    
    glTexParameteri(_target, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(_target, GL_TEXTURE_WRAP_T, wrapMode);
    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, filterMode);
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, filterMode);

    int error = glGetError();
    if (error)
    {
        printf("\nNPRTexture2D::createGLTexture : GL error: %s\n", gluErrorString(error));
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

    GLenum internal_format = GL_RGBA32F_ARB;

    return create(image.width(), image.height(), internal_format, format, 
                  GL_FLOAT, image.raster(), target);
}

bool GQTexture2D::create(int width, int height, int internalFormat, int format, 
                         int type, const void *data, int target)
{
    _target = target;
    _width = width;
    _height = height;
    
    return genTexture(width, height, internalFormat, format, type, data);
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
