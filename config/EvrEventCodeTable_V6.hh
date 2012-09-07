#ifndef Pds_EvrEventCodeTable_V6_hh
#define Pds_EvrEventCodeTable_V6_hh

#include "pdsapp/config/Parameters.hh"

#include "pds/service/LinkedList.hh"

#include <QtCore/QObject>

class QLayout;
class QGridLayout;
class QWidget;

namespace Pds { 
  namespace EvrData { 
    class ConfigV5; 
    class ConfigV6; 
    class EventCodeV5; 
    class SequencerConfigV1; 
  }
}

namespace Pds_ConfigDb {
  namespace EvrConfig_V6 {

    class EvrSeqEventDesc;
    class EvrGlbEventDesc;

    class EvrEventCodeTable : public QObject, public Parameter {
      Q_OBJECT
    public:
      enum { MaxCodes=12 };
      EvrEventCodeTable();
      ~EvrEventCodeTable();
    public:
      void pull(const Pds::EvrData::ConfigV5&);
      void pull(const Pds::EvrData::ConfigV6&);
      int  push(Pds::EvrData::EventCodeV5* to) const;
      bool validate();
      unsigned ncodes() const;
      const Pds::EvrData::EventCodeV5* codes() const;
    public:
      void insert(Pds::LinkedList<Parameter>& pList);
      QLayout* initialize(QWidget*);
      void update();
      void flush();
      void enable(bool v);
      signals:
      void update_codes(bool);
    public:
      QStringList code_names() const; // list of code names
      unsigned    code_lookup(unsigned) const; // code value from list index
      unsigned    code_index (unsigned) const; // list index from code value
              public slots:
              void update_range();
    private:
      NumericInt<unsigned> _range_lo;
      NumericInt<unsigned> _range_hi;
      QGridLayout*         _elayout;
      EvrSeqEventDesc*     _seq_code;
      EvrGlbEventDesc*     _glb_code;
      unsigned             _ncodes;
      char*                _code_buffer;
    };
  };
};

#endif
