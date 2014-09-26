#include "pdsapp/config/EvsConfig.hh"
#include "pdsapp/config/EvsPulseTable.hh"
#include "pdsapp/config/GlobalCfg.hh"
#include "pds/config/EvsConfigType.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QTabWidget>
#include <QtGui/QMessageBox>
#include <QtGui/QLabel>

namespace Pds_ConfigDb {

  class EvsCode : public Parameter {
  public:
    EvsCode() : 
      Parameter(NULL),
      _code  ("Code",59,59,59),
      _rate  ("Rate [Hz]",120,0,119e6)
    {}
  public:
    QLayout* initialize(QWidget*) {
      QHBoxLayout* layout = new QHBoxLayout;
      layout->addStretch();
      layout->addLayout(_code  .initialize(0));
      layout->addLayout(_rate  .initialize(0));
      layout->addStretch();
      return layout;
    }
    void     update    () { _code.update(); _rate.update(); }
    void     flush     () { _code.flush (); _rate.flush (); }
    void     enable    (bool) {}
  public:
    unsigned ncodes() const { return 1; }
    const Pds::EvrData::SrcEventCode* codes() const { 
      return _result;
    };
  public:
    void pull(const EvsConfigType& tc) {
      _code  .value = tc.eventcodes()[0].code();
      unsigned period = tc.eventcodes()[0].period();
      _rate  .value = period ? 119.e6/double(period) : 0;
    }
    bool validate() {
      new(_result) Pds::EvrData::SrcEventCode(_code.value, 
					      _rate.value ? unsigned(119e6/_rate.value+0.5) : 0,
					      0, 0, 
					      0, 1);
      return true;
    }
  private:
    NumericInt<unsigned> _code;
    NumericFloat<double> _rate;
    Pds::EvrData::SrcEventCode _result[2];
  };

  class EvsConfig::Private_Data : public Parameter {
  public:
    Private_Data() :
      Parameter(NULL),
      _pulse_table (new EvsPulseTables),
      _code_table  (new EvsCode)
    {}
  public:
    QLayout* initialize(QWidget*) {
      QVBoxLayout* layout = new QVBoxLayout;
      layout->addLayout(_pulse_table->initialize(0));
      layout->addLayout(_code_table ->initialize(0));
      return layout;
    }
    void     update    () { _pulse_table->update(); _code_table ->update(); }
    void     flush     () { _pulse_table->flush (); _code_table ->flush (); }
    void     enable    (bool) {}
  public:
    int pull(void *from) {
      const EvsConfigType& tc = *reinterpret_cast<const EvsConfigType*>(from);
      _pulse_table->pull(tc);
      _code_table ->pull(tc);
      return Pds::EvsConfig::size(tc);
    }
    int push(void *to) {      
      const_cast<EvsConfig::Private_Data*>(this)->validate();      
      EvsConfigType& tc = *new(to) EvsConfigType(_code_table->ncodes   (),
						 _pulse_table->npulses (),
                                                 _pulse_table->noutputs(),
                                                 _code_table->codes    (),
                                                 _pulse_table->pulses (),
                                                 _pulse_table->outputs());
      return Pds::EvsConfig::size(tc);
    }

    int dataSize() const {
      const_cast<EvsConfig::Private_Data*>(this)->validate();
      
      return sizeof(EvsConfigType) + 
        _code_table ->ncodes  ()*sizeof(EvsCodeType) +
        _pulse_table->npulses ()*sizeof(PulseType) +
        _pulse_table->noutputs()*sizeof(OutputMapType);
    }
    bool validate() {
      bool result = _code_table   ->validate();
      result &= _pulse_table  ->validate(_code_table->ncodes(),
					 _code_table->codes());
      return result;
    }
  private:
    EvsPulseTables*   _pulse_table;
    EvsCode*          _code_table;
  };

};


using namespace Pds_ConfigDb;

EvsConfig::EvsConfig():
  Serializer("Evr_Config"), _private_data(new Private_Data)
{
  pList.insert(_private_data);
}

EvsConfig::~EvsConfig()
{
  delete _private_data;
}

int EvsConfig::readParameters(void *from)
{
  return _private_data->pull(from);
}

int EvsConfig::writeParameters(void *to)
{
  return _private_data->push(to);
}

int EvsConfig::dataSize() const
{
  return _private_data->dataSize();
}

bool EvsConfig::validate() 
{
  return _private_data->validate();
}

