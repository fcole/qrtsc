#ifndef _DIALS_AND_KNOBS_UI_H
#define _DIALS_AND_KNOBS_UI_H

#include <QString>
#include <QLineEdit>
#include <QFileDialog>
#include <QTextEdit>
#include <QDoubleSpinBox>

class FileNameLineEdit : public QLineEdit
{
    Q_OBJECT
  public:
    FileNameLineEdit(const QString& name) : _name(name) {}
  signals:
    void updateFilename(const QString& name);
    
    public slots:
    void setFromBrowser()
    {
        QString chosen = QFileDialog::getOpenFileName(this, 
                QString("Choose %1").arg(_name), "", "");
        
        if (!chosen.isNull())
        {
            setText( chosen );
            emit updateFilename(chosen);
        }
    }
    
    void callUpdateFilename()
    {
        emit updateFilename(text());
    }
  private:
    QString _name;
};

class UpdatingTextEdit : public QTextEdit
{
    Q_OBJECT

  signals:
    void sendText(const QString& name);
    
  public slots:
    void callSendText()
    {
        emit sendText(toPlainText());
    }
    void updateText(const QString& value)
    {
        if (value != toPlainText())
        {
            setPlainText(value);
        }
    }
};

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


#endif // _DIALS_AND_KNOBS_UI_H_
