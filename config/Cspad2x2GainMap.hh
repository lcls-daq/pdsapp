#ifndef PdsConfigDb_Cspad2x2GainMap_hh
#define PdsConfigDb_Cspad2x2GainMap_hh

#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QWidget;
class QVBoxLayout;

namespace Pds { namespace CsPad2x2 { class CsPad2x2GainMapCfg; } }

namespace Pds_ConfigDb
{
  class Quad2x2GainMap;
  class SectionDisplay2x2;

  class Cspad2x2GainMap : public QObject {
    Q_OBJECT
  public:
    Cspad2x2GainMap();
    ~Cspad2x2GainMap();
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    void initialize(QWidget* parent, QVBoxLayout* layout);
    void flush();
    void show_map (unsigned);
  public:
    Pds::CsPad2x2::CsPad2x2GainMapCfg* quad();
  public slots:
    void import_();
    void export_();
  private:
    Quad2x2GainMap* _quad[1];
    SectionDisplay2x2* _display;
    unsigned _q, _s;
  };
}

#endif
