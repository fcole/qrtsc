/*****************************************************************************\

GQVertexBufferSet.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "GQInclude.h"
#include "GQVertexBufferSet.h"
#include <assert.h>

bool          GQVertexBufferSet::_gl_buffer_object_bound = false;
int           GQVertexBufferSet::_last_used_guid = 1;
int           GQVertexBufferSet::_bound_guid = 0;
QList<int>    GQVertexBufferSet::_bound_attribs;

GLenum kGlArrays[GQ_NUM_VERTEX_BUFFER_TYPES] = { GL_VERTEX_ARRAY, 
                                       GL_NORMAL_ARRAY, 
                                       GL_COLOR_ARRAY,
                                       GL_TEXTURE_COORD_ARRAY };

void handleGLError()
{
    GLint error = glGetError();
    if (error != 0)
    {
        qCritical("GQVertexBufferSet::handleGLError: %s\n", gluErrorString(error));
        qFatal("GQVertexBufferSet::handleGLError: %s\n", gluErrorString(error));
    }
}


void GQVertexBufferSet::clear()
{
    assert(_bound_guid == 0);

    if (_buffers.size() > 0)
        deleteVBOs();

    _starting_element = 0;
    _element_stride = 0;
    _guid = _last_used_guid++;
    _gl_usage_mode = GL_STATIC_DRAW;

    if (_guid == 0) _guid = _last_used_guid++;

    _buffer_hash.clear();
    _buffers.clear();
}

void GQVertexBufferSet::add( GQVertexBufferType semantic, int width, 
                             const QVector<float>& data )
{
    QString name = GQVertexBufferNames[semantic];
    add(name, width, data);
    _buffer_hash[name]->_semantic = semantic;
}

void GQVertexBufferSet::add( GQVertexBufferType semantic, int width, 
                             const QVector<uint8>& data )
{
    QString name = GQVertexBufferNames[semantic];
    add(name, width, data);
    _buffer_hash[name]->_semantic = semantic;
}

void GQVertexBufferSet::add( GQVertexBufferType semantic, int width, 
                             int format, int length )
{
    QString name = GQVertexBufferNames[semantic];
    add(name, width, format, length);
    _buffer_hash[name]->_semantic = semantic;
}

void GQVertexBufferSet::add( GQVertexBufferType semantic, 
                            const std::vector<vec>& data )
{
    QString name = GQVertexBufferNames[semantic];
    add(name, data);
    _buffer_hash[name]->_semantic = semantic;
}

void GQVertexBufferSet::add( const QString& name, int width, 
                             const QVector<float>& data )
{
    BufferInfo newinfo;
    newinfo.init(name, _gl_usage_mode, GL_FLOAT, 
        width, data.size() / width, 
        reinterpret_cast<const uint8*>(data.constData()));
    add(newinfo);
}

void GQVertexBufferSet::add( const QString& name, int width, 
                             const QVector<uint8>& data )
{
    BufferInfo newinfo;
    newinfo.init(name, _gl_usage_mode, GL_UNSIGNED_BYTE, 
        width, data.size() / width, 
        reinterpret_cast<const uint8*>(data.constData()));
    add(newinfo);
}

void GQVertexBufferSet::add( const QString& name, int width, 
                             int format, int length )
{
    BufferInfo newinfo;
    newinfo.init(name, _gl_usage_mode, format, width, length, 0);
    add(newinfo);
}

void GQVertexBufferSet::add(const QString& name,
                            const std::vector<vec>& data )
{
    BufferInfo newinfo;
    int width = 3;
    int length = data.size();
    newinfo.init(name, _gl_usage_mode, GL_FLOAT, width, length, 
        reinterpret_cast<const uint8*>(&data[0]));
    add(newinfo);
}

void GQVertexBufferSet::add( const BufferInfo& buffer_info )
{
    _buffers.push_back(buffer_info);
    _buffer_hash[buffer_info._name] = &(_buffers.last());
}
        
void GQVertexBufferSet::setUsageMode(GQVertexBufferUsage usage_mode)
{
    assert(_bound_guid != _guid);

    switch (usage_mode)
    {
        case GQ_STATIC_DRAW: _gl_usage_mode = GL_STATIC_DRAW; break;
        case GQ_DYNAMIC_DRAW: _gl_usage_mode = GL_DYNAMIC_DRAW; break;
        case GQ_STREAM_DRAW: _gl_usage_mode = GL_STREAM_DRAW; break;
        default: assert(0); break;
    }
    for (int i = 0; i < _buffers.size(); i++)
    {
        _buffers[i]._gl_usage_mode = _gl_usage_mode;
    }
}

int GQVertexBufferSet::vboId( GQVertexBufferType semantic ) const
{
    BufferInfo* info = _buffer_hash.value(GQVertexBufferNames[semantic], 0);
    if (info)
        return info->_vbo_id;
    
    return -1;
}

int GQVertexBufferSet::vboId( const QString& name ) const
{
    BufferInfo* info = _buffer_hash.value(name, 0);
    if (info)
        return info->_vbo_id;
    
    return -1;
}

void GQVertexBufferSet::copyDataToVBOs()
{
    for (int i = 0; i < _buffers.size(); i++)
    {
        BufferInfo& buf = _buffers[i];

        if (buf._vbo_id < 0)
        {
            GLuint id;
            glGenBuffers(1, &id);
            buf._vbo_id = (int)id;
        }
        if (buf._vbo_size <= 0)
        {
            buf._vbo_size = buf.dataSize();
        }
        glBindBuffer(GL_ARRAY_BUFFER, (GLuint)(buf._vbo_id));
        glBufferData(GL_ARRAY_BUFFER, buf._vbo_size, 
                     buf.dataPointer(), buf._gl_usage_mode);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    handleGLError();
}

void GQVertexBufferSet::deleteVBOs()
{
    for (int i = 0; i < _buffers.size(); i++)
    {
        if (_buffers[i]._vbo_id >= 0)
        {
            glDeleteBuffers(1, (GLuint*)(&(_buffers[i]._vbo_id)));
        }
        _buffers[i]._vbo_id = -1;
    }
    handleGLError();
}

bool GQVertexBufferSet::vbosLoaded() const
{
    if (_buffers.size() > 0)
        return _buffers[0]._vbo_id >= 0;
    return false;
}


void GQVertexBufferSet::bind() const
{
    assert(_bound_guid == 0);

    for (int i = 0; i < _buffers.size(); i++)
    {
        if (_buffers[i]._semantic < GQ_NUM_VERTEX_BUFFER_TYPES)
        {
            bindBuffer(_buffers[i], -1);
        }
    }
    _bound_guid = _guid;
    reportGLError();
}

void GQVertexBufferSet::bind( const GQShaderRef& current_shader ) const
{
    assert(_bound_guid == 0);

    for (int i = 0; i < _buffers.size(); i++)
    {
        if (_buffers[i]._semantic < GQ_NUM_VERTEX_BUFFER_TYPES)
        {
            bindBuffer(_buffers[i], -1);
        }
        else
        {
            int attrib_loc = current_shader.attribLocation(_buffers[i]._name);
            if (attrib_loc >= 0)
            {
                bindBuffer(_buffers[i], attrib_loc);
                _bound_attribs.push_back(attrib_loc);
            }
            else
            {
                qCritical("GQVertexBufferSet::bind: Could not find matching attribute for %s.\n", qPrintable(_buffers[i]._name));
            }
        }
        reportGLError();
    }
    _bound_guid = _guid;
    reportGLError();
}


void GQVertexBufferSet::unbind() const
{
    assert(_bound_guid == _guid);

    if (_gl_buffer_object_bound)
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        _gl_buffer_object_bound = false;
    }

    for (int i = 0; i < _buffers.size(); i++)
    {
        if (_buffers[i]._semantic < GQ_NUM_VERTEX_BUFFER_TYPES)
        {
            glDisableClientState(kGlArrays[_buffers[i]._semantic]);
        }
    }
    for (int i = 0; i < _bound_attribs.size(); i++)
    {
        glDisableVertexAttribArray(_bound_attribs[i]);
    }

    _bound_attribs.clear();

    reportGLError();

    _bound_guid = 0;
}


void GQVertexBufferSet::bindBuffer( const BufferInfo& info, int attrib ) const 
{
    const uint8* datap = 0;
    if (info._vbo_id >= 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, info._vbo_id);
        _gl_buffer_object_bound = true;
    }
    else
    {
        datap = info.dataPointer();
        if (_gl_buffer_object_bound)
        {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            _gl_buffer_object_bound = false;
        }
    }

    int stride = info._type_size * info._width * _element_stride;
    int offset = stride * _starting_element;

    switch (info._semantic)
    {
        case GQ_VERTEX : 
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(info._width, info._data_type, stride, 
                            (const void*)(datap + offset));
            break;
        case GQ_NORMAL : 
            glEnableClientState(GL_NORMAL_ARRAY);
            glNormalPointer(info._data_type, stride, 
                            (const void*)(datap + offset));
            break;
        case GQ_COLOR : 
            glEnableClientState(GL_COLOR_ARRAY);
            glColorPointer(info._width, info._data_type, stride, 
                           (const void*)(datap + offset));
            break;
        case GQ_TEXCOORD : 
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(info._width, info._data_type, stride, 
                              (const void*)(datap + offset));
            break;
        default:
            assert(attrib >= 0);
            glEnableVertexAttribArray(attrib);
            glVertexAttribPointer(attrib, info._width, info._data_type, 
                                  info._normalize, stride, 
                                  (const void*)(datap + offset));
            break;
    }
}

void GQVertexBufferSet::copyFromFBO(const GQFramebufferObject& fbo, 
                                    int fbo_buffer, 
                                    GQVertexBufferType vbo_semantic )
{
    copyFromFBO(fbo, fbo_buffer, GQVertexBufferNames[vbo_semantic]);
}

void GQVertexBufferSet::copyFromFBO(const GQFramebufferObject& fbo, 
                                    int fbo_buffer, 
                                    const QString& vbo_name )
{
    copyFromSubFBO(fbo, fbo_buffer, 0, 0, fbo.width(), fbo.height(), vbo_name);
}
        
void GQVertexBufferSet::copyFromSubFBO(const GQFramebufferObject& fbo, 
                                       int fbo_buffer, int x, int y, 
                                       int width, int height, 
                                       GQVertexBufferType vbo_semantic )
{
    copyFromSubFBO(fbo, fbo_buffer, x, y, width, height, 
                   GQVertexBufferNames[vbo_semantic]);
}

void GQVertexBufferSet::copyFromSubFBO(const GQFramebufferObject& fbo, 
                                       int fbo_buffer, int x, int y, 
                                       int width, int height, 
                                       const QString& vbo_name )
{
    assert(fbo.isBound());
    if (!fbo.isBound())
        return;

    BufferInfo* info = _buffer_hash.value(vbo_name, 0);
    assert(info);
    if (!info)
        return;

    assert(info->_vbo_id >= 0);
    assert(info->_width == 3 || info->_width == 4);
    assert(info->_width * width * height <= info->_vbo_size);

    int data_type = info->_data_type;
    int format;
    if (info->_width == 3)
        format = GL_RGB;
    else
        format = GL_RGBA;

    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, info->_vbo_id);
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT + fbo_buffer);
    glReadPixels(x, y, width, height, format, data_type, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);

    reportGLError();
}

void GQVertexBufferSet::BufferInfo::init(const QString& name, int usage_mode, 
                                         int data_type, int width, int length,
                                         const uint8* data_pointer)
{
    _name = name;
    _semantic = GQ_NUM_VERTEX_BUFFER_TYPES;
    for (int i = 0; i < GQ_NUM_VERTEX_BUFFER_TYPES; i++)
    {
        if (GQVertexBufferNames[i] == _name)
            _semantic = (GQVertexBufferType)i;
    }
    _width = width;
    _length = length;
    _data_type = data_type;
    switch (data_type) {
        case GL_FLOAT           : _type_size = sizeof(GLfloat); break;
        case GL_DOUBLE          : _type_size = sizeof(GLdouble); break;
        case GL_UNSIGNED_BYTE   : _type_size = 1; break;
        case GL_UNSIGNED_INT    : _type_size = sizeof(GLint); break;
        default: 
            qFatal("GQVertexBufferSet::BufferInfo::init: unexpected data type"); 
            break;
    }
    _gl_usage_mode = usage_mode;
    _vbo_id = -1;
    _vbo_size = length * width * _type_size;
    _normalize = false;

    _data_pointer = data_pointer;
}

const uint8* GQVertexBufferSet::BufferInfo::dataPointer() const
{
    return _data_pointer;
}
            
int GQVertexBufferSet::BufferInfo::dataSize() const
{
    if (_data_pointer)
        return _type_size * _width * _length;
    else 
        return 0;
}
