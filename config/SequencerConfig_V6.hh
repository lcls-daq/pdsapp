#ifndef Pds_SequencerConfig_V6_hh
#define Pds_SequencerConfig_V6_hh

#include "pdsapp/config/Serializer.hh"
#include "pdsapp/config/Parameters.hh"

#include <QtGui/QWidget>

#include "pdsdata/psddl/evr.ddl.h"
typedef Pds::EvrData::SequencerEntry SeqEntryType;

class QGridLayout;
class QButtonGroup;
class QStackedWidget;

namespace Pds {
  namespace EvrData {
    class ConfigV5;
    class ConfigV6;
    class SequencerConfigV1;
  }
}

namespace Pds_ConfigDb {
  namespace EvrConfig_V6 {
    class DetailConfig;
    class EvrEventCodeTable;

    class SequencerConfig : public Parameter {
      enum { Detail, External };
    public:
      SequencerConfig(const EvrEventCodeTable&);
      ~SequencerConfig();
    public:
      QLayout* initialize(QWidget*);
      void     update    ();
      void     flush     ();
      void     enable    (bool);
    public:
      void pull  (const Pds::EvrData::ConfigV6&);
      void pull  (const Pds::EvrData::ConfigV5&);
      bool validate();
      const Pds::EvrData::SequencerConfigV1& result() const;
    private:
      const EvrEventCodeTable& _code_table;
      QButtonGroup*      _mode;
      DetailConfig*      _detail;
      QStackedWidget*    _stack;
      char*              _config_buffer;
    };

    class SeqEntryInput : public QWidget {
      Q_OBJECT
      enum { InitSize=32 };
    public:
      SeqEntryInput(const EvrEventCodeTable&);
    public:
      void pull(const Pds::EvrData::SequencerConfigV1&);
      void update();
      void flush ();
              public slots:
              void update_codes (int);
      void update_codes (bool);
      void update_totals();
      void update_length();
    public:
      SeqEntryType* entries() const;
      unsigned     nentries() const;
    private:
      const EvrEventCodeTable&          _code_table;
      unsigned                          _ulength;
      NumericInt<unsigned>              _length;
      QGridLayout*                      _elayout;
    };
  };
};

#endif
