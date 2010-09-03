/*****************************************************************************\

Console.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2010 Forrester Cole

Class for window based command line operations

demoutils is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QDockWidget>
#include <QProcess>

class Console : public QDockWidget
{
    Q_OBJECT

public:
    Console(QMainWindow* parent, QMenu* menu);

    void print( const QString& str );
    void execute( const QString& cmd, const QStringList& args );

    void installMsgHandler();
    void removeMsgHandler();

public slots:
    void getProcessStdout();

protected:
    static void msgHandler( QtMsgType type, const char* msg );

protected:
    QTextEdit _edit_widget;
    QProcess _process;

    static Console* _current_msg_console;
};

#endif // CONSOLE_H_
