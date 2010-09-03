/*****************************************************************************\

GQShaderManager.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

A class to load, compile, and manage GLSL shader programs. Programs themselves
are specified in an external file (programs.xml).

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef GQ_SHADER_MANAGER_H_
#define GQ_SHADER_MANAGER_H_

#include "GQInclude.h"
#include <QString>
#include <QDomElement>
#include <QHash>
#include <QVector>
#include <QDir>

class GQTexture;

typedef enum
{
    GQ_SHADERS_OK,
    GQ_SHADERS_NOT_LOADED,
    GQ_SHADERS_COMPILE_ERROR,

    GQ_NUM_SHADER_STATUS
} GQShaderStatus;

class GQShaderManager;

// This is a class to keep track of the currently bound shader program. 
// If the ref is destroyed (i.e. passes out of scope) it decrements the 
// shader manager's ref count. If the ref count reaches zero, the program
// is unbound. The program can also be directly unbound by calling unbind().

class GQShaderRef
{
    public:
        GQShaderRef();
        GQShaderRef(const GQShaderRef& input);
        ~GQShaderRef();
        const GQShaderRef& operator=(const GQShaderRef& input);

        const QString& name() const { return _name; }
        bool isValid() const;
    
        int  uniformLocation( const QString& name ) const;
        int  uniformLocationExistsCheck( const QString& name ) const;
        bool setUniform1f( const QString& name, float value ) const;
        bool setUniform1i( const QString& name, int value ) const;
        bool setUniform2f( const QString& name, float a, float b ) const;
        bool setUniform3f( const QString& name, float a, float b,
                           float c ) const;
        bool setUniform3fv( const QString& name, const float* value ) const;
        bool setUniform4f( const QString& name, float a, float b, 
                           float c, float d ) const;
        bool setUniform4fv( const QString& name, const float* value ) const;
        bool setUniformMatrix4fv( const QString& name, const float* value ) const;
        //bool setUniformXform( const QString& name, const xform& xf ) const;
        bool setUniformMatrixUpper3x3( const QString& name, const xform& xf ) const;
    
        bool setUniformVec3Array( const QString& name, const QVector<vec>& v, int count = -1 ) const;
        bool setUniformVec2Array( const QString& name, const QVector<vec2>& v, int count = -1 ) const;
    
        int  attribLocation( const QString& name ) const;

        bool bindNamedTexture( const QString& name, const GQTexture* tex ) const;

        void unbind();

    private:
        // Only the shader manager can create valid references.
        GQShaderRef( const QString& name, int guid );

    private:
        QString _name;
        int     _guid;

    friend class GQShaderManager;
};

class GQShaderManager
{
    public:
        static GQShaderStatus status() { return _status; }
        static void initialize();
        static void deinitialize();
        static void reload();

        static void setShaderDirectory( const QDir& directory ) { _shader_directory = directory; }

         static GQShaderRef bindProgram( const QString& name );
        static const QString& currentProgramName() { return _current_program_name; }
       
    // interface to the shader ref class
    protected:
        static void unbindProgram( int program_bind_guid );

        static int  uniformLocation( int program_bind_guid,  const QString& name );
        static int  attribLocation( int program_bind_guid, const QString& name );

        static bool bindNamedTexture(int program_bind_guid, const QString& name, 
                                     const GQTexture* tex );
        
        static int  currentProgramRefGuid() { return _current_program_ref_guid; }
        static void incRef( int program_bind_guid );
        static void decRef( int program_bind_guid );

    private:
        static void checkHardwareCompatibility();

        static int  filterWarnings( QStringList& log );
        static void shaderInfoLog( const QString& filename, GLuint obj );
        static void programInfoLog( const QString& filename, GLuint obj );
        static void processCompileReport();

        static QString loadShaderFile( const QString& programname, const QString& filename );

        static void setGeometryShaderProgramParams(const QString& program_name, 
                                                   GLuint program_handle, 
                                                   const QDomElement& element);

        static void incProgramRefGuid();

    private:
        static QHash<QString, int> _program_hash;
        static QVector<GLuint> _programs;
        static QVector<GLuint> _shaders;

        static QDir           _shader_directory;

        static QString        _compile_report;
        static int            _total_filtered_warnings;
        static bool           _dump_compile_report;
        static int            _warning_level;

        static int              _current_program;
        static QString        _current_program_name;

        static QVector<QString> _bound_texture_names;
        static QVector<const GQTexture*> _bound_textures;

        static int            _current_program_ref_guid;
        static int            _current_program_ref_count;

        static GQShaderStatus _status;

    friend class GQShaderRef;
};

#endif /*GQ_SHADER_MANAGER_H_*/
