/*****************************************************************************\

MainWindow.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

dpix is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include <QtGui>
#include <QFileDialog>
#include <QRegExp>
#include <QSignalMapper>

#include "GLViewer.h"
#include "MainWindow.h"
#include "Scene.h"
#include "GQShaderManager.h"

#include "Stats.h"
#include "DialsAndKnobs.h"
#include "Console.h"

#include <XForm.h>
#include <assert.h>

const int CURRENT_INTERFACE_VERSION = 1;

const int MAX_RECENT_SCENES = 4;


MainWindow::MainWindow( )
{
    _console = NULL;
    _scene = NULL;
    _dials_and_knobs = NULL;
    _gl_viewer = new GLViewer;
}

MainWindow::~MainWindow()
{
    delete _scene;
}

void MainWindow::init( const QDir& working_dir, const QString& scene_name )
{
    _working_dir = working_dir;

    QGLFormat format;
    format.setAlpha(true);
    // enabling multisampling caused weird buffer corruption bugs 
    // (back buffer sometimes is visible, back buffer sometimes is not 
    // cleared properly)
    //format.setSampleBuffers(true);
    format.setDoubleBuffer(true);
    QGLFormat::setDefaultFormat(format);

    setCentralWidget(_gl_viewer);
    setupUi();

    // read back qsettings and update window state and recent file list
    QSettings settings("qrtsc", "qrtsc");
    settings.beginGroup("mainwindow");
    QByteArray windowstate = settings.value("windowstate").toByteArray();
    bool restored = restoreState(windowstate, CURRENT_INTERFACE_VERSION);
    if (!restored)
    {
        hideAllDockWidgets();
    }
    QByteArray dk_state = settings.value("dials_and_knobs").toByteArray();
    restored = _dials_and_knobs->restoreState(dk_state, CURRENT_INTERFACE_VERSION);

    settings.endGroup();
    settings.beginGroup("recent_scenes");
    for (int i = 0; i < MAX_RECENT_SCENES; i++)
    {
        QString scene_name = 
            settings.value(QString("scene_%1").arg(i)).toString();

        if (!scene_name.isEmpty())
        {
            _recent_scenes.append(scene_name);
        }
    }
    updateRecentScenesMenu();

    _last_scene_dir = working_dir.absolutePath();
    _last_export_dir = working_dir.absolutePath();
    _last_camera_dir = working_dir.absolutePath();
    _last_screenshot_dir = working_dir.absolutePath();

    resize(1000,800);

    if (!scene_name.isEmpty())
    {
        openScene(scene_name);
    }

    makeWindowTitle();

    _gl_viewer->finishInit();
}

void MainWindow::closeEvent( QCloseEvent* event )
{
    if (_console)
        _console->removeMsgHandler();

    QSettings settings("qrtsc", "qrtsc");
    settings.beginGroup("mainwindow");
    settings.setValue("windowstate", saveState(CURRENT_INTERFACE_VERSION));
    settings.setValue("dials_and_knobs", _dials_and_knobs->saveState(CURRENT_INTERFACE_VERSION));
    settings.endGroup();

    addCurrentSceneToRecentList();

    settings.beginGroup("recent_scenes");
    for (int i = 0; i < _recent_scenes.size(); i++)
    {
        if (!_recent_scenes[i].isEmpty())
        {
            settings.setValue(QString("scene_%1").arg(i), _recent_scenes[i]);
        }
    }

    settings.sync();

    event->accept();
}

bool MainWindow::openScene( const QString& filename )
{
    // Try to do all the file loading before we change 
    // the mainwindow state at all.

    QString absfilename = 
        QDir::fromNativeSeparators(_working_dir.absoluteFilePath(filename));
    QFileInfo fileinfo(absfilename);
    
    if (!fileinfo.exists())
    {
        QMessageBox::critical(this, "File Not Found", 
            QString("\"%1\" does not exist.").arg(absfilename));
        return false;
    }

    Scene* new_scene = new Scene();
    
    if (!new_scene->load(absfilename))
    {
        QMessageBox::critical(this, "Open Failed", 
            QString("Failed to load \"%1\". Check console.").arg(absfilename));
        delete new_scene;
        return false;
    }

    // Success: file has been loaded, now change state

    if (_scene)
        delete _scene;

    _scene = new_scene;
    _scene_name = QDir::fromNativeSeparators(filename);

    Stats::instance().clear();
    _scene->recordStats(Stats::instance());

    _gl_viewer->setScene(_scene);
    _dials_and_knobs->load(_scene->dialsAndKnobsState());

    addCurrentSceneToRecentList();

    makeWindowTitle();
    
    return true;
}

bool MainWindow::saveScene( const QString& filename )
{
    return _scene->save(filename, _gl_viewer, _dials_and_knobs);
}

void MainWindow::on_actionOpen_Recent_Scene_triggered(int which)
{
    if (_recent_scenes.size() > which && 
        !_recent_scenes[which].isEmpty())
    {
        QFileInfo fileinfo(_recent_scenes[which]);
        QString fullpath = fileinfo.absoluteFilePath();

        openScene( _recent_scenes[which] );
        _gl_viewer->updateGL();
    }
}

void MainWindow::addCurrentSceneToRecentList()
{
    if (!_scene_name.isEmpty() && !_recent_scenes.contains(_scene_name))
    {
        _recent_scenes.push_front(_scene_name);
        if (_recent_scenes.size() > MAX_RECENT_SCENES)
        {
            _recent_scenes.pop_back();
        }

        updateRecentScenesMenu();
    }
}

void MainWindow::updateRecentScenesMenu()
{
    for (int i=0; i < MAX_RECENT_SCENES; i++)
    {
        if (_recent_scenes.size() > i && !_recent_scenes[i].isEmpty())
        {
            if (_recent_scenes_actions.size() > i)
            {
                _recent_scenes_actions[i]->setText(_recent_scenes[i]);
            }
            else
            {
                QAction* new_action = new QAction(_recent_scenes[i], this);
                connect(new_action, SIGNAL(triggered(bool)), 
                        &_recent_scenes_mapper, SLOT(map()));
                _recent_scenes_mapper.setMapping(new_action, i);
                new_action->setShortcut(Qt::CTRL + Qt::Key_1 + i);
                _recent_scenes_actions.append(new_action);
                _recent_scenes_menu->addAction(new_action);
            }
        }
    }
    connect(&_recent_scenes_mapper, SIGNAL(mapped(int)), 
            this, SLOT(on_actionOpen_Recent_Scene_triggered(int)));
}

void MainWindow::setupUi()
{
    setupFileMenu();
    QMenu* windowMenu = menuBar()->addMenu(tr("&Window"));
    setupViewerResizeActions(windowMenu);
    windowMenu->addSeparator();
    setupDockWidgets(windowMenu);

}

void MainWindow::setupFileMenu()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* openSceneAction = new QAction(tr("&Open Scene"), 0);
    openSceneAction->setShortcut(QKeySequence(tr("Ctrl+O")));
    connect(openSceneAction, SIGNAL(triggered()), 
        this, SLOT(on_actionOpen_Scene_triggered()));
    fileMenu->addAction(openSceneAction);

    _recent_scenes_menu = fileMenu->addMenu(tr("&Recent Scenes"));

    fileMenu->addSeparator();

    QAction* saveSceneAction = new QAction(tr("&Save Scene..."), 0);
    connect(saveSceneAction, SIGNAL(triggered()), 
        this, SLOT(on_actionSave_Scene_triggered()));
    fileMenu->addAction(saveSceneAction);

    QAction* save_screenshot = new QAction(tr("Save S&creenshot..."), 0);
    save_screenshot->setShortcut(QKeySequence(tr("Ctrl+S")));
    connect(save_screenshot, SIGNAL(triggered()), 
        this, SLOT(on_actionSave_Screenshot_triggered()));
    fileMenu->addAction(save_screenshot);

    fileMenu->addSeparator();

    QAction* reloadShadersAction = new QAction(tr("&Reload Shaders"), 0);
    reloadShadersAction->setShortcut(QKeySequence(tr("Ctrl+L")));
    connect(reloadShadersAction, SIGNAL(triggered()), 
        this, SLOT(on_actionReload_Shaders_triggered()));
    fileMenu->addAction(reloadShadersAction);

    fileMenu->addSeparator();

    QAction* quit_action = new QAction(tr("&Quit"), 0);
    quit_action->setShortcut(QKeySequence(tr("Ctrl+Q")));
    connect(quit_action, SIGNAL(triggered()), 
        this, SLOT(close()));
    fileMenu->addAction(quit_action);
}

void MainWindow::setupDockWidgets(QMenu* menu)
{
    QStringList other_docks = QStringList() << "Lines" << "Tests" << 
        "Style" << "Vectors";
    _dials_and_knobs = new DialsAndKnobs(this, menu, other_docks);
    connect(_dials_and_knobs, SIGNAL(dataChanged()),
        _gl_viewer, SLOT(updateGL()));

    _console = new Console(this, menu);
    _console->installMsgHandler();

    _stats_widget = new StatsWidget(this, menu);
}

void MainWindow::makeWindowTitle()
{
    QFileInfo fileinfo(_scene_name);
    QString title = QString("qrtsc - %1").arg( fileinfo.fileName() );
    setWindowTitle( title );
}

void MainWindow::hideAllDockWidgets()
{
    _dials_and_knobs->hide();
    _console->hide();
}

void MainWindow::on_actionOpen_Scene_triggered()
{
    QString filename = 
        myFileDialog(QFileDialog::AcceptOpen, "Open Scene", 
        "Scenes (*." + Scene::fileExtension() + " *.off *.obj *.ply)", 
        _last_scene_dir );

    if (!filename.isNull())
    {
        openScene( filename );
    }
}

void MainWindow::on_actionSave_Scene_triggered()
{
    QString filename = 
        myFileDialog(QFileDialog::AcceptSave, "Save Scene", 
        "Scenes (*." + Scene::fileExtension() + ")", _last_scene_dir );

    if (!filename.isNull())
    {
        saveScene( filename );
    }
}


void MainWindow::on_actionSave_Screenshot_triggered()
{
	QString filename = 
        myFileDialog( QFileDialog::AcceptSave, "Save Screenshot", 
                "Images (*.jpg *.png *.pfm)", _last_screenshot_dir);

    if (!filename.isNull())
    {
        _gl_viewer->saveScreenshot(filename);
	}
}

void MainWindow::setFoV(float degrees)
{
    _gl_viewer->camera()->setFieldOfView( degrees * ( 3.1415926f / 180.0f ) );
    _gl_viewer->updateGL();
}


void MainWindow::resizeToFitViewerSize( int x, int y )
{
    QSize currentsize = size();
    QSize viewersize = _gl_viewer->size();
    QSize newsize = currentsize - viewersize + QSize(x,y);
    resize( newsize );
}

void MainWindow::resizeToFitViewerSize(const QString& size)
{
    QStringList dims = size.split("x");
    resizeToFitViewerSize(dims[0].toInt(), dims[1].toInt());
}

void MainWindow::on_actionReload_Shaders_triggered()
{
    GQShaderManager::reload();
    _gl_viewer->updateGL();
}


void MainWindow::setupViewerResizeActions(QMenu* menu)
{
    QStringList sizes = QStringList() << "512x512" <<
        "640x480" << "800x600" << "1024x768" << "1024x1024";

    QMenu* resize_menu = menu->addMenu("&Resize Viewer");

    for (int i = 0; i < sizes.size(); i++)
    {
        QAction* size_action = new QAction(sizes[i], 0);
        _viewer_size_mapper.setMapping(size_action, sizes[i]);
        connect(size_action, SIGNAL(triggered()), 
                &_viewer_size_mapper, SLOT(map()));
        resize_menu->addAction(size_action);
    }

    connect(&_viewer_size_mapper, SIGNAL(mapped(const QString&)),
            this, SLOT(resizeToFitViewerSize(const QString&)));

    menu->addMenu(resize_menu);
}

QString MainWindow::myFileDialog( int mode, const QString& caption, 
        const QString& filter, QString& last_dir)
{
    QFileDialog dialog(this, caption, last_dir, filter);
    dialog.setAcceptMode((QFileDialog::AcceptMode)mode);

    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted)
    {
        last_dir = dialog.directory().path();
        if (dialog.selectedFiles().size() > 0)
        {
            filename = dialog.selectedFiles()[0];
            if (mode == QFileDialog::AcceptSave)
            {
                QStringList acceptable_extensions;
                int last_pos = filter.indexOf("*.", 0);
                while (last_pos > 0)
                {
                    int ext_end = filter.indexOf(QRegExp("[ ;)]"), last_pos);
                    acceptable_extensions << filter.mid(last_pos+1, 
                            ext_end-last_pos-1);
                    last_pos = filter.indexOf("*.", last_pos+1);
                }
                if (acceptable_extensions.size() > 0)
                {
                    bool ext_ok = false;
                    for (int i = 0; i < acceptable_extensions.size(); i++)
                    {
                        if (filename.endsWith(acceptable_extensions[i]))
                        {
                            ext_ok = true;
                        }
                    }
                    if (!ext_ok)
                    {
                        filename = filename + acceptable_extensions[0];
                    }
                }
            }
        }
    }
    return filename;
}
