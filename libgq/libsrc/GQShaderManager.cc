/*****************************************************************************\

GQShaderManager.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

libgq is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "GQShaderManager.h"
#include "GQTexture.h"
#include <assert.h>

#include <QtGlobal>
#include <QImage>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QFile>
#include <QTextStream>

// GQShaderRef

GQShaderRef::GQShaderRef()
{
    _guid = -1;
}

GQShaderRef::GQShaderRef( const QString& name, int guid )
{
    _name = name;
    _guid = guid;
}

GQShaderRef::GQShaderRef(const GQShaderRef& input)
{
    *this = input;
}

const GQShaderRef& GQShaderRef::operator=(const GQShaderRef& input)
{
    this->_name = input._name;
    this->_guid = input._guid;
    GQShaderManager::incRef( _guid );
    return *this;
}

GQShaderRef::~GQShaderRef()
{
    GQShaderManager::decRef( _guid );
}

int GQShaderRef::uniformLocation( const QString& name ) const
{
    assert( _guid == GQShaderManager::getCurrentProgramRefGuid() );
    return GQShaderManager::uniformLocation( _guid, name );
}

int GQShaderRef::uniformLocationExistsCheck( const QString& name ) const
{
    int loc = uniformLocation(name);
    if (loc < 0)
    {
        qWarning("GQShaderManager::uniformLocation: Could not find uniform \"%s\"",
                qPrintable(name));
    }
    return loc;
}

bool GQShaderRef::setUniform1f( const QString& name, float value ) const
{
    glUniform1f(uniformLocationExistsCheck(name), value );
    return true;
}

bool GQShaderRef::setUniform1i( const QString& name, int value ) const
{
    glUniform1i(uniformLocationExistsCheck(name), value );
    return true;
}

bool GQShaderRef::setUniform2f( const QString& name, float a, float b ) const
{
    glUniform2f(uniformLocationExistsCheck(name), a, b );
    return true;
}

bool GQShaderRef::setUniform3f( const QString& name, float a, float b, 
                                float c) const
{
    glUniform3f(uniformLocationExistsCheck(name), a, b, c);
    return true;
}

bool GQShaderRef::setUniform3fv( const QString& name, const float* value ) const
{
    glUniform3fv(uniformLocationExistsCheck(name), 1, value );
    return true;
}


bool GQShaderRef::setUniform4f( const QString& name, float a, float b, 
                                float c, float d ) const
{
    glUniform4f(uniformLocationExistsCheck(name), a, b, c, d );
    return true;
}

bool GQShaderRef::setUniform4fv( const QString& name, const float* value ) const
{
    glUniform4fv(uniformLocationExistsCheck(name), 1, value );
    return true;
}

bool GQShaderRef::setUniformMatrix4fv( const QString& name, const float* value ) const
{
    const bool transpose = false;
    glUniformMatrix4fv(uniformLocationExistsCheck(name), 1, transpose, value );
    return true;
}

int GQShaderRef::attribLocation( const QString& name ) const
{
    assert( _guid == GQShaderManager::getCurrentProgramRefGuid() );
    return GQShaderManager::attribLocation( _guid, name );
}
        
bool GQShaderRef::bindNamedTexture( const QString& name, const GQTexture* tex ) const
{
    assert( _guid == GQShaderManager::getCurrentProgramRefGuid() );
    return GQShaderManager::bindNamedTexture(_guid, name, tex);
}

void GQShaderRef::unbind()
{
    assert( _guid == GQShaderManager::getCurrentProgramRefGuid() );
    GQShaderManager::unbindProgram( _guid );
}


// GQShaderManager

QHash<QString, int>		GQShaderManager::_program_hash;
QVector<GLuint>			GQShaderManager::_programs;
QVector<GLuint>			GQShaderManager::_shaders;
QString                 GQShaderManager::_compile_report;
int                     GQShaderManager::_total_filtered_warnings;
bool                    GQShaderManager::_dump_compile_report = true;
GQShaderStatus          GQShaderManager::_status = GQ_SHADERS_NOT_LOADED;
int                     GQShaderManager::_warning_level = 1;

QDir                    GQShaderManager::_shader_directory;

int						GQShaderManager::_current_program;
QString                 GQShaderManager::_current_program_name;
QVector<const GQTexture*>    GQShaderManager::_bound_textures;
QVector<QString>        GQShaderManager::_bound_texture_names;

int                     GQShaderManager::_current_program_ref_guid = 0;
int                     GQShaderManager::_current_program_ref_count = 0;

void GQShaderManager::deinitialize()
{
    glUseProgram(0);

    for (int i = 0; i < _programs.size(); i++)
    {
        glDeleteProgram(_programs[i]);
    }
    for (int i = 0; i < _shaders.size(); i++)
    {
        glDeleteShader(_shaders[i]);
    }

    _program_hash.clear();
    _programs.clear();
    _shaders.clear();

    _current_program = -1;
    _status = GQ_SHADERS_NOT_LOADED;
}

void GQShaderManager::checkHardwareCompatibility()
{
    GLint max_tex_coords;
    GLint max_varying_floats;

    glGetIntegerv(GL_MAX_TEXTURE_COORDS, &max_tex_coords);
    glGetIntegerv(GL_MAX_VARYING_FLOATS, &max_varying_floats);

    assert(max_tex_coords >= 8);
    assert(max_varying_floats >= 32);
}


void GQShaderManager::initialize()
{
    checkHardwareCompatibility();

    if (_status == GQ_SHADERS_OK) 
        qFatal("GQShaderManager::initialize() called on initialized object");

    _status = GQ_SHADERS_OK;
    _compile_report = QString();
    _total_filtered_warnings = 0;
    
    QDomDocument shader_doc("programs");
    QString programsfilename = _shader_directory.filePath("programs.xml");
    QFile file(programsfilename);
    if (!file.open(QIODevice::ReadOnly))
        qFatal("GQShaderManager::initialize(): could not open programs.xml");

    QString parse_errors;
    if (!shader_doc.setContent(&file, &parse_errors))
        qFatal("GQShaderManager::initialize(): %s", qPrintable(parse_errors));

    file.close();

    QDomElement shader_root = shader_doc.documentElement();
    QDomElement program_element = shader_root.firstChildElement();
    while (!program_element.isNull())
    {
        assert(program_element.tagName() == "program");

        QString program_name = program_element.attribute("name");

        // make sure we haven't seen a program with the same name already
        assert( _program_hash.value(program_name, -1) == -1 );

        GLuint program_handle = glCreateProgram();

        QDomElement shader_element = program_element.firstChildElement();
        while(!shader_element.isNull())
        {
            assert(shader_element.tagName() == "shader");
			
            QString shader_type = shader_element.attribute("type");

            int gl_type;
            if (shader_type == "vertex")
                gl_type = GL_VERTEX_SHADER;
            else if (shader_type == "fragment")
                gl_type = GL_FRAGMENT_SHADER;
            else if (shader_type == "geometry")
            {
                gl_type = GL_GEOMETRY_SHADER_EXT;
                setGeometryShaderProgramParams(program_name, program_handle, 
                                               shader_element);
            }
            else
                assert(0);
            GLuint shader_handle = glCreateShader(gl_type);

            int num_source_files = 0;
            QByteArray source_files[10];
            char* p_source_files[10];
            QDomElement source_element = shader_element.firstChildElement();
            while(!source_element.isNull())
            {
                assert(source_element.tagName() == "source");
		
                QString shaderfile = loadShaderFile(program_name, source_element.attribute("filename"));
                source_files[num_source_files] = shaderfile.toAscii();
                p_source_files[num_source_files] = source_files[num_source_files].data();
                num_source_files++;

                source_element = source_element.nextSiblingElement();
            }

            glShaderSource( shader_handle, num_source_files, (const GLchar**)(p_source_files), NULL);

            glCompileShader( shader_handle );
            shaderInfoLog( QString(program_name + ", " + shader_type), shader_handle );

            _shaders.push_back(shader_handle);

            glAttachShader(program_handle, shader_handle);

            shader_element = shader_element.nextSiblingElement();
        }

        glLinkProgram( program_handle );

        // The program info log is redundant with the shader info logs, 
        // at least on windows. 
        //programInfoLog( program_name, program_handle);

        _program_hash.insert(program_name, _programs.size());
        _programs.push_back( program_handle );
			
        program_element = program_element.nextSiblingElement();
    }

    processCompileReport();

    _current_program = -1;
}

void GQShaderManager::reload()
{
    qDebug("Reloading Shaders...");
    deinitialize();
    initialize();
    qDebug("Done.");
}

QString GQShaderManager::loadShaderFile( const QString& programname, const QString& filename )
{
    QString absname = _shader_directory.filePath(filename);
    QFile file(absname);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCritical("GQShaderManager: failed to open \'%s\' in program \'%s\'\n", 
			qPrintable(filename), qPrintable(programname));
        return QString();
    }
    QString out = file.readAll() + "\n";
    file.close();
    return out;
}

GQShaderRef GQShaderManager::bindProgram( const QString& name )
{
    if (_current_program_ref_count > 0)
    {
        unbindProgram(_current_program_ref_guid);
    }

    int program_index = _program_hash.value(name, -1);
    if (program_index < 0 || program_index >= (int)(_programs.size()))
    {
        qFatal("GQShaderManger::bindProgram: could not find program %s", 
            qPrintable(name));
        return GQShaderRef();
    }

    assert( _bound_textures.isEmpty() );

    GLuint program_handle = _programs[program_index];
    glUseProgram(program_handle);
    _current_program = program_index;
    _current_program_name = name;

    reportGLError();

    incProgramRefGuid();
    _current_program_ref_count = 1;

    return GQShaderRef( _current_program_name, _current_program_ref_guid );
}

void GQShaderManager::unbindProgram( int program_ref_guid )
{
    if ( _current_program >= 0 && program_ref_guid == _current_program_ref_guid )
    {
        glUseProgram(0);
        for (int i = 0; i < _bound_textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            _bound_textures[i]->unbind();
        }
        glActiveTexture(GL_TEXTURE0);

        _bound_texture_names.clear();
        _bound_textures.clear();
        _current_program = -1;
        _current_program_name = QString();

        incProgramRefGuid();
    }
}

int GQShaderManager::uniformLocation(int program_ref_guid, 
                                     const QString& name)
{
    int loc = -1;
    if (_current_program >= 0 && 
        _current_program_ref_guid == program_ref_guid)
    {
        loc = glGetUniformLocation(_programs[_current_program], 
                                   qPrintable(name) );
       
        reportGLError();
    }
    return loc;
}

int GQShaderManager::attribLocation(int program_ref_guid, 
                                    const QString& name) 
{
    int loc = -1;
    if (_current_program >= 0 && 
        _current_program_ref_guid == program_ref_guid)
    {
        loc = glGetAttribLocation(_programs[_current_program], 
                                  qPrintable(name) );
        reportGLError();
    }
    return loc;
}


bool GQShaderManager::bindNamedTexture(int program_ref_guid, 
                                       const QString& name, 
                                       const GQTexture* texture)
{
    if (_current_program >= 0 && 
        _current_program_ref_guid == program_ref_guid)
    {
        GLint max_units;
        glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &max_units );
        if (max_units <= (int)(_bound_textures.size()))
        {
            qCritical("Attempted to bind too many textures (limit is %d)", 
                max_units);
            return false;
        }
        int loc = uniformLocation( program_ref_guid, name );
        if (loc < 0)
        {
            qCritical("Cannot find texture \"%s\" in program %s\n", 
                qPrintable(name), qPrintable(_current_program_name));
            return false;
        }

        // check if this texture name is already bound,
        // if so, bind over the existing one
        int tex_unit = 0;
        while (tex_unit < (int)(_bound_textures.size()))
        {
            if (_bound_texture_names[tex_unit] == name)
                break;
            tex_unit++;
        }
        if (tex_unit >= (int)(_bound_textures.size()))
        {
            _bound_texture_names.push_back(name);
            _bound_textures.push_back(texture);
        }

        glUniform1i( loc, tex_unit ); 
        glActiveTexture( GL_TEXTURE0 + tex_unit );
        texture->bind();

        //qWarning("binding %s on unit %d\n", qPrintable(name), tex_unit);
        reportGLError();
        return true;
    }
    return false;
}

void GQShaderManager::incRef( int program_ref_guid )
{
    if (program_ref_guid == _current_program_ref_guid)
    {
        _current_program_ref_count++;
    }
}

void GQShaderManager::decRef( int program_ref_guid )
{
    if (program_ref_guid == _current_program_ref_guid)
    {
        assert(_current_program_ref_count > 0);
        _current_program_ref_count--;
        if (_current_program_ref_count == 0)
            unbindProgram(_current_program_ref_guid);
    }
}

int GQShaderManager::filterWarnings( QStringList& log )
{
    int num_filtered = 0;
    QStringList level_0_warnings = QStringList() << 
        QString("WARNING: vertex shader writes varying '\\w+' which is not active.");
    QList<QRegExp> level_0_rx;
    for (int i = 0; i < level_0_warnings.size(); i++)
    {
        level_0_rx.append(QRegExp(level_0_warnings[i]));
    }

    if (_warning_level > 0)
    {
        QMutableListIterator<QString> iter(log);
        while( iter.hasNext() )
        {
            QString val = iter.next();
            bool filter = false;
            for (int i = 0; i < level_0_rx.size(); i++)
            {
                if (level_0_rx[i].indexIn(val) == 0)
                {
                    filter = true;
                    break;
                }
            }

            if (filter)
            {
                num_filtered++;
                iter.remove();
            }
        }
    }

    return num_filtered;
}

void GQShaderManager::shaderInfoLog( const QString& filename, GLuint obj)
{
    GLint infologLength = 0;
    GLint charsWritten  = 0;
    char *infoLog;

    glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 1)
    {
        infoLog = (char *)malloc(infologLength);
        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
        QStringList list = QString(infoLog).trimmed().split("\n");
        _total_filtered_warnings += filterWarnings(list);
        if (list.size() > 0) 
        {
            list.prepend(QString("Shader info log for %1\n").arg(filename));
            _compile_report.append(list.join("\n") + QString("\n"));
        }
        free(infoLog);
    }
}

void GQShaderManager::programInfoLog( const QString& filename, GLuint obj)
{
    GLint infologLength = 0;
    GLint charsWritten  = 0;
    char *infoLog;

    glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 1)
    {
        infoLog = (char *)malloc(infologLength);
        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);

        QStringList list = QString(infoLog).trimmed().split("\n");
        _total_filtered_warnings += filterWarnings(list);
        if (list.size() > 0) 
        {
            list.prepend(QString("Program info log for %1\n").arg(filename));
            _compile_report.append(list.join("\n") + "\n");
        }
        free(infoLog);
    }
}

void GQShaderManager::processCompileReport()
{
    QDir working = _shader_directory;
    QString filename = working.filePath("glsl_compile_log.txt"); 
    if (QFile::exists(filename))
    {
        working.remove("glsl_compile_log_old.txt");
        working.rename("glsl_compile_log.txt", "glsl_compile_log_old.txt");
    }

    if (!_compile_report.isEmpty())
    {
        qCritical("%s", qPrintable(_compile_report));
        if (_compile_report.indexOf("error", 0, Qt::CaseInsensitive) >= 0)
            _status = GQ_SHADERS_COMPILE_ERROR;

        if (_dump_compile_report)
        {
            QFile logfile(filename);
            if (logfile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream out(&logfile);
                out << _compile_report;
                logfile.close();
            }
            else
            {
                qCritical("GQShaderManager::processCompileReport: Unable to open log file \"%s\"\n", qPrintable(filename));
            }
        }
    }

    if (_total_filtered_warnings > 0)
    {
        qWarning("%d shader warnings below current threshold.\n", _total_filtered_warnings);
    }
}

void GQShaderManager::incProgramRefGuid()
{
    _current_program_ref_guid++;
    if (_current_program_ref_guid <= 0)
        _current_program_ref_guid = 1;
}

int translateStringToGLType(const QString& string)
{
    int gl_type = -1;
    if (string == "points")
        gl_type = GL_POINTS;
    else if (string == "lines")
        gl_type = GL_LINES;
    else if (string == "line_strip")
        gl_type = GL_LINE_STRIP;
    else if (string == "triangle_strip")
        gl_type = GL_TRIANGLE_STRIP;

    return gl_type;
}

void GQShaderManager::setGeometryShaderProgramParams(const QString& program_name, 
                                                     GLuint program_handle,
                                                     const QDomElement& element)
{
    QString empty_error_msg = 
        "error: geometry shader parameter \"%1\" is empty in program %2\n";
    QString unrecognized_error_msg = 
        "error: geometry shader parameter \"%1\" has unrecognized value \"%2\" in program %3\n";

    QString input_type = element.attribute("input_type");
    if (!input_type.isEmpty())
    {
        int gl_type = translateStringToGLType(input_type);
        if (gl_type >= 0)
        {
            glProgramParameteriEXT(program_handle, GL_GEOMETRY_INPUT_TYPE_EXT, gl_type);
            reportGLError();
        }
        else
        {
            _compile_report.append(
                unrecognized_error_msg.arg("input_type").arg(input_type).arg(program_name));
        }
    }
    else
    {
        _compile_report.append(empty_error_msg.arg("input_type").arg(program_name));
    }

    QString output_type = element.attribute("output_type");
    if (!output_type.isEmpty())
    {
        int gl_type = translateStringToGLType(output_type);
        if (gl_type >= 0)
        {
            // Currently, only POINTS, LINE_STRIP, and TRIANGLE_STRIP are supported
            // as output types. 
            glProgramParameteriEXT(program_handle, GL_GEOMETRY_OUTPUT_TYPE_EXT, gl_type);
            reportGLError();
        }
        else
        {
            _compile_report.append(
                unrecognized_error_msg.arg("output_type").arg(output_type).arg(program_name));
        }
    }
    else
    {
        _compile_report.append(empty_error_msg.arg("output_type").arg(program_name));
    }

    QString vertices_out = element.attribute("vertices_out");
    if (!vertices_out.isEmpty())
    {
        int vertices = vertices_out.toInt();
        if (vertices > 0)
        {
            glProgramParameteriEXT(program_handle, GL_GEOMETRY_VERTICES_OUT_EXT, vertices);
            reportGLError();
        }
        else
        {
            _compile_report.append(
                unrecognized_error_msg.arg("vertices_out").arg(vertices_out).arg(program_name));
        }
    }
    else
    {
        _compile_report.append(empty_error_msg.arg("vertices_out").arg(program_name));
    }

}




