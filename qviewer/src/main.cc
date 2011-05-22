/*****************************************************************************\
 
 MainWindow.cc
 Author: Forrester Cole (fcole@cs.princeton.edu)
 Copyright (c) 2009 Forrester Cole
 
 qviewer is distributed under the terms of the GNU General Public License.
 See the COPYING file for details.
 
 \*****************************************************************************/

#include "GQInclude.h"
#include "GQShaderManager.h"

#include <QApplication>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QMessageBox>
#include <QFileOpenEvent>

#include <stdio.h>
#include <stdlib.h>
#include "XForm.h"
#include "MainWindow.h"
#include "qrtscApp.h"

qrtscApp::qrtscApp(int& argc, char** argv) : QApplication(argc, argv)
{
    QString scene_name;
    
    QDir shaders_dir = findShadersDirectory();
    QDir working_dir = QDir::current();
    
    GQShaderManager::setShaderDirectory(shaders_dir);
    
    for (int i = 1; i < arguments().size(); i++)
    {
        const QString& arg = arguments()[i];
        
        if (i == 1 && !arg.startsWith("-"))
        {
            scene_name = arg;
        }
        else
            printUsage(argv[0]);
    }
    
    _main_window.init( working_dir, scene_name );	
    _main_window.show();
}

bool qrtscApp::event(QEvent *event)
{
    switch (event->type()) {
        case QEvent::FileOpen:
            _main_window.openScene(static_cast<QFileOpenEvent *>(event)->file());
            return true;
        default:
            break;
    }
    return QApplication::event(event);
}

void qrtscApp::printUsage(const char *myname)
{
    fprintf(stderr, "\n");
    fprintf(stderr, "\n Usage    : %s [infile]\n", myname);
    exit(1);
}

QDir qrtscApp::findShadersDirectory()
{
    QString app_path = applicationDirPath();
    // look for the shaders/programs.xml file to find the working directory
    QStringList candidates;
    candidates << QDir::cleanPath(app_path)
    << QDir::cleanPath(QDir::currentPath() + "/../")
    << QDir::currentPath();
    
    for (int i = 0; i < candidates.size(); i++)
    {
        if (QFileInfo(candidates[i] + "/shaders/programs.xml").exists())
            return QDir(candidates[i] + "/shaders");
    }
    
    QString pathstring;
    for (int i = 0; i < candidates.size(); i++)
        pathstring = pathstring + candidates[i] + "/shaders/programs.xml\n";
    
    QMessageBox::critical(NULL, "Error", 
                          QString("Could not find programs.xml (or the working directory). Tried: \n %1").arg(pathstring));
    
    exit(1);
}

// The main routine makes the window, and then runs an event loop
// until the window is closed.
int main( int argc, char** argv )
{
    qrtscApp app(argc, argv);

    return app.exec();
}
