#include "AcqTdcConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/PolarityButton.hh"
#include "pds/config/AcqConfigType.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

#include <new>

using Pds::Acqiris::TdcChannel;
using Pds::Acqiris::TdcAuxIO;
using Pds::Acqiris::TdcVetoIO;

namespace Pds_ConfigDb {

#define TT TdcChannel
  static const TT::Channel chvals[] = { TT::Veto, TT::Common, 
					TT::Input1, TT::Input2,
					TT::Input3, TT::Input4,
					TT::Input5, TT::Input6};
  static const char* chvalnames[] = { "Veto","Common",
				      "Input1","Input2",
				      "Input3","Input4",
				      "Input5","Input6",
				      NULL };
#undef TT
  static const char* mode_names[] = {"BankSwitch","Marker","OutputLo","OutputHi",NULL};
  static const TdcAuxIO::Mode mode_values[]={TdcAuxIO::BankSwitch,
					     TdcAuxIO::Marker,
					     TdcAuxIO::OutputLo,
					     TdcAuxIO::OutputHi};
  static const char* term_names[] = {"HighZ","50 Ohms",NULL};
  static const char* vmode_names[] = {"Veto","SwitchVeto","InvVeto","InvSwitchVeto",NULL};
  static const TdcVetoIO::Mode vmode_values[]={TdcVetoIO::Veto,
					       TdcVetoIO::SwitchVeto,
					       TdcVetoIO::InvertedVeto,
					       TdcVetoIO::InvertedSwitchVeto};

  class AcqTdcConfig::Private_Data : public Parameter {
  public:
    Private_Data() :
      Parameter(NULL) 
    {
      for(unsigned i=0; i<NChan; i++)
	_level[i] = new NumericFloat<double>(NULL,1.0,-2.5,2.5);
      for(unsigned i=0; i<NAuxIO; i++) {
	_auxio_mode[i] = new Enumerated<TdcAuxIO::Mode>
	  ("Mode",TdcAuxIO::Marker,mode_names,mode_values);
	_auxio_term[i] = new Enumerated<TdcAuxIO::Termination>
	  ("Term",TdcAuxIO::ZHigh,term_names);
      }
      _veto_mode = new Enumerated<TdcVetoIO::Mode>
	("Mode",TdcVetoIO::Veto,vmode_names,vmode_values);
      _veto_term = new Enumerated<TdcVetoIO::Termination>
	("Term",TdcVetoIO::ZHigh,term_names);
    }
  public:
    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(this);

      for(unsigned i=0; i<NChan; i++)
	_pList.insert(_level[i]);
      for(unsigned i=0; i<NAuxIO; i++) {
	_pList.insert(_auxio_mode[i]);
	_pList.insert(_auxio_term[i]);
      }
      _pList.insert(_veto_mode);
      _pList.insert(_veto_term);
    }
    int pull(void* from) { // pull "from xtc"
      AcqTdcConfigType& c = *new(from) AcqTdcConfigType;
      for(unsigned i=0; i<NChan; i++) {
	PolarityButton::State s = PolarityButton::None;
	if (c.channel(i).mode()==TdcChannel::Active)
	  s = (c.channel(i).slope()==TdcChannel::Positive) ?
	    PolarityButton::Pos : PolarityButton::Neg;
	_slope[i]->setState(s);
	_level[i]->value = c.channel(i).level();
      }
      for(unsigned i=0; i<NAuxIO; i++) {
	_auxio_mode[i]->value = c.auxio(i).mode();
	_auxio_term[i]->value = c.auxio(i).term();
      }
      _veto_mode->value = c.veto().mode();
      _veto_term->value = c.veto().term();
      return sizeof(c);
    }
    int  push(void* to) {
      TdcChannel channels[AcqTdcConfigType::NChannels];
      TdcAuxIO   auxio   [AcqTdcConfigType::NAuxIO];
      TdcVetoIO  veto;

      for(unsigned i=0; i<NChan; i++) {
	TdcChannel::Mode m = 
	  (_slope[i]->state()==PolarityButton::None) ?
	  TdcChannel::Inactive :
	  TdcChannel::Active;
	TdcChannel::Slope s = 
	  (_slope[i]->state()==PolarityButton::Pos) ?
	  TdcChannel::Positive :
	  TdcChannel::Negative;
	new(&channels[i]) TdcChannel(chvals[i],m,s,_level[i]->value);
      }
      new(&auxio[0]) TdcAuxIO(TdcAuxIO::IOAux1,
			      _auxio_mode[0]->value,
			      _auxio_term[0]->value);
      new(&auxio[1]) TdcAuxIO(TdcAuxIO::IOAux2,
			      _auxio_mode[1]->value,
			      _auxio_term[1]->value);
      new(&veto) TdcVetoIO(_veto_mode->value,
			   _veto_term->value);
      AcqTdcConfigType* c = new(to) AcqTdcConfigType(channels, auxio, veto);
      return sizeof(*c);
    }
  public:
    void update() { 
      Parameter* p = _pList.forward();
      while( p != _pList.empty() ) {
	p->update();
	p = p->forward();
      }
    }
    void flush() { 
      Parameter* p = _pList.forward();
      while( p != _pList.empty() ) {
	p->flush();
	p = p->forward();
      }
    }
    void enable(bool) {}
    QLayout* initialize(QWidget*) {
      QVBoxLayout* layout = new QVBoxLayout;

      { QHBoxLayout* h = new QHBoxLayout;
	h->addStretch();
	QGridLayout* l = new QGridLayout;
	l->addWidget(new QLabel("Channel") , 0, 0);
	l->addWidget(new QLabel("Slope")   , 0, 1);
	l->addWidget(new QLabel("Level[V]"), 0, 2);
	for(unsigned i=0; i<NChan; i++) {
	  l->addWidget(new QLabel(chvalnames[i])   , i+1,0);
	  l->addWidget(_slope[i]=new PolarityButton, i+1,1);
	  l->addLayout(_level[i]->initialize(0)    , i+1,2);
	}
	h->addLayout(l);
	h->addStretch();
	layout->addLayout(h); }

      layout->addStretch();

      { QHBoxLayout* h = new QHBoxLayout;
	h->addStretch();
	QGridLayout* l = new QGridLayout;
	for(unsigned i=0; i<NAuxIO; i++) {
	  l->addWidget(new QLabel(QString("AuxIO%1").arg(i+1)), i, 0);
	  l->addLayout(_auxio_mode[i]->initialize(0), i, 1);
	  l->addLayout(_auxio_term[i]->initialize(0), i, 2);
	}
	l->addWidget(new QLabel("VetoIO")     , 2, 0);
	l->addLayout(_veto_mode->initialize(0), 2, 1);
	l->addLayout(_veto_term->initialize(0), 2, 2); 
	h->addLayout(l);
	h->addStretch();
	layout->addLayout(h); }
      return layout;
    }
    int dataSize() const { return sizeof(AcqTdcConfigType); }
  private:
    enum { NChan = AcqTdcConfigType::NChannels };
    enum { NAuxIO = 2 };
    PolarityButton*       _slope[NChan];
    NumericFloat<double>* _level[NChan];
    Enumerated<TdcAuxIO::Mode>*         _auxio_mode[NAuxIO];
    Enumerated<TdcAuxIO::Termination>*  _auxio_term[NAuxIO];
    Enumerated<TdcVetoIO::Mode>*        _veto_mode;
    Enumerated<TdcVetoIO::Termination>* _veto_term;
    Pds::LinkedList<Parameter> _pList;
  };
};

using namespace Pds_ConfigDb;


AcqTdcConfig::AcqTdcConfig() : 
  Serializer("AcqTdc_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int AcqTdcConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  AcqTdcConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  AcqTdcConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

template class Enumerated<TdcAuxIO::Mode>;
template class Enumerated<TdcAuxIO::Termination>;
template class Enumerated<TdcVetoIO::Mode>;
template class Enumerated<TdcVetoIO::Termination>;
