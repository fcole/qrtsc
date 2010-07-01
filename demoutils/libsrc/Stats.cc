/*****************************************************************************\

Stats.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

demoutils is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "Stats.h"
#include "timestamp.h"
#include <assert.h>
#include <QTreeView>
#include <QMainWindow>
#include <QMenu>

Stats Stats::_global_instance;

bool Stats::init()
{
    _dummy_root.children.clear();

    for (int i = 0; i < NUM_CATEGORIES; i++)
    {
        _headers[i].children.clear();
        _headers[i].parent = &_dummy_root;
        _dummy_root.children.append(&_headers[i]);
    }
    _headers[TIMER].name = "Timers";
    _headers[TIMER].category = TIMER;

    _headers[COUNTER].name = "Counters";
    _headers[COUNTER].category = COUNTER;

    _headers[CONSTANT].name = "Constants";
    _headers[CONSTANT].category = CONSTANT;

    clear();

    return true;
}

void Stats::clear()
{
    for (int i = 0; i < NUM_CATEGORIES; i++)
    {
        _records[i].clear();
        _headers[i].children.clear();
    }

    _timer_stack.clear();
    _constant_stack.clear();

    _layout_changed = false;
    _data_changed = false;
    QAbstractItemModel::reset();
}

void Stats::reset()
{
    //assert( _timer_stack.size() == 0 );
    _timer_stack.clear();

    // remove any timers or counters that were not used since the last reset.

    // iterate over timers in reverse so that children are removed before parents
    for (int i = _records[TIMER].size()-1; i >= 0; i--)
    {
        Record& timer = _records[TIMER][i];
        if (timer.touches_since_last_reset == 0)
        {
            removeTimer(i);
        }
        else
        {
            timer.value = 0;
            timer.touches_since_last_reset = 0;
        }
    }

    for (int i = _records[COUNTER].size()-1; i >= 0; i--)
    {
        Record& counter = _records[COUNTER][i];
        if (counter.touches_since_last_reset == 0)
        {
            removeCounter(i);
        }
        else
        {
            counter.value = 0;
            counter.touches_since_last_reset = 0;
        }
    }

    _data_changed = true;
}

void Stats::updateView()
{
    if (_layout_changed)
    {
        QAbstractItemModel::reset();
        _layout_changed = false;
        _data_changed = false;
    }
    else if (_data_changed)
    {
        // I don't really understand how the dataChanged signal works.
        // This updates everything, but it seems like it should just 
        // update the counters.
        QModelIndex a = createIndex(0,0,&_headers[COUNTER]);
        QModelIndex b = createIndex(numCounters()-1,1,&_headers[COUNTER]);

        emit dataChanged(a,b);
        _data_changed = false;
    }
}
        
void Stats::setChildValuesToZero( Record* record )
{
    for (int i = 0; i < record->children.size(); i++)
    {
        Record* child = record->children[i];
        child->value = 0;
        setChildValuesToZero(child);
    }
}

int Stats::findTimer( const QString& name, const Record* parent )
{
    int index = -1;
    for (int i = 0; i < _records[TIMER].size(); i++)
    {
        Record& timer = _records[TIMER][i];
        if (timer.name == name &&
            (parent == 0 || timer.parent == parent))
        {
            index = i;
            break;
        }
    }
    return index;
}

int Stats::findTimer( const Record* pointer )
{
    int index = -1;
    for (int i = 0; i < _records[TIMER].size(); i++)
    {
        if (&(_records[TIMER][i]) == pointer)
        {
            index = i;
            break;
        }
    }
    return index;
}

int Stats::findCounter( const QString& name )
{
    int index = -1;
    for (int i = 0; i < _records[COUNTER].size(); i++)
    {
        if (_records[COUNTER][i].name == name)
        {
            index = i;
            break;
        }
    }
    return index;
}

void Stats::removeTimer( int which )
{
    Record* timer = &(_records[TIMER][which]);
    assert( timer->children.size() == 0 );
    Record* parent = &_headers[TIMER];
    if (timer->parent >= 0)
    {
        parent = timer->parent;
    }

    for (int i = 0; i < parent->children.size(); i++)
    {
        if (parent->children[i] == timer)
        {
            parent->children.removeAt(i);
            _layout_changed = true;
            break;
        }
    }

    _records[TIMER].removeAt(which);
}
    
void Stats::removeCounter( int which )
{
    Record* parent = &_headers[COUNTER];
    for (int i = 0; i < parent->children.size(); i++)
    {
        if (parent->children[i] == &(_records[COUNTER][which]))
        {
            parent->children.removeAt(i);
            _layout_changed = true;
            break;
        }
    }

    _records[COUNTER].removeAt(which);
}

void Stats::startTimer( const QString& name )
{
    int index = findTimer(name, _timer_stack.isEmpty() ? 0 : _timer_stack.last());  
    if (index == -1)
    {
        index = _records[TIMER].size();
        Record newtimer;
        newtimer.name = name;
        newtimer.category = TIMER;
        newtimer.stamp = now();
        newtimer.value = 0;
        _records[TIMER].append(newtimer);
        Record* pointer = &(_records[TIMER].last());

        if (_timer_stack.size() > 0)
        {
            assert( findTimer(_timer_stack.last()) < findTimer(pointer) );
            pointer->parent = _timer_stack.last();
            _timer_stack.last()->children.append(pointer);
        }
        else
        {
            pointer->parent = &_headers[TIMER];
            _headers[TIMER].children.append(pointer);
        }

        _layout_changed = true;
    }
    else
    {
        _records[TIMER][index].stamp = now();
    }

    _records[TIMER][index].touches_since_last_reset++;
    _timer_stack.append(&(_records[TIMER][index]));
}

void Stats::stopTimer( const QString& name )
{
#ifdef QT_NO_DEBUG
    Q_UNUSED(name);
#endif
    assert( _timer_stack.size() > 0 && _timer_stack.last()->name == name );
    Record* rec = _timer_stack.last();

    rec->value += now() - rec->stamp;
    _data_changed = true;

    _timer_stack.removeLast();
}

void Stats::setCounter( const QString& name, float value )
{
    int index = findCounter(name);  
    if (index == -1)
    {
        index = _records[COUNTER].size();
        Record newcounter;
        newcounter.name = name;
        newcounter.category = COUNTER;
        newcounter.value = value;
        newcounter.parent = &_headers[COUNTER];

        _records[COUNTER].append(newcounter);
        _headers[COUNTER].children.append(&(_records[COUNTER][index]));

        _layout_changed = true;
    }
    else
    {
        _records[COUNTER][index].value = value;
        _data_changed = true;
    }
    _records[COUNTER][index].touches_since_last_reset++;
}

void Stats::addToCounter( const QString& name, float value )
{
    int index = findCounter(name);  
    if (index == -1)
    {
        index = _records[COUNTER].size();
        Record newcounter;
        newcounter.name = name;
        newcounter.category = COUNTER;
        newcounter.value = value;
        newcounter.parent = &_headers[COUNTER];

        _records[COUNTER].append(newcounter);
        _headers[COUNTER].children.append(&(_records[COUNTER][index]));

        _layout_changed = true;
    }
    else
    {
        _records[COUNTER][index].value += value;
        _data_changed = true;
    }
    _records[COUNTER][index].touches_since_last_reset++;
}

void Stats::beginConstantGroup( const QString& name )
{
    Record newgroup;
    newgroup.name = name;
    newgroup.category = CONSTANT;
    _records[CONSTANT].append(newgroup);
    Record* ptr = &_records[CONSTANT].last();

    if (_constant_stack.size() > 0)
    {
        ptr->parent = _constant_stack.last();
        _constant_stack.last()->children.append(ptr);
    }
    else
    {
        ptr->parent = &_headers[CONSTANT];
        _headers[CONSTANT].children.append(ptr);
    }
    _constant_stack.append(ptr);

    _layout_changed = true;
}
void Stats::setConstant( const QString& name, float value )
{
    setConstant(name, QString::number(value));
}

void Stats::setConstant( const QString& name, const QString& value )
{
    Record* parent = &_headers[CONSTANT];
    if (_constant_stack.size() > 0)
        parent = _constant_stack.last();

    bool found = false;
    for (int i = 0; i < parent->children.size(); i++)
    {
        if (parent->children[i]->name == name)
        {
            parent->children[i]->str_value = value;
            found = true;
            break;
        }
    }

    if (!found)
    {
        Record constant;
        constant.name = name;
        constant.category = CONSTANT;
        constant.str_value = value;
        _records[CONSTANT].append(constant);
        Record* ptr = &_records[CONSTANT].last();

        if (_constant_stack.size() > 0)
        {
            ptr->parent = _constant_stack.last();
            _constant_stack.last()->children.append(ptr);
        }
        else
        {
            ptr->parent = &_headers[CONSTANT];
            _headers[CONSTANT].children.append(ptr);
        }

        _layout_changed = true;
    }
    _data_changed = true;
}

void Stats::endConstantGroup()
{
    assert( _constant_stack.size() > 0 );
    _constant_stack.removeLast();
}


QString Stats::timerStatistics( const QString& name )
{
    int timer_index = findTimer(name, 0);
    if (timer_index < 0)
    {
        timer_index = findTimer(name + " (wait GL)", 0);
        if (timer_index < 0)
            return QString("Timer not found (%1).").arg(name);
    }

    const Record& timer = _records[TIMER][timer_index];
    float average = 0;
    if (timer.touches_since_last_reset > 0)
        average = timer.value / (float)timer.touches_since_last_reset;
    QString output = QString("%1: %2 total time, %3 calls, %4 average").
        arg(name).arg(timer.value).arg(timer.touches_since_last_reset).arg(average);
    return output;
}


// QAbstractItemModel implementation

QVariant Stats::data( const QModelIndex& index, int role ) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    Record* node = static_cast<Record*>(index.internalPointer());

    QVariant ret;
    if (index.column() == 0)
        ret = node->name;
    else
    {
        if (node->parent == &_dummy_root)
        {
            ret = QString();
        }
        else if (node->category == TIMER)
        {
            ret = QString::number(node->value, 'f', 3);
        }
        else if (node->category == COUNTER)
        {
            ret = node->value;
        }
        else if (node->category == CONSTANT)
        {
            ret = node->str_value;
        }
    }

    return ret;
}

Qt::ItemFlags Stats::flags( const QModelIndex& index ) const
{
    Q_UNUSED(index);
    
    return Qt::ItemIsEnabled;
}

QVariant Stats::headerData( int section, Qt::Orientation orientation, int role ) const
{
     if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
     {
         if (section == 0)
            return "Name";
         else
            return "Value";
     }

     return QVariant();
 }

QModelIndex Stats::index( int row, int column, const QModelIndex& parent ) const
{
    const Record* parent_record = 0;

    if (!parent.isValid())
        parent_record = &_dummy_root;
    else
        parent_record = static_cast<const Record*>(parent.internalPointer());

    if (row >= 0 && row < parent_record->children.size())
    {
        Record* child = parent_record->children[row];
        return createIndex( row, column, child);
    }

    return QModelIndex();
}

QModelIndex Stats::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    Record* child = static_cast<Record*>(index.internalPointer());
    Record* parent_record = child->parent;

    if (parent_record == &_dummy_root)
        return QModelIndex();

    int row = 0;
    Record* grandparent = parent_record->parent;
    if (grandparent)
    {
        for (int i = 0; i < grandparent->children.size(); i++)
        {
            if (grandparent->children[i] == parent_record)
            {
                row = i;
                break;
            }
        }
    }

    return createIndex(row, 0, parent_record);
}
 
int Stats::rowCount( const QModelIndex& parent ) const
{
    int count = 0; 
    const Record* parent_rec;
    if (!parent.isValid())
        parent_rec = &_dummy_root;
    else
        parent_rec = static_cast<Record*>(parent.internalPointer());

    count = parent_rec->children.size();
    return count;
}

int Stats::columnCount( const QModelIndex& parent ) const
{
    (void)parent;
    
    return 2;
}

StatsWidget::StatsWidget(QMainWindow* parent, QMenu* window_menu) : 
    QDockWidget(tr("Statistics"), parent)
{
    QTreeView* tree_view = new QTreeView;
    tree_view->setModel(&(Stats::instance()));
    setWidget(tree_view);

    parent->addDockWidget(Qt::RightDockWidgetArea, this);
    setObjectName("stats");

    if (window_menu) {
        window_menu->addAction(this->toggleViewAction());
    }
}
