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

dkFloat::dkFloat(const QString& name, float value) 
    : dkValue(name, DK_PANEL, QString())
{
    _value = value;
    _lower = -10e7;
    _upper = 10e7;
    _step_size = 1.0;
}

dkFloat::dkFloat(const QString& name, float value, float lower_limit, 
                 float upper_limit, float step_size) 
    : dkValue(name, DK_PANEL, QString())
{
    _value = value;
    _lower = lower_limit;
    _upper = upper_limit;
    _step_size = step_size;
}

dkBool::dkBool(const QString& name, bool value,
               dkLocation location,
               const QString& group)
    : dkValue(name, location, group)
{
    _value = value;
}

dkStringList::dkStringList(const QString& name, const QStringList& choices,
                           dkLocation location,
                           const QString& group)
    : dkValue(name, location, group)
{
    _index = 0;
    _string_list = choices;
}


bool valueSortedBefore(const dkValue* a, const dkValue* b)
{
    return a->name().toLower() < b->name().toLower();
} 

// DialsAndKnobs
DialsAndKnobs::DialsAndKnobs(QMainWindow* parent) : QDockWidget(tr("Dials and Knobs"), parent)
{
    assert(_instance == NULL);
    _instance = this;

    setWidget(&_root_widget);
    parent->addDockWidget(Qt::RightDockWidgetArea, this);
    setObjectName("dials_and_knobs");
    _parent_menu_bar = parent->menuBar();

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
    connect(spin_box, SIGNAL(valueChanged(double)), 
            dk_float, SIGNAL(valueChanged(double)));
    connect(spin_box, SIGNAL(valueChanged(double)), 
            this, SIGNAL(dataChanged()));
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
        connect(check_box, SIGNAL(toggled(bool)), 
                dk_bool, SIGNAL(valueChanged(bool)));
        connect(check_box, SIGNAL(toggled(bool)), 
                this, SIGNAL(dataChanged()));
    }
    else
    {
        QMenu* menu = findMenu(dk_bool->group());
        QAction* action = new QAction(dk_bool->name(), 0);
        action->setCheckable(true);
        action->setChecked(dk_bool->value());
        connect(action, SIGNAL(toggled(bool)), 
                dk_bool, SLOT(setValue(bool)));
        connect(action, SIGNAL(toggled(bool)), 
                dk_bool, SIGNAL(valueChanged(bool)));
        connect(action, SIGNAL(toggled(bool)), 
                this, SIGNAL(dataChanged()));
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
    connect(combo_box, SIGNAL(currentIndexChanged(int)), 
        dk_string_list, SIGNAL(indexChanged(int)));
    connect(combo_box, SIGNAL(currentIndexChanged(int)), 
        this, SIGNAL(dataChanged()));
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
                addFloatWidgets(qobject_cast<dkFloat*>(_values[i]), root_layout, i); 
            }
            else if (qobject_cast<dkBool*>(_values[i])) 
            {
                addBoolWidgets(qobject_cast<dkBool*>(_values[i]), root_layout, i); 
            }
            else if (qobject_cast<dkStringList*>(_values[i])) 
            {
                addStringListWidgets(qobject_cast<dkStringList*>(_values[i]), root_layout, i); 
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
        QCoreApplication::postEvent(_instance, new QEvent(QEvent::Type(UPDATE_LAYOUT_EVENT)));
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
        QCoreApplication::postEvent(_instance, new QEvent(QEvent::Type(UPDATE_LAYOUT_EVENT)));
}


QList<dkValue*> DialsAndKnobs::_values;
QHash<QString, dkValue*> DialsAndKnobs::_values_hash;
bool DialsAndKnobs::_values_changed = false;
DialsAndKnobs* DialsAndKnobs::_instance = NULL;
 
