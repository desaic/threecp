#pragma once
#include <QObject>
#include <QWidget>
#include <QtWidgets\QGridLayout>
#include <QtWidgets\QSlider>
#include <QtWidgets\QLabel>
#include <QtWidgets\QLineEdit>
class ParameterWidget: public QWidget
{
  Q_OBJECT
public:
  explicit ParameterWidget(QWidget *parent = nullptr);
  ~ParameterWidget();

  void setValidator(QValidator * val) { edit->setValidator(val); }

  void setDisplayedValue(int val) {
    edit->setText(QString::number(val));
    slider->setSliderPosition(val);
  }

  void setRange(int minimum, int maximum);

  void setTracking(bool b) { slider->setTracking(b); }

  void setText(QString t) { label->setText(t); }

signals:
  void valueChanged(int val);

public slots:
  void changeVal(int val);
  void updateEdit();
private:
  QGridLayout * gridLayout;
  QSlider * slider;
  QLabel * label;
  QLineEdit * edit;

  int minVal, maxVal;
};