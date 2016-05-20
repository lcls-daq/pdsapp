#ifndef Pds_MonTabs_hh
#define Pds_MonTabs_hh

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
    std::vector<MonCanvas*> _canvases;
  };

  class MonTabs : public QTabWidget {
  public:
    MonTabs(QWidget& parent);
    virtual ~MonTabs();

    void reset(unsigned);
    void clear();
    void setup(const MonCds&, unsigned icolor);
    void update(bool redraw);
    void update(const MonCds&);
  };
};

#endif
