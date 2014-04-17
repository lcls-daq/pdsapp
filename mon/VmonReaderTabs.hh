#ifndef Pds_VmonReaderTabs_hh
#define Pds_VmonReaderTabs_hh

#include <QtGui/QTabWidget>

#include "pdsdata/xtc/Level.hh"

#include <vector>

namespace Pds {

  class MonCds;
  class MonCanvas;
  class MonGroup;

  class MyTab : public QWidget {
  public:
    MyTab( const MonGroup&, unsigned icolor);
    void insert(const MonGroup&, unsigned icolor);
    void update(bool redraw);
    void reset (unsigned);
  private:
    std::vector< std::vector<MonCanvas*> > _canvases;
  };

  class VmonReaderTabs : public QTabWidget {
  public:
    VmonReaderTabs(QWidget& parent);
    virtual ~VmonReaderTabs();

    void reset(unsigned);
    void clear();
    void setup(const MonCds&, unsigned icolor);
    void update(bool redraw);
  };
};

#endif
