
#ifndef _DIALS_AND_KNOBS_H_
#define _DIALS_AND_KNOBS_H_

#include <QString>
#include <QStringList>
#include <QHash>
#include <QDockWidget>
#include <QDomDocument>
#include <QSignalMapper>
#include <QModelIndex>

class QGridLayout;
class QVBoxLayout;
class QEvent;
class QMenu;
class QMenuBar;
class QFileSystemModel;
class QListView;

class DialsAndKnobs;

typedef enum
{
    DK_PANEL,
    DK_MENU,
    DK_NUM_LOCATIONS,
} dkLocation;


// dkValue
// Root class for all the types.

class dkValue : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString _name READ name)

  public:
    dkValue(const QString& name, dkLocation location);
    virtual ~dkValue();

    virtual bool load(QDomElement& root) = 0;
    virtual bool save(QDomDocument& doc, QDomElement& root) const = 0;

    const QString& name() const { return _name; }
    dkLocation location() const { return _location; }
    bool changedLastFrame() const;
    
    static dkValue* find(const QString& name);
    static int numValues() { return values().size(); }

  protected:
    static void add(dkValue* value);
    static void remove(dkValue* value);
    static QList<dkValue*>& values();
    static QHash<QString, dkValue*>& values_hash();

  protected:
    QString _name;
    dkLocation _location;
    int        _last_change_frame_number;

  friend class DialsAndKnobs;
};

// dkFloat
// A single double-precision floating point value.
     
class dkFloat : public dkValue
{
    Q_OBJECT
    Q_PROPERTY(double _value READ value WRITE setValue)

  public:
    dkFloat(const QString& name, double value);
    dkFloat(const QString& name, double value, 
            double lower_limit, double upper_limit, 
            double step_size);

    virtual bool load(QDomElement& root);
    virtual bool save(QDomDocument& doc, QDomElement& root) const;

    double value() const { return _value; }
    operator double() const {return _value; }

    double lowerLimit() const { return _lower; }
    double upperLimit() const { return _upper; }
    double stepSize() const { return _step_size; }

    static dkFloat* find(const QString& name);

  public slots:
    void setValue(double f);

  signals:
    void valueChanged(double f);

  protected:
    double _value;
    double _lower, _upper;
    double _step_size;
};

// dkInt
// A single integer value.

class dkInt : public dkValue
{
    Q_OBJECT
    Q_PROPERTY(int _value READ value WRITE setValue)

  public:
    dkInt(const QString& name, int value);
    dkInt(const QString& name, int value, 
            int lower_limit, int upper_limit, 
            int step_size);

    virtual bool load(QDomElement& root);
    virtual bool save(QDomDocument& doc, QDomElement& root) const;

    int value() const { return _value; }
    operator int() const {return _value; }

    int lowerLimit() const { return _lower; }
    int upperLimit() const { return _upper; }
    int stepSize() const { return _step_size; }

    static dkInt* find(const QString& name);

  public slots:
    void setValue(int i);

  signals:
    void valueChanged(int f);

  protected:
    int _value;
    int _lower, _upper;
    int _step_size;
};

// dkBool
// A single boolean value.

class dkBool : public dkValue
{
    Q_OBJECT
    Q_PROPERTY(bool _value READ value WRITE setValue)

  public:
    dkBool(const QString& name, bool value, 
           dkLocation location = DK_PANEL);

    virtual bool load(QDomElement& root);
    virtual bool save(QDomDocument& doc, QDomElement& root) const;

    bool value() const { return _value; }
    operator bool() const {return _value; }

    static dkBool* find(const QString& name);

  public slots:
    void setValue(bool b);

  signals:
    void valueChanged(bool b);

  protected:
    bool _value;
};

// dkStringList
// A list of strings with a single selected index.

class dkStringList : public dkValue
{
    Q_OBJECT
    Q_PROPERTY(int _index READ index WRITE setIndex)

  public:
    dkStringList(const QString& name, const QStringList& choices,
                 dkLocation location = DK_PANEL);

    virtual bool load(QDomElement& root);
    virtual bool save(QDomDocument& doc, QDomElement& root) const;

    int index() const { return _index; }
    const QString& value() const { return _string_list[_index];}
    const QStringList& stringList() const { return _string_list;}
    operator QString() const {return value();}
    bool operator == (const QString& b) { return value() == b; }
    bool operator != (const QString& b) { return value() != b; }
    QByteArray toLocal8Bit() const { return value().toLocal8Bit(); }

    static dkStringList* find(const QString& name);

  public slots:
    void setIndex(int index);

  signals:
    void indexChanged(int index);

  protected:
    int _index;
    QStringList _string_list;
    QSignalMapper *_signal_mapper;

  friend class DialsAndKnobs;
};

// dkImageBrowser
// A gallery of thumbnail images with the ability to pick a single image.

class dkImageBrowser : public dkValue
{
    Q_OBJECT

  public:
    dkImageBrowser(const QString& name, const QString& dir);

    virtual bool load(QDomElement& root);
    virtual bool save(QDomDocument& doc, QDomElement& root) const;

    QString filename() const;
    operator QString() const {return filename();}
    bool operator == (const QString& b) { return filename() == b; }
    bool operator != (const QString& b) { return filename() != b; }
    QByteArray toLocal8Bit() const { return filename().toLocal8Bit(); }

    void setModelAndView(QFileSystemModel* model, QListView* view);

    static dkImageBrowser* find(const QString& name);

  public slots:
    void itemClicked(const QModelIndex& index);

  signals:
    void selectionChanged(const QModelIndex& index);

  protected:
    QString     _root_dir;
    QFileSystemModel* _model;
    QListView* _view;

  friend class DialsAndKnobs;
};

// DialsAndKnobs
// Holds pointers to each value, but *does not* own the pointers.
// Owns widgets for all the values.
class DialsAndKnobs : public QDockWidget
{
    Q_OBJECT

  public:
    DialsAndKnobs(QMainWindow* parent);
    ~DialsAndKnobs();
    virtual bool event(QEvent* e);

    bool load(const QDomElement& root);
    bool save(QDomDocument& doc, QDomElement& root) const;
    QDomElement domElement(const QString& name, QDomDocument& doc) const;

    int frameCounter() const { return _frame_counter; }
    void incrementFrameCounter() { _frame_counter++; }

    static DialsAndKnobs* instance() { return _instance; }

  signals:
    void dataChanged();

  protected slots:
    void updateLayout();
    void dkValueChanged();

  protected:
    void addFloatWidgets(dkFloat* dk_float);
    void addIntWidgets(dkInt* dk_int);
    void addBoolWidgets(dkBool* dk_bool);
    void addStringListWidgets(dkStringList* dk_string_list);
    void addImageBrowserWidgets(dkImageBrowser* dk_image_browser);
    
    QMenu* findOrCreateMenu(const QString& group);
    QGridLayout* findOrCreateLayout(const QString& group);

    QString splitGroup(const QString& path);
    QString splitBase(const QString& path);

  protected:
    QWidget _root_widget;
    QGridLayout* _root_layout;
    QMenuBar* _parent_menu_bar;
    QHash<QString, QMenu*> _menus;
    QHash<QString, QGridLayout*> _layouts;
    bool _in_load;
    int _frame_counter;

    static DialsAndKnobs* _instance;
};

#endif // _DIALS_AND_KNOBS_H_
