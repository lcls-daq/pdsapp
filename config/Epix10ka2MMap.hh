#ifndef PdsConfigDb_Epix10ka2MMap_hh
#define PdsConfigDb_Epix10ka2MMap_hh

#include <QtGui/QWidget>
#include <stdint.h>
#include <vector>

namespace Pds_ConfigDb
{
  class EpixSector;

  class Epix10ka2MMap : public QWidget {
    Q_OBJECT
  public:
    Epix10ka2MMap(unsigned nq=4, unsigned grouping=1, bool exclusive=false);
    ~Epix10ka2MMap();
  public:
    void     update(uint64_t m);
    uint64_t value() const;
    void setQ(unsigned);
  public slots:
    void     setQ0();
    void     setQ1();
    void     setQ2();
    void     setQ3();
  signals:
    void changed();  
  private:
    std::vector<EpixSector*> _sectors;
  };
};

#endif
