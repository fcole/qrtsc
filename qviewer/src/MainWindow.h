/*****************************************************************************\

MainWindow.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

The main application window. Handles all the Qt event notifications and
state management.

dpix is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSignalMapper>
#include <QDir>

class GLViewer;
class Scene;
class DialsAndKnobs;
class Console;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    MainWindow( );
    ~MainWindow( );
    void init( const QDir& working_dir, const QString& scene_name );

  public slots:
    void on_actionOpen_Scene_triggered();
    void on_actionSave_Scene_triggered();
    void on_actionReload_Shaders_triggered();
    void on_actionOpen_Recent_Scene_triggered(int which);
    void on_actionSave_Screenshot_triggered();
    void on_actionCamera_Perspective_toggled(bool checked);
    void resizeToFitViewerSize(const QString& size);

  protected:
    void closeEvent(QCloseEvent* event );

    bool openScene( const QString& filename );
    bool saveScene( const QString& filename );

    void resizeToFitViewerSize( int x, int y );
    void setFoV(float degrees);

    void setupUi();
    void setupFileMenu();
    void setupDockWidgets(QMenu* menu);
    void makeWindowTitle();
    void hideAllDockWidgets();

    void addCurrentSceneToRecentList();
    void updateRecentScenesMenu();

    void setupViewerResizeActions(QMenu* menu);

    QString myFileDialog( int mode, const QString& caption, const QString& filter, QString& last_dir );

  private:
    GLViewer*	_gl_viewer;
    Scene*      _scene;
    Console*    _console;
    DialsAndKnobs* _dials_and_knobs;
    QDockWidget* _stats_widget;
    QSignalMapper _viewer_size_mapper;

    QString     _scene_name;

    QStringList _recent_scenes;
    QList<QAction*> _recent_scenes_actions;
    QMenu*      _recent_scenes_menu;
    QSignalMapper _recent_scenes_mapper;

    QDir        _working_dir;

    QString     _last_scene_dir;
    QString     _last_export_dir;
    QString     _last_camera_dir;
};

#endif
