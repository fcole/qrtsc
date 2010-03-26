#include "GQInclude.h"
#include "GQShaderManager.h"

#include <QApplication>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QMessageBox>

#include <stdio.h>
#include <stdlib.h>
#include "XForm.h"
#include "MainWindow.h"

void printUsage(const char *myname)
{
    fprintf(stderr, "\n");
    fprintf(stderr, "\n Usage    : %s [infile]\n", myname);
    exit(1);
}

QDir findShadersDirectory( const QString& app_path )
{
    // look for the shaders/programs.xml file to find the working directory
    QStringList candidates;
    candidates << QDir::currentPath()
    << QDir::cleanPath(app_path)
    << QDir::cleanPath(app_path + "/../")
    << QDir::cleanPath(app_path + "/../../")
    << QDir::cleanPath(app_path + "/../../../../")
    << QDir::cleanPath(app_path + "/Contents/MacOS/");

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
    QString scene_name;

    QApplication app(argc, argv);
    QDir shaders_dir = findShadersDirectory(app.applicationDirPath());
    QDir working_dir = QDir::home();

    GQShaderManager::setShaderDirectory(shaders_dir);

    QStringList arguments = app.arguments();

    for (int i = 1; i < arguments.size(); i++)
    {
        const QString& arg = arguments[i];

        if (i == 1 && !arg.startsWith("-"))
        {
            scene_name = arg;
        }
        else
            printUsage(argv[0]);
    }

    MainWindow window;
    window.init( working_dir, scene_name );	
    window.show();

    return app.exec();
}
