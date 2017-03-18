#include "pdsapp/config/QuadAdcChannelMask.hh"

using namespace Pds_ConfigDb;

#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QComboBox>
#include <QtGui/QButtonGroup>
#include <QtGui/QStackedWidget>
#include <stdio.h>

QuadAdcChannelMask::QuadAdcChannelMask(const char* label, unsigned val) :
  NumericInt<unsigned>  (label, val, 1, (1<<(Channels*Modules))-1,Hex)
{
}

QuadAdcChannelMask::~QuadAdcChannelMask() 
{
}

void QuadAdcChannelMask::modeChanged(int mode)
{
  printf("MODE %i\n", mode);
  _group->setExclusive(mode);

  if(mode == 1){
    for(int i=0; i<4; i++){
      if(_box[i]->isChecked())
	_box[i]->setChecked(0);
    }
  }
  

}

int QuadAdcChannelMask::interleave() const {
  return _combobox->currentIndex();
}

void QuadAdcChannelMask::setinterleave(int interleave){

  _combobox->setCurrentIndex(interleave);
  //emit _combobox->currentIndexChanged(interleave);
  modeChanged(interleave);
}



int QuadAdcChannelMask::samplerate() const {
  return _rateBox->currentIndex();
}


void QuadAdcChannelMask::setsamplerate(int samplerate){
  _rateBox->setCurrentIndex(samplerate);
}






QLayout* QuadAdcChannelMask::initialize(QWidget* parent)
{
  QVBoxLayout* l    = new QVBoxLayout;
  QGridLayout* grid = new QGridLayout;
  _group = new QButtonGroup;


  QStringList mode_names;
  mode_names << "4 Channel Mode" << "1 Channel Mode";

  QComboBox* combobox = new QComboBox;
  combobox->addItems(mode_names);
  _combobox = combobox;

  grid->addWidget(combobox);




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
      _group->addButton(box);
      _box[k++] = box;
    }
    
  }

  //Sample Rate Stacked Widget//
  QStringList rate_names;
  rate_names << "1.25 GHz" << "625 MHz" << "250 MHz" << "125 MHz" << "62.5 MHz" << "41.7 MHz" << "31.3 MHz" << "25 MHz" << "20.8 MHz" << "17.9 MHz" << "15.6 MHz" ;

  QStackedWidget* sw = new QStackedWidget;
  _rateBox = new QComboBox;
  _rateBox->addItems(rate_names);

 
  sw->addWidget(_rateBox);
  sw->addWidget(new QLabel("5 MHz"));

  QObject::connect(combobox,SIGNAL(currentIndexChanged(int)), sw, SLOT(setCurrentIndex(int)));
  grid->addWidget(sw, 2, 1,1,4, ::Qt::AlignHCenter);
  grid->addWidget(new QLabel(QString("Sample Rate")), 2, 0,1,1, ::Qt::AlignHCenter);



  l->addLayout(grid);
  l->addLayout(NumericInt<unsigned>::initialize(parent));


  if (allowEdit()) {
    QObject::connect(_input, SIGNAL(editingFinished()),
                     this, SLOT(numberChanged()));

    QObject::connect(combobox, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(modeChanged(int)));

  }
  return l;
  
}

void QuadAdcChannelMask::flush()
{
  NumericInt<unsigned>::flush();
  numberChanged();
}

void QuadAdcChannelMask::boxChanged(int) 
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

void QuadAdcChannelMask::numberChanged() {
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
