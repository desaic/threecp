#include "parameterwidget.h"
#include <algorithm>
const int DEFAULT_SLIDER_MAX=100;

ParameterWidget::ParameterWidget(QWidget * parent)
{
  QSizePolicy sizePolicy3(QSizePolicy::Minimum, QSizePolicy::Fixed);
  sizePolicy3.setHorizontalStretch(0);
  sizePolicy3.setVerticalStretch(0);
  sizePolicy3.setHeightForWidth(sizePolicy().hasHeightForWidth());
  setSizePolicy(sizePolicy3);
  setMinimumSize(QSize(0, 50));
  
  gridLayout = new QGridLayout(this);
  gridLayout->setSpacing(3);
  gridLayout->setContentsMargins(5, 5, 5, 5);
  slider = new QSlider(this);
  QSizePolicy sizePolicy4(QSizePolicy::Expanding, QSizePolicy::Fixed);
  sizePolicy4.setHorizontalStretch(0);
  sizePolicy4.setVerticalStretch(0);
  sizePolicy4.setHeightForWidth(slider->sizePolicy().hasHeightForWidth());
  slider->setSizePolicy(sizePolicy4);
  slider->setMaximum(DEFAULT_SLIDER_MAX);
  slider->setOrientation(Qt::Horizontal);

  gridLayout->addWidget(slider, 0, 1, 1, 1);

  label = new QLabel(this);
  QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);

  sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
  label->setSizePolicy(sizePolicy);
  label->setMinimumSize(QSize(30, 0));
  label->setText("param");
  gridLayout->addWidget(label, 0, 0, 1, 1);

  edit = new QLineEdit(this);
  sizePolicy3.setHeightForWidth(edit->sizePolicy().hasHeightForWidth());
  edit->setSizePolicy(sizePolicy3);
  edit->setText("0");

  gridLayout->addWidget(edit, 0, 2, 1, 1);
  slider->setTracking(false);

  connect(slider, &QSlider::valueChanged, this, &ParameterWidget::changeVal);
  connect(edit, &QLineEdit::editingFinished, this, &ParameterWidget::updateEdit);
}

void ParameterWidget::changeVal(int val) {
  setDisplayedValue(val);
  emit valueChanged(val);
}

void ParameterWidget::updateEdit() {
  int editVal = edit->text().toInt();
  editVal = std::min(editVal, maxVal);
  editVal = std::max(editVal, minVal);
  setDisplayedValue(editVal);
  emit valueChanged(editVal);
}

void ParameterWidget::setRange(int minimum, int maximum) {
  minVal = minimum;
  maxVal = maximum;
  slider->setMinimum(minVal);
  slider->setMaximum(maxVal);
}

ParameterWidget::~ParameterWidget()
{
}
