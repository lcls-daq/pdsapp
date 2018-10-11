#ifndef Pds_FilterDialog_hh
#define Pds_FilterDialog_hh

#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QString;
class QFileDialog;

namespace Pds_ConfigDb {
  class ParameterFile;

  class FilterDialog : public QObject {
    Q_OBJECT
  public:
    FilterDialog(ParameterFile&);
    FilterDialog(ParameterFile&, const QString& filter,
                 const QString& caption = QString(), const QString& directory = QString());
    ~FilterDialog();
  public slots:
    void mport();
    void xport();
  private:
    ParameterFile& _p;
    QFileDialog*   _d;
  };
};

#endif
    
