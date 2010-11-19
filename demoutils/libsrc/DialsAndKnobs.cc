#include "DialsAndKnobs.h"
#include <assert.h>

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
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>

#include <DialsAndKnobs_ui.h>

const int UPDATE_LAYOUT_EVENT = QEvent::User + 1;

int DialsAndKnobs::_frame_counter = 0;
DialsAndKnobs* DialsAndKnobs::_instance = NULL;

// dkValue types
dkValue::dkValue(const QString& name, dkLocation location)
{
    _name = name;
    _location = location;
    _last_change_frame_number = 0;
    _is_sticky = false;
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
    if (_last_change_frame_number == DialsAndKnobs::frameCounter() - 1)
        return true;
    
    return false;
}

void dkValue::setSticky(bool sticky)
{
    if (sticky != _is_sticky) {
        _is_sticky = sticky;
        emit stickyChanged(_is_sticky);
    }
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
    DialsAndKnobs::notifyUpdateLayout();
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
    DialsAndKnobs::notifyUpdateLayout();
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
        _last_change_frame_number = DialsAndKnobs::frameCounter();
        
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
        _last_change_frame_number = DialsAndKnobs::frameCounter();
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
        _last_change_frame_number = DialsAndKnobs::frameCounter();
        emit valueChanged(_value);
    }
}

dkBool* dkBool::find(const QString& name)
{
    return qobject_cast<dkBool*>(dkValue::find(name));
}

dkFilename::dkFilename(const QString& name, const QString& value)
    : dkValue(name, DK_PANEL)
{
    _value = value;
}

bool dkFilename::load(QDomElement& element)
{
    if (element.tagName() != "filename")
        return false;

    QString str = element.attribute("value");
    setValue(str);
    return !str.isNull();
}

bool dkFilename::save(QDomDocument& doc, QDomElement& root) const
{
    QDomElement element = doc.createElement("filename");
    element.setAttribute("name", _name);
    element.setAttribute("value", _value);
    root.appendChild(element);
    return true;
}

void dkFilename::setValue(const QString& value)
{
    if (_value != value)
    {
        _value = value;
        _last_change_frame_number = DialsAndKnobs::frameCounter();
        emit valueChanged(_value);
    }
}

dkFilename* dkFilename::find(const QString& name)
{
    return qobject_cast<dkFilename*>(dkValue::find(name));
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
        _last_change_frame_number = DialsAndKnobs::frameCounter();
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
    _last_change_frame_number = DialsAndKnobs::frameCounter();
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

dkText::dkText(const QString& name, int lines, const QString& value)
    : dkValue(name, DK_PANEL)
{
    _value = value;
    _num_lines = lines;
}

bool dkText::load(QDomElement& element)
{
    if (element.tagName() != "text")
        return false;

    QString str = element.text();
    setValue(str);
    return !str.isNull();
}

bool dkText::save(QDomDocument& doc, QDomElement& root) const
{
    QDomElement element = doc.createElement("text");
    element.setAttribute("name", _name);
    QDomText text = doc.createTextNode(_value);
    element.appendChild(text);
    root.appendChild(element);
    return true;
}

void dkText::setValue(const QString& value)
{
    if (_value != value)
    {
        _value = value;
        _last_change_frame_number = DialsAndKnobs::frameCounter();
        emit valueChanged(_value);
    }
}

dkText* dkText::find(const QString& name)
{
    return qobject_cast<dkText*>(dkValue::find(name));
}


// DialsAndKnobs
DialsAndKnobs::DialsAndKnobs(QMainWindow* parent, QMenu* window_menu,
                             QStringList top_categories)
    : QDockWidget(tr("Dials and Knobs"), parent)
{
    assert(_instance == NULL);
    _instance = this;

    DockScrollArea* scroller = new DockScrollArea;
    setWidget(scroller);
    setObjectName("dials_and_knobs");
    _parent_menu_bar = parent->menuBar();
    _parent_window_menu = window_menu;
    _in_load = false;
    _frame_counter = 0;

    for (int i = 0; i < top_categories.size(); i++) {
        QDockWidget* new_dock = new QDockWidget(top_categories[i], parent);
        _dock_widgets[top_categories[i]] = new_dock;
        new_dock->setObjectName(top_categories[i]);
        DockScrollArea* dock_scroller = new DockScrollArea;
        new_dock->setWidget(dock_scroller);
    }

    updateLayout();

    // Only add dock widgets to the mainwindow if they have any
    // widgets in them.
    for (int i = 0; i < top_categories.size(); i++) {
        QDockWidget* dock = _dock_widgets[top_categories[i]];
        if (dock->layout() && dock->layout()->count() > 0) {
            parent->addDockWidget(Qt::RightDockWidgetArea, dock);
            if (_parent_window_menu) {
                _parent_window_menu->addAction(dock->toggleViewAction());
            }
        }
    }

    if (_root_layout && _root_layout->count() > 0) {
        parent->addDockWidget(Qt::RightDockWidgetArea, this);
        if (_parent_window_menu) {
            _parent_window_menu->addAction(this->toggleViewAction());
        }
    }
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

bool DialsAndKnobs::load(const QString& filename)
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly))
	{
		qWarning("Could not open %s", qPrintable(filename));
		return false;
	}
	
	QDomDocument doc("dials_and_knobs");
	QString parse_errors;
	if (!doc.setContent(&file, &parse_errors))
	{
		qWarning("Parse errors: %s", qPrintable(parse_errors));
		return false;
	}
	
	file.close();
	
	QDomElement root = doc.documentElement();
	QDir path = QFileInfo(filename).absoluteDir();
	
	return load(root); 
}

bool DialsAndKnobs::load(const QDomElement& root, bool set_sticky)
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
        
        if (!set_sticky && value_p->isSticky()) {
            value_e = value_e.nextSiblingElement();
            continue;
        } else if (set_sticky) {
            value_p->setSticky(true);
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
    /*if (num_values_read != dkValue::numValues())
    {
        qWarning("DialsAndKnobs::load: read elements for %d of %d values",
                num_values_read, dkValue::numValues());
    }*/
    _in_load = false;
	incrementFrameCounter();
    return ret;
}

bool DialsAndKnobs::save(const QString& filename) const
{
	QDomDocument doc("dials_and_knobs");
	
	QDomElement blank;
	bool ret = save(doc, blank);
    if (!ret)
        return false;
	
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning("Scene::save - Could not save %s", qPrintable(filename));
        return false;
    }
	
    file.write(doc.toByteArray());
	
    file.close();
	
    return true;
}

bool DialsAndKnobs::save(QDomDocument& doc, QDomElement& root, 
                         bool only_sticky, int version) const
{
    QDomElement e = domElement("dials_and_knobs", doc, only_sticky);
    e.setAttribute("version", version);
    if (!e.isNull())
    {
		if (!root.isNull()) {
			root.appendChild(e);
		} else {
			doc.appendChild(e);
		}
        return true;
    }
    return false;
}

QDomElement DialsAndKnobs::domElement(
        const QString& name, QDomDocument& doc, bool only_sticky) const
{
    QDomElement element = doc.createElement(name);
    for (int i = 0; i < dkValue::numValues(); i++)
    {
        dkValue* v = dkValue::values()[i];
        if (only_sticky && !v->isSticky())
            continue;
        v->save(doc, element);
    }
    return element;
}

QByteArray DialsAndKnobs::saveState(int version)
{
    QDomDocument doc("dials_and_knobs");
	
	QDomElement blank;
    QByteArray out;
	bool ret = save(doc, blank, true, version);
    if (ret)
        out = doc.toByteArray(version);
    return out;
}

bool DialsAndKnobs::restoreState(const QByteArray& state, int version)
{
    if (state.isEmpty())
        return false;
    
    QString parse_errors;
    QDomDocument doc("dials_and_knobs");
	if (!doc.setContent(state, &parse_errors))
	{
		qWarning("Parse errors: %s", qPrintable(parse_errors));
		return false;
	}
	
	QDomElement root = doc.documentElement();
    bool ret = false;
    if (root.attribute("version", 0).toInt() == version)
        ret = load(root, true);
    return ret;
}

void DialsAndKnobs::addFloatWidgets(dkFloat* dk_float)
{
    QString base = splitBase(dk_float->name());
    QString group = splitGroup(dk_float->name());
    QGridLayout* layout = findOrCreateLayout(group);
    int row = layout->rowCount();

    layout->addWidget(new ValueLabel(dk_float), row, 0);
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

    layout->addWidget(new ValueLabel(dk_int), row, 0);
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

        layout->addWidget(new ValueLabel(dk_bool), row, 0);
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

	

void DialsAndKnobs::addFilenameWidgets(dkFilename* dk_filename)
{
    QString base = splitBase(dk_filename->name());
    QString group = splitGroup(dk_filename->name());

    QGridLayout* layout = findOrCreateLayout(group);
    int row = layout->rowCount();

    layout->addWidget(new ValueLabel(dk_filename), row, 0);
    QWidget* container = new QWidget;
    QHBoxLayout* line_layout = new QHBoxLayout;

    FileNameLineEdit* filename_box = new FileNameLineEdit(dk_filename->name());
    QPushButton* browse_btn = new QPushButton("...");
    line_layout->addWidget(filename_box);
    line_layout->addWidget(browse_btn);
    container->setLayout(line_layout);

    layout->addWidget(container, row, 1);
	
	connect(browse_btn, SIGNAL(clicked()), 
            filename_box, SLOT(setFromBrowser()));
	connect(filename_box, SIGNAL(editingFinished()), 
            filename_box, SLOT(callUpdateFilename()));
	connect(filename_box, SIGNAL(updateFilename(const QString&)), 
            dk_filename, SLOT(setValue(const QString&)));
	connect(dk_filename, SIGNAL(valueChanged(const QString&)), 
            filename_box, SLOT(setText(const QString&)));
    connect(dk_filename, SIGNAL(valueChanged(const QString&)), 
            this, SLOT(dkValueChanged()));
}

void DialsAndKnobs::addStringListWidgets(dkStringList* dk_string_list)
{
    QString base = splitBase(dk_string_list->name());
    QString group = splitGroup(dk_string_list->name());
    if (dk_string_list->location() == DK_PANEL)
    {
        QGridLayout* layout = findOrCreateLayout(group);
        int row = layout->rowCount();

        layout->addWidget(new ValueLabel(dk_string_list), row, 0);
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

    layout->addWidget(new ValueLabel(dk_image_browser), row, 0);

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

void DialsAndKnobs::addTextWidgets(dkText* dk_text)
{
    QString base = splitBase(dk_text->name());
    QString group = splitGroup(dk_text->name());

    QGridLayout* layout = findOrCreateLayout(group);
    int row = layout->rowCount();

    layout->addWidget(new ValueLabel(dk_text), row, 0);
    UpdatingTextEdit* editor = new UpdatingTextEdit;

    layout->addWidget(editor, row, 1);

	connect(editor, SIGNAL(textChanged()), 
            editor, SLOT(callSendText()));
	connect(editor, SIGNAL(sendText(const QString&)), 
            dk_text, SLOT(setValue(const QString&)));
	connect(dk_text, SIGNAL(valueChanged(const QString&)), 
            editor, SLOT(updateText(const QString&)));
    connect(dk_text, SIGNAL(valueChanged(const QString&)), 
            this, SLOT(dkValueChanged()));
}


bool valueSortedBefore(const dkValue* a, const dkValue* b)
{
    return a->name().toLower() < b->name().toLower();
} 

void DialsAndKnobs::notifyUpdateLayout()
{
    if (_instance)
    {
        QCoreApplication::postEvent(_instance, 
                new QEvent(QEvent::Type(UPDATE_LAYOUT_EVENT)));
    }
}

void DialsAndKnobs::updateLayout()
{
    QList<dkValue*>& values = dkValue::values();
    qSort(values.begin(), values.end(), valueSortedBefore);

    DockScrollArea* root_scroller = qobject_cast<DockScrollArea*>(this->widget());
    if (root_scroller->childLayout())
    {
        delete root_scroller->childLayout();
     
        for (int i = 0; i < _dock_widgets.size(); i++) {
            DockScrollArea* dock_scroller = 
                qobject_cast<DockScrollArea*>(_dock_widgets.values()[i]->widget());
            if (dock_scroller->childLayout())
                delete dock_scroller->childLayout();
        }

        _layouts.clear();
    }

    _root_layout = new QGridLayout;

    for (int i = 0; i < _dock_widgets.size(); i++) {
        QString name = _dock_widgets.values()[i]->windowTitle();
        QGridLayout* new_layout = new QGridLayout;
        _layouts[name] = new_layout;
    }

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
		else if (qobject_cast<dkFilename*>(values[i])) 
        {
            addFilenameWidgets(qobject_cast<dkFilename*>(values[i])); 
        }
        else if (qobject_cast<dkStringList*>(values[i])) 
        {
            addStringListWidgets(qobject_cast<dkStringList*>(values[i])); 
        }
        else if (qobject_cast<dkImageBrowser*>(values[i])) 
        {
            addImageBrowserWidgets(qobject_cast<dkImageBrowser*>(values[i])); 
        }
        else if (qobject_cast<dkText*>(values[i])) 
        {
            addTextWidgets(qobject_cast<dkText*>(values[i])); 
        }
        else
        {
            assert(0);
        }
    }

    root_scroller->setChildLayout(_root_layout);
    for (int i = 0; i < _dock_widgets.size(); i++) {
        QString name = _dock_widgets.values()[i]->windowTitle();
        DockScrollArea* dock_scroller = 
            qobject_cast<DockScrollArea*>(_dock_widgets.values()[i]->widget());
        dock_scroller->setChildLayout(_layouts[name]);
    }
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
            if (_parent_menu_bar) 
                out_menu = _parent_menu_bar->addMenu(group);
            else 
                out_menu = NULL;
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
