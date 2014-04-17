#ifndef Pds_MonCONSUMERFACTORY_HH
#define Pds_MonCONSUMERFACTORY_HH

class QWidget;

namespace Pds {

  class MonDesc;
  class MonCanvas;
  class MonEntry;
  class MonGroup;

  class MonConsumerFactory {
  public:
    static MonCanvas* create(QWidget& parent,
			     const MonDesc& clientdesc,
			     const MonGroup& groupdesc,
			     const MonEntry& entry);
  };
};

#endif
