#ifndef Pds_DiodeFexItem_hh
#define Pds_DiodeFexItem_hh

#include "pdsapp/config/Parameters.hh"

class QWidget;
class QGridLayout;

namespace Pds { namespace Lusi { class DiodeFexConfigV1; } }

namespace Pds_ConfigDb {

  class DiodeFexItem {
  public:
    DiodeFexItem();
    ~DiodeFexItem();
  public:
    void set (const Pds::Lusi::DiodeFexConfigV1& c);
    void initialize(QWidget* parent,
                    QGridLayout* layout,
                    int row, int column);
    void insert(Pds::LinkedList<Parameter>& pList);
  public:
    NumericFloat<float> base0;
    NumericFloat<float> base1;
    NumericFloat<float> base2;
    NumericFloat<float> scale0;
    NumericFloat<float> scale1;
    NumericFloat<float> scale2;
  };

};

#endif
