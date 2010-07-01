#include "DialsAndKnobs_ui.h"

#include <QString>
#include <QLineEdit>
#include <QFileDialog>
#include <QTextEdit>
#include <QDoubleSpinBox>

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
