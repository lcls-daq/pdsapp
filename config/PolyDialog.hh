#ifndef Pds_PolyDialog_hh
#define Pds_PolyDialog_hh

#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QString;
class QFileDialog;

namespace Pds_ConfigDb {
  class ParameterFile;

  class PolyDialog : public QObject {
    Q_OBJECT
  public:
    PolyDialog(ParameterFile&);    
    ~PolyDialog();
  public slots:
    void mport();
    void xport();
  private:
    ParameterFile& _p;
    QFileDialog*   _d;
  };
};

#endif
    
