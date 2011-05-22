/*****************************************************************************\
 
 qrtscApp.h
 Author: Forrester Cole (fcole@cs.princeton.edu)
 Copyright (c) 2009 Forrester Cole
 
 qviewer is distributed under the terms of the GNU General Public License.
 See the COPYING file for details.
 
 \*****************************************************************************/

#include <QApplication>
#include <QString>
#include <QDir>
#include "MainWindow.h"

class qrtscApp : public QApplication
{
    Q_OBJECT
    
  public:
    qrtscApp(int& argc, char** argv);
    
  public slots:
    bool event(QEvent *event);
    
    void printUsage(const char *myname);
    
    QDir findShadersDirectory();

  protected:
    MainWindow _main_window;
};
