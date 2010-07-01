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
    void setFromBrowser();
    void callUpdateFilename();

  private:
    QString _name;
};

class UpdatingTextEdit : public QTextEdit
{
    Q_OBJECT

  signals:
    void sendText(const QString& name);
    
  public slots:
    void callSendText();
    void updateText(const QString& value);
};

class ArbitraryPrecisionSpinBox : public QDoubleSpinBox
{
  public:
    ArbitraryPrecisionSpinBox(QWidget* parent = 0);

    virtual QString textFromValue( double value ) const;
};


#endif // _DIALS_AND_KNOBS_UI_H_
