#include "DialsAndKnobs_ui.h"

#include <QString>
#include <QLineEdit>
#include <QFileDialog>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QLayout>
#include <QVBoxLayout>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>

void FileNameLineEdit::setFromBrowser()
{
    QString chosen = QFileDialog::getOpenFileName(this, 
            QString("Choose %1").arg(_name), "", "");
    
    if (!chosen.isNull())
    {
        setText( chosen );
        emit updateFilename(chosen);
    }
}
    
void FileNameLineEdit::callUpdateFilename()
{
    emit updateFilename(text());
}

void UpdatingTextEdit::callSendText()
{
    emit sendText(toPlainText());
}

void UpdatingTextEdit::updateText(const QString& value)
{
    if (value != toPlainText())
    {
        setPlainText(value);
    }
}

ArbitraryPrecisionSpinBox::ArbitraryPrecisionSpinBox(QWidget* parent) : 
    QDoubleSpinBox(parent)
{
    setDecimals(15);
    setKeyboardTracking(false);
}

QString ArbitraryPrecisionSpinBox::textFromValue( double value ) const
{
    QString str = QDoubleSpinBox::textFromValue(value);
    int last_non_zero = str.lastIndexOf(QRegExp("[^0]"));
    if (str[last_non_zero] == QWidget::locale().decimalPoint())
        last_non_zero--;
    return str.left(last_non_zero+1);
}


DockScrollArea::DockScrollArea(QWidget* parent) 
{
    _scroller.setWidget(&_scroller_child);
    _scroller.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _scroller.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QVBoxLayout* vbox = new QVBoxLayout;
    vbox->addWidget(&_scroller);
    vbox->setContentsMargins(0,0,0,0);
    //vbox->addStretch();
    setLayout(vbox);
};

QSize DockScrollArea::sizeHint() 
{ 
    return _scroller_child.layout()->sizeHint(); 
}

void DockScrollArea::setChildLayout(QLayout* layout) 
{ 
    _scroller_child.setLayout(layout);
    QSize size = layout->sizeHint();
    _scroller_child.setMinimumSize(size); 
    _scroller.setMinimumSize(size.width(), 10);
    
}

ValueLabel::ValueLabel(dkValue* dk_value, QWidget* parent) : QLabel(parent)
{
    _dk_value = dk_value;
    QString label = DialsAndKnobs::splitBase(dk_value->name());
    if (dk_value->isSticky())
        label += " *";
    setText(label);
    connect(_dk_value, SIGNAL(stickyChanged(bool)), this, SLOT(stickyToggled(bool)));
}
   
void ValueLabel::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction* set_sticky = new QAction("Sticky", this);
    set_sticky->setCheckable(true);
    set_sticky->setChecked(_dk_value->isSticky());
    connect(set_sticky, SIGNAL(toggled(bool)), this, SLOT(stickyToggled(bool)));
    menu.addAction(set_sticky);
    menu.exec(event->globalPos());
}

void ValueLabel::stickyToggled(bool toggled)
{
    QString label = DialsAndKnobs::splitBase(_dk_value->name());
    if (toggled)
        label += " *";
    setText(label);
    _dk_value->setSticky(toggled);
}


