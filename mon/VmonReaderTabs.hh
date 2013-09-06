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
    MyTab( const MonGroup& );
    void update(bool redraw);
    void reset ();
  private:
    const MonGroup& _group;
    std::vector<MonCanvas*> _canvases;
  };

  class VmonReaderTabs : public QTabWidget {
  public:
    VmonReaderTabs(QWidget& parent);
    virtual ~VmonReaderTabs();

    void reset();
    void reset(const MonCds&);
    void update(bool redraw);
  };
};

#endif
