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

int                 GQVertexBufferSet::_last_used_guid = 1;
QHash<int,int>      GQVertexBufferSet::_bound_guids;
QHash<QString,int>  GQVertexBufferSet::_bound_buffers;

GLenum kGlArrays[GQ_NUM_VERTEX_BUFFER_TYPES] = { GL_VERTEX_ARRAY, 
                                       GL_NORMAL_ARRAY, 
                                       GL_COLOR_ARRAY,
                                       GL_TEXTURE_COORD_ARRAY,
									   0, // GQ_INDEX 
									   };

/*void handleGLError()
{
    GLint error = glGetError();
    if (error != 0)
    {
        qCritical("GQVertexBufferSet::handleGLError: %s\n", gluErrorString(error));
        qFatal("GQVertexBufferSet::handleGLError: %s\n", gluErrorString(error));
    }
}*/


void GQVertexBufferSet::clear()
{
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
							const QVector<int>& data )
{
    QString name = GQVertexBufferNames[semantic];
    add(name, width, data);
    _buffer_hash[name]->_semantic = semantic;
}

void GQVertexBufferSet::add( GQVertexBufferType semantic, int width, 
							const std::vector<int>& data )
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
                            const QVector<vec>& data )
{
    QString name = GQVertexBufferNames[semantic];
    add(name, data);
    _buffer_hash[name]->_semantic = semantic;
}

void GQVertexBufferSet::add( GQVertexBufferType semantic, 
                            const std::vector<vec>& data )
{
    QString name = GQVertexBufferNames[semantic];
    add(name, data);
    _buffer_hash[name]->_semantic = semantic;
}

void GQVertexBufferSet::add( GQVertexBufferType semantic, 
                            const std::vector<vec2>& data )
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
							const QVector<int>& data )
{
    BufferInfo newinfo;
    newinfo.init(name, _gl_usage_mode, GL_INT, 
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

void GQVertexBufferSet::add(const QString& name, int width, 
							const std::vector<int>& data )
{
	BufferInfo newinfo;
    newinfo.init(name, _gl_usage_mode, GL_INT, 
				 width, data.size() / width, 
				 reinterpret_cast<const uint8*>(&data[0]));
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
                            const QVector<vec>& data )
{
    BufferInfo newinfo;
    int width = 3;
    int length = data.size();
    newinfo.init(name, _gl_usage_mode, GL_FLOAT, width, length, 
        reinterpret_cast<const uint8*>(data.constData()));
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

void GQVertexBufferSet::add(const QString& name,
                            const std::vector<vec2>& data )
{
    BufferInfo newinfo;
    int width = 2;
    int length = data.size();
    newinfo.init(name, _gl_usage_mode, GL_FLOAT, width, length, 
				 reinterpret_cast<const uint8*>(&data[0]));
    add(newinfo);
}

void GQVertexBufferSet::add( const BufferInfo& buffer_info )
{
    // If a buffer with this name already exists, add
    // replaces it.
    if (_buffer_hash.contains(buffer_info._name))
    {
        for (int i = 0; i < _buffers.size(); i++)
        {
            if (&_buffers[i] == _buffer_hash[buffer_info._name])
            {
                _buffers.removeAt(i);
                break;
            }
        }
    }
    _buffers.push_back(buffer_info);
    _buffer_hash[buffer_info._name] = &(_buffers.last());
}

void GQVertexBufferSet::remove( const BufferInfo& buffer_info )
{
    if (_buffer_hash.contains(buffer_info._name))
    {
        for (int i = 0; i < _buffers.size(); i++)
        {
            if (&_buffers[i] == _buffer_hash[buffer_info._name])
            {
                _buffers.removeAt(i);
                break;
            }
        }
    }
}
        
void GQVertexBufferSet::setUsageMode(GQVertexBufferUsage usage_mode)
{
    assert(!_bound_guids.contains(_guid));

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

void GQVertexBufferSet::copyToVBOs()
{
    for (int i = 0; i < _buffers.size(); i++)
    {
        BufferInfo& buf = _buffers[i];

        if (buf._vbo_id < 0)
        {
            GLuint id;
            glGenBuffers(1, &id);
			// Check for a fishy buffer id. Mac driver sometimes
			// returns crap values without raising a GL error.
			if (id == 0 || id > 1000000)
				qWarning("GQVertexBufferSet::copyToVBOs: possibly bogus buffer id: %d", id);
            buf._vbo_id = (int)id;
        }
        if (buf._vbo_size <= 0)
        {
            buf._vbo_size = buf.dataSize();
        }
		int target = GL_ARRAY_BUFFER;
		if (buf._semantic == GQ_INDEX)
			target = GL_ELEMENT_ARRAY_BUFFER;
			
        glBindBuffer(target, (GLuint)(buf._vbo_id));
        glBufferData(target, buf._vbo_size, 
                     buf.dataPointer(), buf._gl_usage_mode);
        glBindBuffer(target, 0);
    }
    reportGLError();
}

void GQVertexBufferSet::deleteVBOs()
{
    for (int i = 0; i < _buffers.size(); i++)
    {
        _buffers[i].deleteVBO();
    }
    reportGLError();
}

bool GQVertexBufferSet::vbosLoaded() const
{
    if (_buffers.size() > 0)
        return _buffers[0]._vbo_id >= 0;
    return false;
}


void GQVertexBufferSet::bind() const
{
    assert(!_bound_guids.contains(_guid));

    for (int i = 0; i < _buffers.size(); i++)
    {
        if (_buffers[i]._semantic < GQ_NUM_VERTEX_BUFFER_TYPES)
        {
            bindBuffer(_buffers[i], -1);
        }
    }
    _bound_guids[_guid] = 1;
    reportGLError();
}

void GQVertexBufferSet::bind( const GQShaderRef& current_shader ) const
{
    assert(!_bound_guids.contains(_guid));

    for (int i = 0; i < _buffers.size(); i++)
    {
		// Don't use GL_ELEMENT_ARRAY_BUFFER when using client side draw arrays.
		if (_buffers[i]._semantic == GQ_INDEX && _buffers[i]._vbo_id < 0)
			continue;
		
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
            }
#ifdef GQ_DEBUGGING_LEVEL_VERBOSE
            else
            {
                qCritical("GQVertexBufferSet::bind: Could not find matching attribute for %s.\n", qPrintable(_buffers[i]._name));
            }
#endif
        }
        reportGLError();
    }
    _bound_guids[_guid] = 1;
    reportGLError();
}


void GQVertexBufferSet::unbind() const
{
    assert(_bound_guids.contains(_guid));

    for (int i = 0; i < _buffers.size(); i++)
    {
        if (_bound_buffers.contains(_buffers[i]._name))
        {
            unbindBuffer(_buffers[i]);
        }
    }

    reportGLError();

    _bound_guids.remove(_guid);
}


void GQVertexBufferSet::bindBuffer( const BufferInfo& info, int attrib ) const 
{
	int stride = info._type_size * info._width * _element_stride;
	int offset = stride * _starting_element;
	
	int bind_target = GL_ARRAY_BUFFER;
	if (info._semantic == GQ_INDEX)
	{
		assert(info._vbo_id >= 0);
		bind_target = GL_ELEMENT_ARRAY_BUFFER;
	}

    const uint8* datap;	
    if (info._vbo_id >= 0)
    {
        glBindBuffer(bind_target, info._vbo_id);
		datap = reinterpret_cast<const uint8*>(offset);
    }
    else
    {
		glBindBuffer(bind_target, 0);
        datap = info.dataPointer() + offset;	
    }

    switch (info._semantic)
    {
        case GQ_VERTEX : 
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(info._width, info._data_type, stride, 
                            (const void*)(datap));
            break;
        case GQ_NORMAL : 
            glEnableClientState(GL_NORMAL_ARRAY);
            glNormalPointer(info._data_type, stride, 
                            (const void*)(datap));
            break;
        case GQ_COLOR : 
            glEnableClientState(GL_COLOR_ARRAY);
            glColorPointer(info._width, info._data_type, stride, 
                           (const void*)(datap));
            break;
        case GQ_TEXCOORD : 
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(info._width, info._data_type, stride, 
                              (const void*)(datap));
            break;
		case GQ_INDEX :
			// No client state needs to change here.
			break;
        default:
            assert(attrib >= 0);
            glEnableVertexAttribArray(attrib);
            glVertexAttribPointer(attrib, info._width, info._data_type, 
                                  info._normalize, stride, 
                                  (const void*)(datap));
            break;
    }

	// Have to leave some VBO bound here (on mac),
	// even if we have bound and set multiple VBOs or glDraw*
	// will crash.
	// glBindBuffer(bind_target, 0);

    _bound_buffers[info._name] = attrib;
}

void GQVertexBufferSet::unbindBuffer( const BufferInfo& info ) const
{
    int attrib = _bound_buffers[info._name];
    if (attrib < 0)
    {
		int client_state = kGlArrays[info._semantic];
		if (client_state > 0)
			glDisableClientState(client_state);
    }
    else
    {
        glDisableVertexAttribArray(attrib);
    }
	
	int target = GL_ARRAY_BUFFER;
	if (info._semantic == GQ_INDEX)
		target = GL_ELEMENT_ARRAY_BUFFER;
	glBindBuffer(target, 0);
    
	_bound_buffers.remove(info._name);
}

void GQVertexBufferSet::copyFromFBO(GQVertexBufferType vbo_semantic,
									const GQFramebufferObject& fbo, 
                                    int fbo_buffer)
{
    copyFromFBO(GQVertexBufferNames[vbo_semantic], fbo, fbo_buffer);
}

void GQVertexBufferSet::copyFromFBO(const QString& vbo_name,
									const GQFramebufferObject& fbo, 
                                    int fbo_buffer)
{
    copyFromSubFBO(vbo_name, fbo, fbo_buffer, 0, 0, fbo.width(), fbo.height());
}
        
void GQVertexBufferSet::copyFromSubFBO(GQVertexBufferType vbo_semantic,
									   const GQFramebufferObject& fbo, 
                                       int fbo_buffer, int x, int y, 
                                       int width, int height)
{
    copyFromSubFBO(GQVertexBufferNames[vbo_semantic], fbo, fbo_buffer, 
				   x, y, width, height);
}

void GQVertexBufferSet::copyFromSubFBO(const QString& vbo_name,
									   const GQFramebufferObject& fbo, 
                                       int fbo_buffer, int x, int y, 
                                       int width, int height)
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
		case GL_INT				: _type_size = sizeof(GLint); break;
        case GL_UNSIGNED_BYTE   : _type_size = 1; break;
        case GL_UNSIGNED_INT    : _type_size = sizeof(GLuint); break;
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

void GQVertexBufferSet::BufferInfo::deleteVBO()
{
    if (_vbo_id >= 0)
    {
        glDeleteBuffers(1, (GLuint*)(&_vbo_id));
    }
    _vbo_id = -1;
}