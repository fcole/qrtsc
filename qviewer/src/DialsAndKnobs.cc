#include "DialsAndKnobs.h"
#include <assert.h>

#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QItemDelegate>
#include <QStandardItem>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QEvent>
#include <QCoreApplication>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QListView>
#include <QFileSystemModel>

const int UPDATE_LAYOUT_EVENT = QEvent::User + 1;

DialsAndKnobs* DialsAndKnobs::_instance = NULL;

// dkValue types
dkValue::dkValue(const QString& name, dkLocation location)
{
    _name = name;
    _location = location;
    _last_change_frame_number = 0;
    add(this);
}

dkValue::~dkValue()
{
    remove(this);
}

// These functions are necessary to make sure the values list
// always exists when a dkValue is created.
QList<dkValue*>& dkValue::values()
{
    static QList<dkValue*>* list = new QList<dkValue*>;
    return *list;
}
QHash<QString, dkValue*>& dkValue::values_hash()
{
    static QHash<QString, dkValue*>* table = new QHash<QString, dkValue*>;
    return *table;
}

bool dkValue::changedLastFrame() const
{
    if (DialsAndKnobs::instance() &&
        _last_change_frame_number == 
            DialsAndKnobs::instance()->frameCounter() - 1)
        return true;
    
    return false;
}

void dkValue::add(dkValue* value)
{
    // This function can rename dkValues if there is a name
    // collision.
    int name_counter = 1;
    QString old_name = value->_name;
    while (values_hash().contains(value->_name))
    {
        value->_name = QString("%1 %2").arg(old_name).arg(name_counter);
        name_counter++;
    }
    values_hash()[value->_name] = value;
    values().append(value);
    if (DialsAndKnobs::instance())
    {
        QCoreApplication::postEvent(DialsAndKnobs::instance(), 
                new QEvent(QEvent::Type(UPDATE_LAYOUT_EVENT)));
    }
}

void dkValue::remove(dkValue* value)
{
    assert(values_hash().contains(value->_name));
    values_hash().remove(value->_name);
    for (int i = 0; i < values().size(); i++)
    {
        if (values()[i]->_name == value->_name)
        {
            values().removeAt(i);
            break;
        }
    }
    if (DialsAndKnobs::instance())
    {
        QCoreApplication::postEvent(DialsAndKnobs::instance(), 
                new QEvent(QEvent::Type(UPDATE_LAYOUT_EVENT)));
    }
}

dkValue* dkValue::find(const QString& name)
{
    return values_hash().value(name);
}

dkFloat::dkFloat(const QString& name, double value) 
    : dkValue(name, DK_PANEL)
{
    _value = value;
    _lower = -10e7;
    _upper = 10e7;
    _step_size = 1.0;
}

dkFloat::dkFloat(const QString& name, double value, double lower_limit, 
                 double upper_limit, double step_size) 
    : dkValue(name, DK_PANEL)
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
        if (DialsAndKnobs::instance())
            _last_change_frame_number = 
                DialsAndKnobs::instance()->frameCounter();
        
        emit valueChanged(_value);
    }
}

dkFloat* dkFloat::find(const QString& name)
{
    // Will return 0 if the value is not a float.
    return qobject_cast<dkFloat*>(dkValue::find(name));
}

dkInt::dkInt(const QString& name, int value) 
    : dkValue(name, DK_PANEL)
{
    _value = value;
    _lower = -10e7;
    _upper = 10e7;
    _step_size = 1;
}

dkInt::dkInt(const QString& name, int value, int lower_limit, 
                 int upper_limit, int step_size) 
    : dkValue(name, DK_PANEL)
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
        if (DialsAndKnobs::instance())
            _last_change_frame_number = 
                DialsAndKnobs::instance()->frameCounter();
        emit valueChanged(_value);
    }
}

dkInt* dkInt::find(const QString& name)
{
    return qobject_cast<dkInt*>(dkValue::find(name));
}

dkBool::dkBool(const QString& name, bool value,
               dkLocation location)
    : dkValue(name, location)
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
        if (DialsAndKnobs::instance())
            _last_change_frame_number = 
                DialsAndKnobs::instance()->frameCounter();
        emit valueChanged(_value);
    }
}

dkBool* dkBool::find(const QString& name)
{
    return qobject_cast<dkBool*>(dkValue::find(name));
}

dkStringList::dkStringList(const QString& name, const QStringList& choices,
                           dkLocation location)
    : dkValue(name, location)
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
        if (DialsAndKnobs::instance())
            _last_change_frame_number = 
                DialsAndKnobs::instance()->frameCounter();
        emit indexChanged(_index);
    }
}

dkStringList* dkStringList::find(const QString& name)
{
    return qobject_cast<dkStringList*>(dkValue::find(name));
}

dkImageBrowser::dkImageBrowser(const QString& name, const QString& dir)
    : dkValue(name, DK_PANEL)
{
    _root_dir = dir;
    _model = NULL;
    _view = NULL;
}

bool dkImageBrowser::load(QDomElement& element)
{
    if (element.tagName() != "image_browser")
        return false;

    return true;
}

bool dkImageBrowser::save(QDomDocument& doc, QDomElement& root) const
{
    QDomElement element = doc.createElement("image_browser");
    root.appendChild(element);
    return true;
}

QString dkImageBrowser::filename() const
{
    if (_view && _view->currentIndex().isValid() && 
        !_model->isDir(_view->currentIndex()))
        return _model->filePath(_view->currentIndex());

    return QString();
}

void dkImageBrowser::itemClicked(const QModelIndex& index)
{
    if (DialsAndKnobs::instance())
        _last_change_frame_number = 
            DialsAndKnobs::instance()->frameCounter();
    emit selectionChanged(index);
}
         
void dkImageBrowser::setModelAndView(QFileSystemModel* model, QListView* view)
{
    _model = model;
    _view = view;
    _model->setRootPath(_root_dir);
}

dkImageBrowser* dkImageBrowser::find(const QString& name)
{
    return qobject_cast<dkImageBrowser*>(dkValue::find(name));
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
    _frame_counter = 0;

    updateLayout();
}

DialsAndKnobs::~DialsAndKnobs()
{
    assert(_instance == this);
    _instance = NULL;
}

bool DialsAndKnobs::event(QEvent* e)
{
    if (e->type() == UPDATE_LAYOUT_EVENT)
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
        dkValue* value_p = dkValue::find(name);
        if (!value_p)
        {
            qWarning("DialsAndKnobs::load: don't recognize value %s", 
                    qPrintable(name));
            value_e = value_e.nextSiblingElement();
            ret = false;
            continue;
        }

        if (!value_p->load(value_e))
        {
            qWarning("DialsAndKnobs::load: failed to parse element for %s", 
                     qPrintable(name));
            ret = false;
        }
        num_values_read++;
        value_e = value_e.nextSiblingElement();
    }
    if (num_values_read != dkValue::numValues())
    {
        qWarning("DialsAndKnobs::load: read elements for %d of %d values",
                num_values_read, dkValue::numValues());
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
    for (int i = 0; i < dkValue::numValues(); i++)
    {
        dkValue::values()[i]->save(doc, element);
    }
    return element;
}

class ArbitraryPrecisionSpinBox : public QDoubleSpinBox
{
  public:
    ArbitraryPrecisionSpinBox(QWidget* parent = 0) : 
        QDoubleSpinBox(parent)
    {
        setDecimals(15);
        setKeyboardTracking(false);
    }

    virtual QString textFromValue( double value ) const
    {
        QString str = QDoubleSpinBox::textFromValue(value);
        int last_non_zero = str.lastIndexOf(QRegExp("[^0]"));
        if (str[last_non_zero] == QWidget::locale().decimalPoint())
            last_non_zero--;
        return str.left(last_non_zero+1);
    }
};

void DialsAndKnobs::addFloatWidgets(dkFloat* dk_float)
{
    QString base = splitBase(dk_float->name());
    QString group = splitGroup(dk_float->name());
    QGridLayout* layout = findOrCreateLayout(group);
    int row = layout->rowCount();

    layout->addWidget(new QLabel(base), row, 0);
    ArbitraryPrecisionSpinBox* spin_box = new ArbitraryPrecisionSpinBox;
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

void DialsAndKnobs::addIntWidgets(dkInt* dk_int)
{
    QString base = splitBase(dk_int->name());
    QString group = splitGroup(dk_int->name());
    QGridLayout* layout = findOrCreateLayout(group);
    int row = layout->rowCount();

    layout->addWidget(new QLabel(base), row, 0);
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

void DialsAndKnobs::addBoolWidgets(dkBool* dk_bool)
{
    QString base = splitBase(dk_bool->name());
    QString group = splitGroup(dk_bool->name());
    if (dk_bool->location() == DK_PANEL)
    {
        QGridLayout* layout = findOrCreateLayout(group);
        int row = layout->rowCount();

        layout->addWidget(new QLabel(base), row, 0);
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
        QMenu* menu;
        if (group.isEmpty())
            menu = findOrCreateMenu(base);
        else
            menu = findOrCreateMenu(group);
        QAction* action = new QAction(base, 0);
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

void DialsAndKnobs::addStringListWidgets(dkStringList* dk_string_list)
{
    QString base = splitBase(dk_string_list->name());
    QString group = splitGroup(dk_string_list->name());
    if (dk_string_list->location() == DK_PANEL)
    {
        QGridLayout* layout = findOrCreateLayout(group);
        int row = layout->rowCount();

        layout->addWidget(new QLabel(base), row, 0);
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
    else
    {
        // Note: This currently doesn't support updating the
        // action group from code.

        QMenu* group_menu;
        if (group.isEmpty())
            group_menu = findOrCreateMenu(base);
        else
            group_menu = findOrCreateMenu(group);
        QMenu* menu = new QMenu(base);
        group_menu->addMenu(menu);
        QActionGroup* action_group = new QActionGroup(this);
        dk_string_list->_signal_mapper = new QSignalMapper(this);
        for (int i = 0; i < dk_string_list->stringList().size(); i++)
        {
            QAction* action = 
                new QAction(dk_string_list->stringList()[i], action_group);
            action->setCheckable(true);
            menu->addAction(action);
            if (i == 0) 
                action->setChecked(true);
            dk_string_list->_signal_mapper->setMapping(action, i);
            connect(action, SIGNAL(triggered()), 
                    dk_string_list->_signal_mapper, SLOT(map()));
        }
        connect(dk_string_list->_signal_mapper, SIGNAL(mapped(int)),
                dk_string_list, SLOT(setIndex(int)));
        connect(dk_string_list->_signal_mapper, SIGNAL(mapped(int)),
                this, SLOT(dkValueChanged()));
    }
}

void DialsAndKnobs::addImageBrowserWidgets(dkImageBrowser* dk_image_browser)
{

    QString base = splitBase(dk_image_browser->name());
    QString group = splitGroup(dk_image_browser->name());

    QGridLayout* layout = findOrCreateLayout(group);
    int row = layout->rowCount();

    layout->addWidget(new QLabel(base), row, 0);

    // Might want to make this model deallocate somehow, but it's tricky...
    QFileSystemModel* model = new QFileSystemModel;
    QListView* list_view = new QListView;
    list_view->setModel(model);
    list_view->setRootIndex(model->index(dk_image_browser->_root_dir));
    list_view->setViewMode(QListView::IconMode);
    layout->addWidget(list_view, row+1, 0, 1, 2);

    dk_image_browser->setModelAndView(model, list_view);

    connect(list_view, SIGNAL(clicked(const QModelIndex&)), 
            dk_image_browser, SLOT(itemClicked(const QModelIndex&)));
    connect(dk_image_browser, SIGNAL(selectionChanged(const QModelIndex&)), 
        this, SLOT(dkValueChanged()));
}

bool valueSortedBefore(const dkValue* a, const dkValue* b)
{
    return a->name().toLower() < b->name().toLower();
} 

void DialsAndKnobs::updateLayout()
{
    QList<dkValue*>& values = dkValue::values();
    qSort(values.begin(), values.end(), valueSortedBefore);

    if (_root_widget.layout())
    {
        delete _root_widget.layout();
        _layouts.clear();
    }

    QVBoxLayout* vbox = new QVBoxLayout;
    _root_layout = new QGridLayout;
    vbox->insertLayout(0, _root_layout);

    for (int i = 0; i < values.size(); i++)
    {
        if (qobject_cast<dkFloat*>(values[i])) 
        {
            addFloatWidgets(qobject_cast<dkFloat*>(values[i])); 
        }
        else if (qobject_cast<dkInt*>(values[i])) 
        {
            addIntWidgets(qobject_cast<dkInt*>(values[i])); 
        }
        else if (qobject_cast<dkBool*>(values[i])) 
        {
            addBoolWidgets(qobject_cast<dkBool*>(values[i])); 
        }
        else if (qobject_cast<dkStringList*>(values[i])) 
        {
            addStringListWidgets(qobject_cast<dkStringList*>(values[i])); 
        }
        else if (qobject_cast<dkImageBrowser*>(values[i])) 
        {
            addImageBrowserWidgets(qobject_cast<dkImageBrowser*>(values[i])); 
        }
        else
        {
            assert(0);
        }
    }

    vbox->addStretch();

    _root_widget.setLayout(vbox);
}
    
QMenu* DialsAndKnobs::findOrCreateMenu(const QString& group)
{
    QMenu* out_menu;
    if (_menus.contains(group))
    {
        out_menu = _menus[group];
    }
    else
    {
        QString parent = splitGroup(group);
        if (parent.isEmpty())
        {
            out_menu = _parent_menu_bar->addMenu(group);
        }
        else
        {
            QMenu* menu = findOrCreateMenu(parent);
            out_menu = menu->addMenu(splitBase(group));
        }
        _menus[group] = out_menu;
    }
    return out_menu;
}

QGridLayout* DialsAndKnobs::findOrCreateLayout(const QString& group)
{
    QGridLayout* out_layout;
    if (_layouts.contains(group))
    {
        out_layout = _layouts[group];
    }
    else
    {
        if (group.isEmpty())
        {
            out_layout = _root_layout;
        }
        else
        {
            QGroupBox* new_box = new QGroupBox(splitBase(group));
            out_layout = new QGridLayout;
            new_box->setLayout(out_layout);
            _layouts[group] = out_layout;

            QGridLayout* parent_layout = findOrCreateLayout(splitGroup(group));
            int row = parent_layout->rowCount();
            parent_layout->addWidget(new_box, row, 0, 1, 2);
        }
    }
    return out_layout;
}

void DialsAndKnobs::dkValueChanged()
{
    if (!_in_load)
        emit dataChanged();
}

QString DialsAndKnobs::splitGroup(const QString& path)
{
    QStringList tokens = path.split("->");
    tokens.removeLast();
    return tokens.join("->");
}

QString DialsAndKnobs::splitBase(const QString& path)
{
    QStringList tokens = path.split("->");
    return tokens.last();
}
