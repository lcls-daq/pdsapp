#ifndef PdsConfigDb_CspadGainMap_hh
#define PdsConfigDb_CspadGainMap_hh

#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QWidget;
class QVBoxLayout;

namespace Pds { namespace CsPad { class CsPadGainMapCfg; } }

namespace Pds_ConfigDb
{
  class QuadGainMap;
  class SectionDisplay;

  class CspadGainMap : public QObject {
    Q_OBJECT
  public:
    CspadGainMap();
    ~CspadGainMap();
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    void initialize(QWidget* parent, QVBoxLayout* layout);
    void flush();
    void show_map (unsigned,unsigned);
  public:
    Pds::CsPad::CsPadGainMapCfg* quad(int q);
  public slots:
    void import_();
    void export_();
    void set_section();
    void clear_section();
  private:
    QuadGainMap* _quad[4];
    SectionDisplay* _display;
    unsigned _q, _s;
  };
}

#endif
