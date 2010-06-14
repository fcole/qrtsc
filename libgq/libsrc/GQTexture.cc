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
#include <QStringList>
#include <QDir>
#include <QVector>
 

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
        
bool GQTexture2D::load( const QString& filename )
{
    if (filename.endsWith(".pfm") || filename.endsWith(".pbm"))
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
    
	glGenTextures(1, (GLuint*)(&_id));
    glBindTexture(_target, _id);
    
    int wrap_mode = (_target == GL_TEXTURE_2D) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    int filter_mag_mode = (_target == GL_TEXTURE_2D) ? GL_LINEAR : GL_NEAREST;
	int filter_min_mode = (_target == GL_TEXTURE_2D) ? GL_LINEAR : GL_NEAREST;
	
	glTexImage2D(_target, 0, internal_format, width, height, 0, 
				 format, type, data);
    
    glTexParameteri(_target, GL_TEXTURE_WRAP_S, wrap_mode);
    glTexParameteri(_target, GL_TEXTURE_WRAP_T, wrap_mode);
    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, filter_mag_mode);
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, filter_min_mode);
	
	glBindTexture(_target, 0);
	
    int error = glGetError();
    if (error)
    {
        qWarning("\nGQTexture2D::createGLTexture : GL error: %s\n", 
				 gluErrorString(error));
        return false;
    } 
	
    return true;
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

int GQTexture2D::target() const
{
    return _target;
}

void GQTexture2D::setMipmapping(bool enable) const
{
	int filter_min_mode;
	if (enable)
		filter_min_mode = GL_LINEAR_MIPMAP_LINEAR;
	else
		filter_min_mode = (_target == GL_TEXTURE_2D) ? GL_LINEAR : GL_NEAREST;
	
	bind();
	glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, filter_min_mode);
	unbind();
}

void GQTexture2D::setAnisotropicFiltering(bool enable) const
{
	GLfloat aniso = 1.0f;
	if (enable)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);

	bind();
	glTexParameterf(_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
	unbind();
}

void GQTexture2D::generateMipmaps()
{
	bind();
	glGenerateMipmap(_target);
	setMipmapping(true);
	unbind();
}

bool GQTexture3D::load( const QString& filename )
{
    if (filename.endsWith(".mat"))
    {
        return loadMat(filename);
    }
    else if (filename.endsWith(".3dt"))
    {
        return load3dt(filename);
    }
    else 
    {
        GQImage image;
        if (image.load(filename))
            return create(image);
    }
    return false;
}

bool GQTexture3D::loadMat( const QString& filename )
{
    GQMatlabArray array;
    if (!array.load(filename)) {
        qWarning("Could not open: %s", qPrintable(filename));
        return false;
    }

    if (array.glType() == GL_DOUBLE)
        array.convertToFloat();

    array.normalize();

    GLenum format = GL_LUMINANCE;
    GLenum internal_format = GL_LUMINANCE;
    if (array.glType() == GL_FLOAT)
        internal_format = GL_LUMINANCE32F_ARB;

    return create(array.width(), array.height(), array.depth(),
                    internal_format, format, 
                    array.glType(), array.data());
}

bool GQTexture3D::load3dt( const QString& filename )
{
    QFile header_file(filename);
    if (!header_file.open(QFile::ReadOnly)) {
        qWarning("Could not open: %s", qPrintable(filename));
        return false;
    }

    QTextStream header(&header_file);
    int depth;
    QStringList slice_names;
    header >> depth;
    QDir dir = QFileInfo(filename).dir();
    for (int i = 0; i < depth; i++)
    {
        QString slice;
        header >> slice;
        slice_names << dir.absoluteFilePath(slice);
    }

    int width, height, channels;
    QVector<uint8> data;
    GQImage first;
    if (!first.load(slice_names[0])) {
        qWarning("Could not open: %s", qPrintable(slice_names[0]));
        return false;
    }

    width = first.width();
    height = first.height();
    channels = first.chan();
    data.reserve(width*height*depth*channels);

    for (int i = 0; i < width*height*channels; i++)
        data.push_back(first.raster()[i]);

    for (int j = 1; j < depth; j++) {
        GQImage img;
        if (!img.load(slice_names[j])) {
            qWarning("Could not open: %s", qPrintable(slice_names[j]));
            return false;
        }
        for (int i = 0; i < width*height*channels; i++)
            data.push_back(img.raster()[i]);
    }

    GLint format;
    if (channels == 1) 
        format = GL_LUMINANCE;
    else if (channels == 2)
        format = GL_LUMINANCE_ALPHA;
    else if (channels == 3) 
        format = GL_RGB;
    else if (channels == 4)
        format = GL_RGBA;
    else
        assert(0);

    return create(width, height, depth,
                  format, format, 
                  GL_UNSIGNED_BYTE, data.constData());
}

bool GQTexture3D::create(const GQImage& image)
{
    GLint format;
    if (image.chan() == 1) 
        format = GL_LUMINANCE;
    else if (image.chan() == 2)
        format = GL_LUMINANCE_ALPHA;
    else if (image.chan() == 3) 
        format = GL_RGB;
    else if (image.chan() == 4)
        format = GL_RGBA;
    else
        assert(0);
 
    return create(image.width(), image.height(), 1,
                 format, format, GL_UNSIGNED_BYTE, image.raster());
}

bool GQTexture3D::create(int width, int height, int depth, 
                         int internal_format, int format, 
                         int type, const void *data)
{
    _width = width;
    _height = height;
    _depth = depth;
    
    return genTexture(internal_format, format, type, data);
}

bool GQTexture3D::genTexture(int internal_format, int format, 
                             int type, const void *data)
{
    int target = GL_TEXTURE_3D;
    int wrap_mode = GL_REPEAT;
    int filter_mag_mode = GL_LINEAR;
    int filter_min_mode = GL_LINEAR;

    glGenTextures(1, (GLuint*)(&_id));
    glBindTexture(target, _id);
    
	glTexImage3D(target, 0, internal_format, _width, _height, _depth, 0, 
                 format, type, data);
    
    glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap_mode);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap_mode);
    glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter_mag_mode);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter_min_mode);
	
    glBindTexture(target, 0);

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

void GQTexture3D::setMipmapping(bool enable) const
{
	int filter_min_mode;
	if (enable)
		filter_min_mode = GL_LINEAR_MIPMAP_LINEAR;
	else
		filter_min_mode = GL_LINEAR;
	
	bind();
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filter_min_mode);
	unbind();
}

void GQTexture3D::setAnisotropicFiltering(bool enable) const
{
	GLfloat aniso = 1.0f;
	if (enable)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
	
	bind();
	glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
	unbind();
}

void GQTexture3D::generateMipmaps()
{
	bind();
	glGenerateMipmap(GL_TEXTURE_3D);
	setMipmapping(true);
	unbind();
}
