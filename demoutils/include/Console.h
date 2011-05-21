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
#include <QLineEdit>
#include <QDockWidget>
#include <QStringList>
#include <QProcess>
#include <QScriptEngine>

class ScriptUtilities : public QObject
{
    Q_OBJECT
    
  public slots:
    QStringList findFiles(const QString& dir, const QString& filters);
    QString basename(const QString& path);
    QString dirname(const QString& path);
};

class CommandLine : public QLineEdit
{
    Q_OBJECT

  protected:
    void keyPressEvent(QKeyEvent* event);

  protected:
    QStringList _history;
    int         _history_position;
};

class Console : public QDockWidget
{
    Q_OBJECT

public:
    Console(QMainWindow* parent, QMenu* menu);

    void print( const QString& str );
    void execute( const QString& cmd, const QStringList& args );

    void installMsgHandler();
    void removeMsgHandler();
    
    bool exposeObjectToScript(QObject* object, const QString& name);

public slots:
    void getProcessStdout();
    void processCommand();
    void runScript(const QString& filename);

protected:
    static void msgHandler( QtMsgType type, const char* msg );
    
protected:
    void addGlobalsToEngine();

protected:
    QTextEdit _edit_widget;
    CommandLine _command_line;
    QProcess _process;
    
    QScriptEngine _engine;
    ScriptUtilities _utils;

    static Console* _current_msg_console;
};

#endif // CONSOLE_H_
