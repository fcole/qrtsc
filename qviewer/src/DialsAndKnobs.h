
#ifndef _DIALS_AND_KNOBS_H_
#define _DIALS_AND_KNOBS_H_

#include <QString>
#include <QStringList>
#include <QHash>
#include <QDockWidget>
#include <QDomDocument>

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

    virtual bool load(QDomElement& root) = 0;
    virtual bool save(QDomDocument& doc, QDomElement& root) const = 0;

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
    Q_PROPERTY(double value READ value WRITE setValue)

  public:
    dkFloat(const QString& name, double value);
    dkFloat(const QString& name, double value, 
            double lower_limit, double upper_limit, 
            double step_size);

    virtual bool load(QDomElement& root);
    virtual bool save(QDomDocument& doc, QDomElement& root) const;

    double value() const { return _value; }
    operator float() const {return float(_value); }
    operator double() const {return _value; }

    double lowerLimit() const { return _lower; }
    double upperLimit() const { return _upper; }
    double stepSize() const { return _step_size; }

  public slots:
    void setValue(double f);

  signals:
    void valueChanged(double f);

  protected:
    double _value;
    double _lower, _upper;
    double _step_size;
};

class dkInt : public dkValue
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)

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

  public slots:
    void setValue(int i);

  signals:
    void valueChanged(int f);

  protected:
    int _value;
    int _lower, _upper;
    int _step_size;
};


class dkBool : public dkValue
{
    Q_OBJECT
    Q_PROPERTY(bool value READ value WRITE setValue)

  public:
    dkBool(const QString& name, bool value, 
           dkLocation location = DK_PANEL,
           const QString& group = QString());

    virtual bool load(QDomElement& root);
    virtual bool save(QDomDocument& doc, QDomElement& root) const;

    bool value() const { return _value; }
    operator bool() const {return _value; }

  public slots:
    void setValue(bool b);

  signals:
    void valueChanged(bool b);

  protected:
    bool _value;
};

class dkStringList : public dkValue
{
    Q_OBJECT
    Q_PROPERTY(int index READ index WRITE setIndex)

  public:
    dkStringList(const QString& name, const QStringList& choices,
                 dkLocation location = DK_PANEL,
                 const QString& group = QString());

    virtual bool load(QDomElement& root);
    virtual bool save(QDomDocument& doc, QDomElement& root) const;

    int index() const { return _index; }
    const QString& value() const { return _string_list[_index];}
    const QStringList& stringList() const { return _string_list;}
    operator QString() const {return value();}
    bool operator == (const QString& b) { return value() == b; }

  public slots:
    void setIndex(int index);

  signals:
    void indexChanged(int index);

  protected:
    int _index;
    QStringList _string_list;
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
	void clear();
    virtual bool event(QEvent* e);

    bool load(const QDomElement& root);
    bool save(QDomDocument& doc, QDomElement& root) const;
    QDomElement domElement(const QString& name, QDomDocument& doc) const;

    static void addValue(dkValue* value);
    static void removeValue(dkValue* value);

  signals:
    void dataChanged();

  protected slots:
    void updateLayout();
    void dkValueChanged();

  protected:
    void addFloatWidgets(const dkFloat* dk_float, 
                         QGridLayout* layout, int row);
    void addIntWidgets(const dkInt* dk_int, 
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
    bool _in_load;

    static QList<dkValue*> _values;
    static QHash<QString, dkValue*> _values_hash;
    static bool _values_changed;
    static DialsAndKnobs* _instance;
};

#endif // _DIALS_AND_KNOBS_H_
