#include "pdsapp/config/EvrConfigP.hh"
#include "pdsapp/config/EvrEventCodeTable.hh"
#include "pdsapp/config/EvrPulseTable.hh"
#include "pdsapp/config/SequencerConfig.hh"
#include "pds/config/EventcodeTiming.hh"
#include "pdsapp/config/GlobalCfg.hh"
#include "pds/config/EvrIOConfigType.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QTabWidget>
#include <QtGui/QMessageBox>
#include <QtGui/QLabel>

#define ADDTAB(p,title) {                       \
    QWidget* w = new QWidget;                   \
    w->setLayout(p->initialize(0));             \
    tab->addTab(w,title); }

namespace Pds_ConfigDb {

  class EvrConfigP::Private_Data : public Parameter {
  public:
    Private_Data() :
      Parameter(NULL),
      _pulse_table (new EvrPulseTables) ,
      _code_table  (new EvrEventCodeTable(_pulse_table)),
      _seq_config  (new SequencerConfig(*_code_table))
    {}
    ~Private_Data()
    { delete _pulse_table;
      delete _code_table;
      delete _seq_config; }
  public:
    QLayout* initialize(QWidget*) {
      QHBoxLayout* layout = new QHBoxLayout;
      QTabWidget* tab = new QTabWidget;
      ADDTAB(_pulse_table,"Pulses");
      ADDTAB(_code_table ,"EventCodes");
      ADDTAB(_seq_config ,"Sequencer");
      tab->setTabEnabled(2,false);
      layout->addWidget(tab);
      return layout;
    }
    void     update    () { _pulse_table->update(); _code_table ->update(); _seq_config->update(); }
    void     flush     () { _pulse_table->flush (); _code_table ->flush (); _seq_config->flush (); }
    void     enable    (bool) {}
  public:
    int pull(void *from) {
      //printf("Pds_ConfigDb::EvrConfigP::Private_Data::pull(): begin\n"); //!!!debug
      const EvrConfigType& tc = *reinterpret_cast<const EvrConfigType*>(from);
      _pulse_table->pull(tc);
      _code_table ->pull(tc);
      _seq_config ->pull(tc);
      //printf("Pds_ConfigDb::EvrConfigP::Private_Data::pull(): end enableGroup %d\n", (int)_code_table->enableReadoutGroup()); //!!!debug
      
      //      tc.print(); //!!!debug
      return Pds::EvrConfig::size(tc);
    }
    int push(void *to) {      
      //printf("Pds_ConfigDb::EvrConfigP::Private_Data::push(): begin:  pulse %d enableGroup %d\n", _pulse_table->npulses (),
      //  (int)_code_table->enableReadoutGroup()); //!!!debug
      const_cast<EvrConfigP::Private_Data*>(this)->validate();      
      EvrConfigType& tc = *new(to) EvrConfigType(_code_table ->ncodes  (),
                                                 _pulse_table->npulses (),
                                                 _pulse_table->noutputs(),
                                                 _code_table->codes   (),
                                                 _pulse_table->pulses (),
                                                 _pulse_table->outputs(),
                                                _seq_config ->result() );
      //printf("Pds_ConfigDb::EvrConfigP::Private_Data::push(): end\n"); //!!!debug
      //tc.print(); //!!!debug
      return Pds::EvrConfig::size(tc);
    }

    int dataSize() const {
      //printf("Pds_ConfigDb::EvrConfigP::Private_Data::dataSize(): begin:  pulse %d\n", _pulse_table->npulses ()); //!!!debug
      const_cast<EvrConfigP::Private_Data*>(this)->validate();
      //printf("Pds_ConfigDb::EvrConfigP::Private_Data::dataSize(): after validate:  pulse %d\n", _pulse_table->npulses ()); //!!!debug
      
      return sizeof(EvrConfigType) + 
        _code_table ->ncodes  ()*sizeof(EventCodeType) +
        _pulse_table->npulses ()*sizeof(PulseType) +
        _pulse_table->noutputs()*sizeof(OutputMapType) +
        _seq_config ->result()._sizeof();
    }
    bool validate() {
      //printf("Pds_ConfigDb::EvrConfigP::Private_Data::validate(): begin:  pulse %d\n", _pulse_table->npulses ()); //!!!debug
      bool v1 = _code_table   ->validate();
      bool v2 = _pulse_table  ->validate(_code_table->ncodes(),
                                        _code_table->codes () );
      bool v3 = _seq_config   ->validate();
      //printf("Pds_ConfigDb::EvrConfigP::Private_Data::validate(): end:  pulse %d\n", _pulse_table->npulses ()); //!!!debug      
      return v1 && v2 && v3;
    }
  private:
    EvrPulseTables*    _pulse_table;
    EvrEventCodeTable* _code_table;
    SequencerConfig*   _seq_config;
  };

};


using namespace Pds_ConfigDb;

EvrConfigP::EvrConfigP():
  Serializer("Evr_Config"), _private_data(new Private_Data)
{
  pList.insert(_private_data);
}

EvrConfigP::~EvrConfigP()
{
  delete _private_data;
}

int EvrConfigP::readParameters(void *from)
{
  return _private_data->pull(from);
}

int EvrConfigP::writeParameters(void *to)
{
  return _private_data->push(to);
}

int EvrConfigP::dataSize() const
{
  return _private_data->dataSize();
}

bool EvrConfigP::validate() 
{
  return _private_data->validate();
}
