/*****************************************************************************\

Console.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2010 Forrester Cole

Class for window based command line operations

demoutils is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "Console.h"
#include "DialsAndKnobs.h"

#include <QMessageBox>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QKeyEvent>
#include <assert.h>

#ifdef WIN32
    #include <string.h>
    #include "windows.h"
#endif

Console* Console::_current_msg_console = 0;

Console::Console(QMainWindow* parent, QMenu* window_menu) : 
    QDockWidget(tr("Console"), parent)
{
    QWidget* widget = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(&_edit_widget);
    layout->addWidget(&_command_line);
    widget->setLayout(layout);
    setWidget(widget);
    
    parent->addDockWidget(Qt::RightDockWidgetArea, this);
    setObjectName("console");

    connect( &_process, SIGNAL(readyReadStandardOutput()), this, 
        SLOT(getProcessStdout()) );
    _process.setReadChannelMode( QProcess::MergedChannels );
    _process.setReadChannel( QProcess::StandardOutput );

    connect(&_command_line, SIGNAL(editingFinished()), this, 
            SLOT(processCommand()) );
    
    if (window_menu) {
        window_menu->addAction(this->toggleViewAction());
    }
    
    addGlobalsToEngine();
}

void Console::print( const QString& str )
{
    _edit_widget.insertPlainText( str );
    _edit_widget.moveCursor( QTextCursor::End );
}

void Console::execute( const QString& cmd, const QStringList& args )
{
    _process.start(cmd, args);
    _process.waitForFinished();
}

void Console::processCommand()
{
    QString command = _command_line.text();
    if (command.isEmpty()) {
        print(">>\n");
        return;
    }
    
    print(">> " + command + "\n");
    QScriptValue r = _engine.evaluate(command);
    if (_engine.hasUncaughtException()) {
        QStringList backtrace = _engine.uncaughtExceptionBacktrace();
        qDebug("    %s\n%s\n", qPrintable(r.toString()),
               qPrintable(backtrace.join("\n")));
    }
    _command_line.clear();
}

void Console::runScript(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug("Could not open %s", qPrintable(filename));
        return;
    }
    
    QTextStream in(&file);
    QString program = in.readAll();
    QScriptValue r = _engine.evaluate(program);
    if (_engine.hasUncaughtException()) {
        QStringList backtrace = _engine.uncaughtExceptionBacktrace();
        qDebug("    %s\n%s\n", qPrintable(r.toString()),
               qPrintable(backtrace.join("\n")));
    } else {
        qDebug("ok.");
    }
}

void Console::getProcessStdout()
{
    QString out = _process.readAll();
    print( out );
}

void Console::installMsgHandler()
{
    assert(_current_msg_console == 0);
    qInstallMsgHandler( msgHandler );
    _current_msg_console = this;

}

void Console::removeMsgHandler()
{
    assert(_current_msg_console == this);
    qInstallMsgHandler( 0 );
    _current_msg_console = 0;
}

void Console::msgHandler( QtMsgType type, const char* msg )
{
    if (type == QtFatalMsg)
    {
        QMessageBox::critical( 0, "Error", QString(msg) );
        abort();
    }

    QString out = QString(msg) + QString("\n");

    switch (type) {
        case QtWarningMsg:  out = QString("Warning: ") + out; break;
        case QtCriticalMsg: out = QString("Critical: ") + out; break;
        case QtDebugMsg: break;
        case QtFatalMsg: break;
    }

    _current_msg_console->print( out );
    fprintf( stderr, "%s", qPrintable(out) );

#ifdef WIN32
    wchar_t wcharstr[1024];
    int length = out.trimmed().left(1023).toWCharArray(wcharstr);
    wcharstr[length] = 0;
    OutputDebugStringW(wcharstr);
    OutputDebugStringA("\r\n");
#endif
}

bool Console::exposeObjectToScript(QObject* object, const QString& name)
{
    if (_engine.globalObject().property(name).isValid()) {
        qDebug("Console already has a %s", qPrintable(name));
        return false;
    }
    
    QScriptValue value = _engine.newQObject(object);
    _engine.globalObject().setProperty(name, value);

    return true;
}

void Console::addGlobalsToEngine()
{
    QList<dkValue*> dk_values = dkValue::allValues();
    for (int i = 0; i < dk_values.size(); i++) {
        exposeObjectToScript(dk_values[i], dk_values[i]->scriptName());
    }
    
    exposeObjectToScript(&_utils, "util");
    exposeObjectToScript(this, "console");
}


QStringList ScriptUtilities::findFiles(const QString& dir_name, const QString& filters)
{
    QStringList filter_list = filters.split(";");
    QDir dir = QDir(dir_name);
    QFileInfoList file_infos = dir.entryInfoList(filter_list, QDir::Files, QDir::Name);
    QStringList output;
    for (int i = 0; i < file_infos.size(); i++) {
        output << file_infos[i].absoluteFilePath();
    }
    return output;
}

QString ScriptUtilities::basename(const QString& path)
{
    QFileInfo info(path);
    return info.baseName();
}

QString ScriptUtilities::dirname(const QString& path)
{
    QFileInfo info(path);
    return info.dir().dirName();
}


void CommandLine::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (!text().isEmpty()) {
            _history << text();
            _history_position = _history.size();
        }
        QLineEdit::keyPressEvent(event);
    } else if (event->key() == Qt::Key_Up) {
        if (_history.size() > 0) {
            _history_position--;
            if (_history_position < 0)
                _history_position = 0;
            setText(_history[_history_position]);
        }
    } else if (event->key() == Qt::Key_Down) {
        if (_history.size() > 0) {
            _history_position++;
            if (_history_position < _history.size()) {
                setText(_history[_history_position]);
            } else {
                _history_position = _history.size()-1;
            }
        }
    } else {
        QLineEdit::keyPressEvent(event);
    }
}
