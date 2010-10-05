/*****************************************************************************\

Console.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2010 Forrester Cole

Class for window based command line operations

demoutils is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "Console.h"
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>
#include <assert.h>

#ifdef WIN32
    #include <string.h>
    #include "windows.h"
#endif

Console* Console::_current_msg_console = 0;

Console::Console(QMainWindow* parent, QMenu* window_menu) : 
    QDockWidget(tr("Console"), parent)
{
    setWidget(&_edit_widget);
    parent->addDockWidget(Qt::RightDockWidgetArea, this);
    setObjectName("console");

    connect( &_process, SIGNAL(readyReadStandardOutput()), this, 
        SLOT(getProcessStdout()) );
    _process.setReadChannelMode( QProcess::MergedChannels );
    _process.setReadChannel( QProcess::StandardOutput );

    if (window_menu) {
        window_menu->addAction(this->toggleViewAction());
    }
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
        case QtDebugMsg: out = QString("Debug: ") + out; break;
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
