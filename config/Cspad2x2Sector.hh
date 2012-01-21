#ifndef PdsConfigDb_Cspad2x2Sector_hh
#define PdsConfigDb_Cspad2x2Sector_hh

#include <QtGui/QLabel>

class QLineEdit;

namespace Pds_ConfigDb
{
  class Cspad2x2Sector : public QLabel {
  public:
    Cspad2x2Sector(QLineEdit& edit, unsigned q=0);
  public:
    void update(unsigned m);
  protected:
    void mousePressEvent( QMouseEvent* e );
  private:
    QLineEdit& _edit;
    unsigned   _quad;
  };
};

#endif
