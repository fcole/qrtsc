#include "DialsAndKnobs.h"
#include <assert.h>

#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QItemDelegate>
#include <QStandardItem>
#include <QGridLayout>
#include <QLabel>
#include <QEvent>
#include <QCoreApplication>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>

const int UPDATE_LAYOUT_EVENT = QEvent::User + 1;

// dkValue types
dkValue::dkValue(const QString& name, dkLocation location,
                 const QString& group)
{
    _name = name;
    _location = location;
    _group = group;
    DialsAndKnobs::addValue(this);
}

dkValue::~dkValue()
{
    DialsAndKnobs::removeValue(this);
}

dkFloat::dkFloat(const QString& name, double value) 
    : dkValue(name, DK_PANEL, QString())
{
    _value = value;
    _lower = -10e7;
    _upper = 10e7;
    _step_size = 1.0;
}

dkFloat::dkFloat(const QString& name, double value, double lower_limit, 
                 double upper_limit, double step_size) 
    : dkValue(name, DK_PANEL, QString())
{
    _value = value;
    _lower = lower_limit;
    _upper = upper_limit;
    _step_size = step_size;
}

bool dkFloat::load(QDomElement& element)
{
    if (element.tagName() != "float")
        return false;

    bool ret;
    double f = element.attribute("value").toDouble(&ret);
    setValue(f);
    return ret;
}

bool dkFloat::save(QDomDocument& doc, QDomElement& root) const
{
    QDomElement element = doc.createElement("float");
    element.setAttribute("name", _name);
    element.setAttribute("value", _value);
    root.appendChild(element);
    return true;
}

void dkFloat::setValue(double f)
{
    if (_value != f)
    {
        _value = f;
        emit valueChanged(_value);
    }
}

dkInt::dkInt(const QString& name, int value) 
    : dkValue(name, DK_PANEL, QString())
{
    _value = value;
    _lower = -10e7;
    _upper = 10e7;
    _step_size = 1;
}

dkInt::dkInt(const QString& name, int value, int lower_limit, 
                 int upper_limit, int step_size) 
    : dkValue(name, DK_PANEL, QString())
{
    _value = value;
    _lower = lower_limit;
    _upper = upper_limit;
    _step_size = step_size;
}

bool dkInt::load(QDomElement& element)
{
    if (element.tagName() != "int")
        return false;

    bool ret;
    int i = element.attribute("value").toInt(&ret);
    setValue(i);
    return ret;
}

bool dkInt::save(QDomDocument& doc, QDomElement& root) const
{
    QDomElement element = doc.createElement("int");
    element.setAttribute("name", _name);
    element.setAttribute("value", _value);
    root.appendChild(element);
    return true;
}


void dkInt::setValue(int i)
{
    if (_value != i)
    {
        _value = i;
        emit valueChanged(_value);
    }
}

dkBool::dkBool(const QString& name, bool value,
               dkLocation location,
               const QString& group)
    : dkValue(name, location, group)
{
    _value = value;
}

bool dkBool::load(QDomElement& element)
{
    if (element.tagName() != "bool")
        return false;

    bool ret;
    int i = element.attribute("value").toInt(&ret);
    setValue(bool(i));
    return ret;
}

bool dkBool::save(QDomDocument& doc, QDomElement& root) const
{
    QDomElement element = doc.createElement("bool");
    element.setAttribute("name", _name);
    element.setAttribute("value", int(_value));
    root.appendChild(element);
    return true;
}

void dkBool::setValue(bool b)
{
    if (_value != b)
    {
        _value = b;
        emit valueChanged(_value);
    }
}

dkStringList::dkStringList(const QString& name, const QStringList& choices,
                           dkLocation location,
                           const QString& group)
    : dkValue(name, location, group)
{
    _index = 0;
    _string_list = choices;
}

bool dkStringList::load(QDomElement& element)
{
    if (element.tagName() != "string_list")
        return false;

    bool ret;
    int i = element.attribute("index").toInt(&ret);
    if (ret && i >= 0 && i < _string_list.size())
    {
        setIndex(i);
        return true;
    }
    return false;
}

bool dkStringList::save(QDomDocument& doc, QDomElement& root) const
{
    QDomElement element = doc.createElement("string_list");
    element.setAttribute("name", _name);
    element.setAttribute("index", _index);
    root.appendChild(element);
    return true;
}

void dkStringList::setIndex(int i)
{
    if (_index != i)
    {
        _index = i;
        emit indexChanged(_index);
    }
}

bool valueSortedBefore(const dkValue* a, const dkValue* b)
{
    return a->name().toLower() < b->name().toLower();
} 

// DialsAndKnobs
DialsAndKnobs::DialsAndKnobs(QMainWindow* parent) 
    : QDockWidget(tr("Dials and Knobs"), parent)
{
    assert(_instance == NULL);
    _instance = this;

    setWidget(&_root_widget);
    parent->addDockWidget(Qt::RightDockWidgetArea, this);
    setObjectName("dials_and_knobs");
    _parent_menu_bar = parent->menuBar();
    _in_load = false;

    updateLayout();
}

DialsAndKnobs::~DialsAndKnobs()
{
    assert(_instance == this);
    _instance = NULL;
}

void DialsAndKnobs::clear()
{
    _values.clear();
    _values_hash.clear();
    _values_changed = false;
}

bool DialsAndKnobs::event(QEvent* e)
{
    if (e->type() == UPDATE_LAYOUT_EVENT && _values_changed)
    {
        updateLayout();
        return true;
    }
    else
    {
        return QDockWidget::event(e);
    }
}

bool DialsAndKnobs::load(const QDomElement& root)
{
    bool ret = true;
    int num_values_read = 0;
    _in_load = true;

    QDomElement value_e = root.firstChildElement();
    while (!value_e.isNull())
    {
        QString name = value_e.attribute("name");
        if (!_values_hash.contains(name))
        {
            qWarning("DialsAndKnobs::load: don't recognize value %s", 
                    qPrintable(name));
            value_e = value_e.nextSiblingElement();
            ret = false;
            continue;
        }

        dkValue* value_p = _values_hash.value(name);
        if (!value_p->load(value_e))
        {
            qWarning("DialsAndKnobs::load: failed to parse element for %s", 
                     qPrintable(name));
            ret = false;
        }
        num_values_read++;
        value_e = value_e.nextSiblingElement();
    }
    if (num_values_read != _values.size())
    {
        qWarning("DialsAndKnobs::load: read elements for %d of %d values",
                num_values_read, _values.size());
    }
    _in_load = false;
    return ret;
}

bool DialsAndKnobs::save(QDomDocument& doc, QDomElement& root) const
{
    QDomElement e = domElement("dials_and_knobs", doc);
    if (!e.isNull())
    {
        root.appendChild(e);
        return true;
    }
    return false;
}

QDomElement DialsAndKnobs::domElement(const QString& name, 
                                      QDomDocument& doc) const
{
    QDomElement element = doc.createElement(name);
    for (int i = 0; i < _values.size(); i++)
    {
        _values[i]->save(doc, element);
    }
    return element;
}

void DialsAndKnobs::addFloatWidgets(const dkFloat* dk_float, 
                                    QGridLayout* layout, int row)
{
    layout->addWidget(new QLabel(dk_float->name()), row, 0);
    QDoubleSpinBox* spin_box = new QDoubleSpinBox;
    spin_box->setMinimum(dk_float->lowerLimit());
    spin_box->setMaximum(dk_float->upperLimit());
    spin_box->setSingleStep(dk_float->stepSize());
    spin_box->setValue(dk_float->value());
    layout->addWidget(spin_box, row, 1);

    connect(spin_box, SIGNAL(valueChanged(double)), 
            dk_float, SLOT(setValue(double)));
    connect(dk_float, SIGNAL(valueChanged(double)), 
            spin_box, SLOT(setValue(double)));
    connect(dk_float, SIGNAL(valueChanged(double)), 
            this, SLOT(dkValueChanged()));
}

void DialsAndKnobs::addIntWidgets(const dkInt* dk_int, 
                                  QGridLayout* layout, int row)
{
    layout->addWidget(new QLabel(dk_int->name()), row, 0);
    QSpinBox* spin_box = new QSpinBox;
    spin_box->setMinimum(dk_int->lowerLimit());
    spin_box->setMaximum(dk_int->upperLimit());
    spin_box->setSingleStep(dk_int->stepSize());
    spin_box->setValue(dk_int->value());
    layout->addWidget(spin_box, row, 1);
    connect(spin_box, SIGNAL(valueChanged(int)), 
            dk_int, SLOT(setValue(int)));
    connect(dk_int, SIGNAL(valueChanged(int)), 
            spin_box, SLOT(setValue(int)));
    connect(dk_int, SIGNAL(valueChanged(int)), 
            this, SLOT(dkValueChanged()));
}

void DialsAndKnobs::addBoolWidgets(const dkBool* dk_bool, 
                                    QGridLayout* layout, int row)
{
    if (dk_bool->location() == DK_PANEL)
    {
        layout->addWidget(new QLabel(dk_bool->name()), row, 0);
        QCheckBox* check_box = new QCheckBox;
        check_box->setChecked(dk_bool->value());
        layout->addWidget(check_box, row, 1);
        connect(check_box, SIGNAL(toggled(bool)), 
                dk_bool, SLOT(setValue(bool)));
        connect(dk_bool, SIGNAL(valueChanged(bool)), 
                check_box, SLOT(setChecked(bool)));
        connect(dk_bool, SIGNAL(valueChanged(bool)), 
                this, SLOT(dkValueChanged()));
    }
    else
    {
        QMenu* menu = findMenu(dk_bool->group());
        QAction* action = new QAction(dk_bool->name(), 0);
        action->setCheckable(true);
        action->setChecked(dk_bool->value());
        connect(action, SIGNAL(toggled(bool)), 
                dk_bool, SLOT(setValue(bool)));
        connect(dk_bool, SIGNAL(valueChanged(bool)), 
                action, SLOT(setChecked(bool)));
        connect(dk_bool, SIGNAL(valueChanged(bool)), 
                this, SLOT(dkValueChanged()));
        menu->addAction(action);
    }
}

void DialsAndKnobs::addStringListWidgets(const dkStringList* dk_string_list, 
                                   QGridLayout* layout, int row)
{
    layout->addWidget(new QLabel(dk_string_list->name()), row, 0);
    QComboBox* combo_box = new QComboBox;
    combo_box->addItems(dk_string_list->stringList());
    combo_box->setCurrentIndex(dk_string_list->index());
    layout->addWidget(combo_box, row, 1);
    connect(combo_box, SIGNAL(currentIndexChanged(int)), 
        dk_string_list, SLOT(setIndex(int)));
    connect(dk_string_list, SIGNAL(indexChanged(int)), 
        combo_box, SLOT(setCurrentIndex(int)));
    connect(dk_string_list, SIGNAL(indexChanged(int)), 
        this, SLOT(dkValueChanged()));
}

void DialsAndKnobs::updateLayout()
{
    if (_values_changed)
    {
        qSort(_values.begin(), _values.end(), valueSortedBefore);

        if (_root_widget.layout())
            delete _root_widget.layout();

        QVBoxLayout* vbox = new QVBoxLayout;
        QGridLayout* root_layout = new QGridLayout;
        vbox->insertLayout(0, root_layout);
        vbox->addStretch();

        for (int i = 0; i < _values.size(); i++)
        {
            if (qobject_cast<dkFloat*>(_values[i])) 
            {
                addFloatWidgets(qobject_cast<dkFloat*>(_values[i]), 
                                root_layout, i); 
            }
            else if (qobject_cast<dkInt*>(_values[i])) 
            {
                addIntWidgets(qobject_cast<dkInt*>(_values[i]), 
                                root_layout, i); 
            }
            else if (qobject_cast<dkBool*>(_values[i])) 
            {
                addBoolWidgets(qobject_cast<dkBool*>(_values[i]), 
                               root_layout, i); 
            }
            else if (qobject_cast<dkStringList*>(_values[i])) 
            {
                addStringListWidgets(qobject_cast<dkStringList*>(_values[i]), 
                                    root_layout, i); 
            }
            else
            {
                assert(0);
            }
        }

        _root_widget.setLayout(vbox);
        _values_changed = false;
    }
}
    
QMenu* DialsAndKnobs::findMenu(const QString& name)
{
    if (_menus.contains(name))
    {
        return _menus[name];
    }
    else
    {
        QMenu* new_menu = _parent_menu_bar->addMenu(name);
        _menus[name] = new_menu;
        return new_menu;
    }
}

void DialsAndKnobs::addValue(dkValue* value)
{
    // This function can rename dkValues if there is a name
    // collision.
    int name_counter = 1;
    QString old_name = value->_name;
    while (_values_hash.contains(value->_name))
    {
        value->_name = QString("%1 %2").arg(old_name).arg(name_counter);
        name_counter++;
    }
    _values_hash[value->_name] = value;
    _values.append(value);
    _values_changed = true;
    if (_instance)
    {
        QCoreApplication::postEvent(_instance, 
                new QEvent(QEvent::Type(UPDATE_LAYOUT_EVENT)));
    }
}

void DialsAndKnobs::removeValue(dkValue* value)
{
    assert(_values_hash.contains(value->_name));
    _values_hash.remove(value->_name);
    for (int i = 0; i < _values.size(); i++)
    {
        if (_values[i]->_name == value->_name)
        {
            _values.removeAt(i);
            break;
        }
    }
    _values_changed = true;
    if (_instance)
    {
        QCoreApplication::postEvent(_instance, 
                new QEvent(QEvent::Type(UPDATE_LAYOUT_EVENT)));
    }
}

void DialsAndKnobs::dkValueChanged()
{
    if (!_in_load)
        emit dataChanged();
}

QList<dkValue*> DialsAndKnobs::_values;
QHash<QString, dkValue*> DialsAndKnobs::_values_hash;
bool DialsAndKnobs::_values_changed = false;
DialsAndKnobs* DialsAndKnobs::_instance = NULL;
 
