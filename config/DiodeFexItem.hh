#ifndef Pds_DiodeFexItem_hh
#define Pds_DiodeFexItem_hh

#include "pdsapp/config/Parameters.hh"

class QWidget;
class QGridLayout;

namespace Pds_ConfigDb {

  class DiodeFexItem {
  public:
    DiodeFexItem(unsigned);
    ~DiodeFexItem();
  public:
    void set (const float* _base, const float* _scale);
    void get (float* _base, float* _scale);
    void initialize(QWidget* parent,
                    QGridLayout* layout,
                    int row, int column);
    void insert(Pds::LinkedList<Parameter>& pList);
  public:
    unsigned             nranges;
    NumericFloat<float>* base;
    NumericFloat<float>* scale;
  };

};

#endif
