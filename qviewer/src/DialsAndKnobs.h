
#ifndef _DIALS_AND_KNOBS_H_
#define _DIALS_AND_KNOBS_H_

#include <QString>
#include <QStringList>
#include <QHash>
#include <QDockWidget>

class QGridLayout;
class QEvent;
class QMenu;
class QMenuBar;

class DialsAndKnobs;

typedef enum
{
    DK_PANEL,
    DK_MENU,
    DK_NUM_LOCATIONS,
} dkLocation;

class dkValue : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString _name READ name)

  public:
    dkValue(const QString& name, dkLocation location,
            const QString& group);
    virtual ~dkValue();
    const QString& name() const { return _name; }
    const QString& group() const { return _group; }
    dkLocation location() const { return _location; }

  protected:
    QString _name;
    QString _group;
    dkLocation _location;

  friend class DialsAndKnobs;
};
     
class dkFloat : public dkValue
{
    Q_OBJECT
    Q_PROPERTY(float value READ value WRITE setValue NOTIFY valueChanged)

  public:
    dkFloat(const QString& name, float value);
    dkFloat(const QString& name, float value, 
            float lower_limit, float upper_limit, 
            float step_size);

    float value() const { return _value; }
    operator float() const {return _value; }

    float lowerLimit() const { return _lower; }
    float upperLimit() const { return _upper; }
    float stepSize() const { return _step_size; }

  public slots:
    void setValue(double f) { _value = float(f); }

  signals:
    void valueChanged(float f);

  protected:
    float _value;
    float _lower, _upper;
    float _step_size;
};

class dkBool : public dkValue
{
    Q_OBJECT
    Q_PROPERTY(bool value READ value WRITE setValue NOTIFY valueChanged)

  public:
    dkBool(const QString& name, bool value, 
           dkLocation location = DK_PANEL,
           const QString& group = QString());

    bool value() const { return _value; }
    operator bool() const {return _value; }

  public slots:
    void setValue(bool b) { _value = b; }

  signals:
    void valueChanged(bool b);

  protected:
    bool _value;
};

class dkStringList : public dkValue
{
    Q_OBJECT
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)

  public:
    dkStringList(const QString& name, const QStringList& choices,
                 dkLocation location = DK_PANEL,
                 const QString& group = QString());

    int index() const { return _index; }
    const QString& value() const { return _string_list[_index];}
    const QStringList& stringList() const { return _string_list;}
    operator QString() const {return value();}

  public slots:
    void setIndex(int index) { _index = index; }

  signals:
    void indexChanged(int index);

  protected:
    int _index;
    QStringList _string_list;
};

// DialsAndKnobs
// Holds pointers to each value, but *does not* own the pointers.
// Owns widgets for all the 
class DialsAndKnobs : public QDockWidget
{
    Q_OBJECT

  public:
    DialsAndKnobs(QMainWindow* parent);
    ~DialsAndKnobs();
	void clear();
    virtual bool event(QEvent* e);

    static void addValue(dkValue* value);
    static void removeValue(dkValue* value);

  signals:
    void dataChanged();

  protected slots:
    void updateLayout();

  protected:
    void addFloatWidgets(const dkFloat* dk_float, 
                         QGridLayout* layout, int row);
    void addBoolWidgets(const dkBool* dk_bool, 
                         QGridLayout* layout, int row);
    void addStringListWidgets(const dkStringList* dk_string_list, 
                         QGridLayout* layout, int row);
    
    QMenu* findMenu(const QString& name);

  protected:
    QWidget _root_widget;
    QMenuBar* _parent_menu_bar;
    QHash<QString, QMenu*> _menus;

    static QList<dkValue*> _values;
    static QHash<QString, dkValue*> _values_hash;
    static bool _values_changed;
    static DialsAndKnobs* _instance;
};

#endif // _DIALS_AND_KNOBS_H_
