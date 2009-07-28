#include "PVDisplay.hh"

#include "pds/management/QualifiedControl.hh"

#include <QtGui/QPushButton>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QPalette>

using namespace Pds;

PVDisplay::PVDisplay(QWidget* parent,
		     QualifiedControl& control) :
  QGroupBox("Detector",parent),
  _control (control),
  _control_display( new QPushButton("-",this)),
  _green          ( new QPalette(Qt::green)),
  _red            ( new QPalette(Qt::red))
{
  //  _monitor_display->setAlignment(Qt::AlignHCenter);
  //  _control_display->setAlignment(Qt::AlignHCenter);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(new QLabel("Control State",this),Qt::AlignHCenter);
  layout->addWidget(_control_display);
  setLayout(layout);

  QObject::connect(this, SIGNAL(control_changed(bool)),
		   this, SLOT  (update_control (bool)));

  update_control(false);
}

PVDisplay::~PVDisplay()
{
  delete _green;
  delete _red;
}

void PVDisplay::runnable_change(bool runnable)
{
  printf("PVDisplay::runnable_change %c\n",runnable ? 'T' : 'F');

  _control.enable(PartitionControl::Enabled, runnable);
  emit control_changed(runnable);
}

void PVDisplay::update_control(bool runnable)
{
  _control_display->setText   (runnable ? "READY" : "NOT READY");
  _control_display->setPalette(runnable ? *_green : *_red);
}
