/*****************************************************************************\

GQVertexBufferSet.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

An abstraction to manage OpenGL vertex arrays and vertex buffer objects. 
A buffer set is stored on the CPU side by default, but may be copied to
the GPU using copyDataToVBOs().

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _GQ_VERTEX_BUFFER_SET_H
#define _GQ_VERTEX_BUFFER_SET_H

#include <QVector>
#include <QHash>
#include <QList>

#include "GQShaderManager.h"
#include "GQFramebufferObject.h"

enum GQVertexBufferType 
{ 
    GQ_VERTEX, 
    GQ_NORMAL, 
    GQ_COLOR, 
    GQ_TEXCOORD, 
    GQ_NUM_VERTEX_BUFFER_TYPES 
};

const QString GQVertexBufferNames[GQ_NUM_VERTEX_BUFFER_TYPES] = 
    { "vertex", "normal", "color", "texcoord" }; 

enum GQVertexBufferUsage 
{ 
    GQ_STATIC_DRAW, 
    GQ_DYNAMIC_DRAW, 
    GQ_STREAM_DRAW, 
    GQ_NUM_VERTEX_BUFFER_USAGE 
};

class GQVertexBufferSet
{
    public:
        GQVertexBufferSet() { clear(); }
        ~GQVertexBufferSet() { clear(); }

        void clear();

        // Note: these calls do *not* copy the data out of the sources unless
        // copyDataToVBOs is called. Thus, the sources cannot be destroyed after being
        // added. (should this change?)
        void add( GQVertexBufferType semantic, int width, const QVector<float>& data );
        void add( GQVertexBufferType semantic, int width, const QVector<uint8>& data );
        void add( GQVertexBufferType semantic, int width, int format, int length );
        void add( GQVertexBufferType semantic, const std::vector<vec>& data );
        void add( const QString& name, int width, const QVector<float>& data );
        void add( const QString& name, int width, const QVector<uint8>& data );
        void add( const QString& name, int width, int format, int length );
        void add( const QString& name, const std::vector<vec>& data );

        int  numBuffers() const { return _buffers.size(); }

        void setStartingElement( int element ) { _starting_element = element; }
        void setElementStride( int stride ) { _element_stride = stride; }
        void setUsageMode(GQVertexBufferUsage usage_mode);

        void bind() const;
        void bind( const GQShaderRef& current_shader ) const;
        void unbind() const;
        bool isBound() const { return _guid == _bound_guid; }

        int  vboId( GQVertexBufferType semantic ) const;
        int  vboId( const QString& name ) const;
        void copyDataToVBOs();
        void deleteVBOs();
        bool vbosLoaded() const;

        void copyFromFBO(const GQFramebufferObject& fbo, int fbo_buffer, 
                         const QString& vbo_name );
        void copyFromFBO(const GQFramebufferObject& fbo, int fbo_buffer, 
                         GQVertexBufferType vbo_semantic );
        void copyFromSubFBO(const GQFramebufferObject& fbo, int fbo_buffer, 
                            int x, int y, int width, int height, 
                            const QString& vbo_name );
        void copyFromSubFBO(const GQFramebufferObject& fbo, int fbo_buffer, 
                            int x, int y, int width, int height, 
                            GQVertexBufferType vbo_semantic );

    protected:
        class BufferInfo {
        public:
            void init( const QString& name, int usage_mode, 
                       int data_type, int width, int length, 
                       const uint8* data_pointer);

            const uint8* dataPointer() const;
            int          dataSize() const;
        public:
            QString _name;
            GQVertexBufferType _semantic;
            int _data_type;
            int _type_size;
            int _width;
            int _length;
            int _vbo_id;
            int _vbo_size;
            int _gl_usage_mode;
            bool _normalize;

            const uint8* _data_pointer;
        };

        QHash<QString, BufferInfo*> _buffer_hash;
        QList<BufferInfo>           _buffers;

        int _starting_element;
        int _element_stride;
        int _gl_usage_mode;

        int _guid;

        static bool              _gl_buffer_object_bound;
        static int               _last_used_guid;
        static int               _bound_guid;
        static QList<int>        _bound_attribs;

    protected:
        void add( const BufferInfo& buffer_info );

        void bindBuffer( const BufferInfo& info, int attrib ) const;
};

#endif