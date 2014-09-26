#ifndef PdsConfigDb_EvrIOChannel_hh
#define PdsConfigDb_EvrIOChannel_hh

#include <QtCore/QObject>

#include "pdsapp/config/Parameters.hh"
#include "pdsdata/xtc/DetInfo.hh"

class QLabel;
class QComboBox;
class QGridLayout;
class QPushButton;

namespace Pds {
  namespace EvrData {
    class IOChannel;
  };
};

namespace Pds_ConfigDb {
  namespace EvrIOConfig_V1 {
    class EvrIOChannel : public QObject {
      Q_OBJECT
    public:
      EvrIOChannel(unsigned i);
    public:
      void layout(QGridLayout*, unsigned);
    public:
      void insert(Pds::LinkedList<Parameter>&);
      void pull  (const Pds::EvrData::IOChannel&);
      void push  (Pds::EvrData::IOChannel&) const;
    public:
      static void initialize();
	      public slots:
	      void add_info();
      void remove_info();
    private:
      enum { MaxInfos = 16 };
      unsigned                           _id;
      QLabel*                            _channel;
      TextParameter                      _label;
      unsigned                           _ninfo;
      Pds::DetInfo                       _detinfo[MaxInfos];
      QComboBox*                         _detnames;
      Enumerated<Pds::DetInfo::Detector> _dettype;
      NumericInt<unsigned>               _detid;
      Enumerated<Pds::DetInfo::Device>   _devtype;
      NumericInt<unsigned>               _devid;
      QPushButton*                       _add;
      QPushButton*                       _remove;
    };
  };
};

#endif
