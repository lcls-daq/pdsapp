#ifndef Pds_MonCONSUMERFACTORY_HH
#define Pds_MonCONSUMERFACTORY_HH

class QWidget;

namespace Pds {

  class MonDesc;
  class MonCanvas;
  class MonEntry;

  class MonConsumerFactory {
  public:
    static MonCanvas* create(QWidget& parent,
			     const MonDesc& clientdesc,
			     const MonDesc& groupdesc,
			     const MonEntry& entry);
  };
};

#endif
