#ifndef PdsConfigDb_CspadSector_hh
#define PdsConfigDb_CspadSector_hh

#include <QtGui/QLabel>

class QLineEdit;

namespace Pds_ConfigDb
{
  class CspadSector : public QLabel {
  public:
    CspadSector(QLineEdit& edit, unsigned q);
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
