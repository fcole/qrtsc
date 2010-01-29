/*****************************************************************************\

GQFramebufferObject.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

A class for managing OpenGL framebuffer objects.

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef GQ_FRAMEBUFFER_OBJECT_H_
#define GQ_FRAMEBUFFER_OBJECT_H_

#include "GQImage.h"
#include "GQTexture.h"
#include <QString>

const uint32 GQ_ATTACH_NONE = 0x0;
const uint32 GQ_ATTACH_DEPTH = 0x1;

const uint32 GQ_COORDS_PIXEL = 0x0;
const uint32 GQ_COORDS_NORMALIZED = 0x1;

const uint32 GQ_FORMAT_RGBA_FLOAT = 0x0;
const uint32 GQ_FORMAT_RGBA_BYTE = 0x1;

class GQFramebufferObject
{
public:
    GQFramebufferObject();
    ~GQFramebufferObject();

    void clear();

    bool init(int width, int height, int num_color_attachments = 1,
              uint32 extra_attachments = GQ_ATTACH_NONE, 
              uint32 coordinates = GQ_COORDS_PIXEL,
              uint32 format = GQ_FORMAT_RGBA_FLOAT);
            
    bool initFullScreen(int num_color_attachments = 1,
              uint32 extra_attachments = GQ_ATTACH_NONE, 
              uint32 coordinates = GQ_COORDS_PIXEL,
              uint32 format = GQ_FORMAT_RGBA_FLOAT);
            
    bool initGL( int target, int format, uint32 attachments, 
        int num_color_attachments, int width, int height );

    void bind() const;
    void unbind() const;
    bool isBound() const { return _bound_guid == _guid; }

    void drawBuffer( int which );
    void drawToAllBuffers();

    int  id() const { return _fbo; }
    int  depthBufferId() const { return _depth_attachment; }

    GQTexture2D* colorTexture( int which );
    const GQTexture2D* colorTexture( int which ) const;
    int  numColorAttachments() const { return _num_color_attachments; }

    int  width() const { return _width; }
    int  height() const { return _height; }
    int  coordinates() const { return _coordinates; }
    int  format() const { return _format; }
    int  glTarget() const { return _gl_target; }
    int  glFormat() const { return _gl_format; }

    void setTexParameteri(unsigned int param, int value);
    void setTexParameteriv(unsigned int param, int* value);
    void setTexParameterf(unsigned int param, float value);
    void setTexParameterfv(unsigned int param, float* value);

    void setTextureFilter(int filter_min, int filter_mag);
    void setTextureWrap(int wrap_s, int wrap_t);

    void readColorTexturei( int which, GQImage& image, int num_channels = 4 ) const;
    void readColorTexturef( int which, GQFloatImage& image, int num_channels = 4 ) const;
    void readSubColorTexturei( int which, int x, int y, int width, int height, 
                              GQImage& image, int num_channels = 4 ) const;
    void readSubColorTexturef( int which, int x, int y, int width, int height, 
                              GQFloatImage& image, int num_channels = 4) const;
    void readDepthBuffer( GQFloatImage& image ) const;
    void readSubDepthBuffer( int x, int y, int width, int height, 
                             GQFloatImage& image ) const;

    void loadColorTexturei( int which, const GQImage& image );
    void loadColorTexturef( int which, const GQFloatImage& image );

    void saveColorTextureToFile( int which, const QString& filename ) const;
    void saveDepthBufferToFile( const QString& filename ) const;

    static int maxFramebufferSize();
    static int maxColorAttachments();

protected:
    int _fbo;
    int _coordinates;
    int _format;
    int _gl_target;
    int _gl_format;
    int _guid;
    static int _bound_guid;
    static int _last_used_guid;

    int _width, _height;

    uint32 _attachments;

    int _num_color_attachments;
    GQTexture2D** _color_attachments;

    int	_depth_attachment;


};

// This is an alternate FBO class for when you need more than
// getMaxAttachments() buffers, but not multiple render targets.
// It binds a single color texture to the FBO prior to rendering.

class GQSingleFBO : public GQFramebufferObject
{
public:
    bool init( int target, int format, uint32 attachments, 
        int num_color_attachments, int width, int height );

    void drawBuffer( int which );
    void drawToAllBuffers();

protected:
    int _current_color_attachment;
};
#endif
