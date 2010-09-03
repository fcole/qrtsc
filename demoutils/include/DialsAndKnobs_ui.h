/*****************************************************************************\

DialsAndKnobs_ui.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2010 Forrester Cole

An easy and simple way to expose variables in a Qt UI.

demoutils is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

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
