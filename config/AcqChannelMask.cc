#include "pdsapp/config/AcqChannelMask.hh"

using namespace Pds_ConfigDb;

#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

AcqChannelMask::AcqChannelMask(const char* label, unsigned val) :
  NumericInt<unsigned>  (label, val, 1, (1<<(Channels*Modules))-1,Hex)
{
}

AcqChannelMask::~AcqChannelMask() 
{
}

QLayout* AcqChannelMask::initialize(QWidget* parent)
{
  QVBoxLayout* l    = new QVBoxLayout;
  QGridLayout* grid = new QGridLayout;

  for(unsigned col=1; col<=Channels; col++)
    grid->addWidget(new QLabel(QString("Ch%1").arg(col)),0,col,::Qt::AlignHCenter);

  unsigned k=0;
  for(unsigned row=1; row<=Modules; row++) {
    grid->addWidget(new QLabel(QString("Module%1").arg(row)),row,0,::Qt::AlignHCenter);
    for(unsigned col=1; col<=Channels; col++) {
      QCheckBox* box = new QCheckBox;
      grid->addWidget(box = new QCheckBox,row,col,::Qt::AlignHCenter);
      if (allowEdit())
        QObject::connect(box, SIGNAL(stateChanged(int)), this, SLOT(boxChanged(int)));
      else {
        box->setCheckState((value>>k)&1 ? ::Qt::Checked : ::Qt::Unchecked);
	box->setEnabled(false);
      }
      _box[k++] = box;
    }
  }

  l->addLayout(grid);
  l->addLayout(NumericInt<unsigned>::initialize(parent));

  if (allowEdit())
    QObject::connect(_input, SIGNAL(editingFinished()),
                     this, SLOT(numberChanged()));

  return l;
}

void AcqChannelMask::flush()
{
  NumericInt<unsigned>::flush();
  numberChanged();
}

void AcqChannelMask::boxChanged(int) 
{
  if (!allowEdit()) return;

  unsigned k=0;
  unsigned mask=0;
  for(unsigned row=1; row<=Modules; row++) {
    for(unsigned col=1; col<=Channels; col++,k++) {
      QCheckBox* box = _box[k];
      if (box->checkState()==::Qt::Checked)
        mask |= (1<<k);
    }
  }

  QObject::disconnect(_input, SIGNAL(editingFinished()),
             this, SLOT(numberChanged()));
  value = mask;
  flush();
  _input->setFocus(::Qt::OtherFocusReason);
  QObject::connect(_input, SIGNAL(editingFinished()),
          this, SLOT(numberChanged()));
}

void AcqChannelMask::numberChanged() {
  unsigned k=0;
  unsigned mask= (allowEdit() ? _input->text() : _display->text()).toUInt(0,16);
  for(unsigned row=1; row<=Modules; row++) {
    for(unsigned col=1; col<=Channels; col++,k++) {
      QCheckBox* box = _box[k];
      QObject::disconnect(box, SIGNAL(stateChanged(int)), this, SLOT(boxChanged(int)));
      box->setChecked( (mask>>k)&1 ? ::Qt::Checked : ::Qt::Unchecked);
      QObject::connect(box, SIGNAL(stateChanged(int)), this, SLOT(boxChanged(int)));
    }
  }
}
