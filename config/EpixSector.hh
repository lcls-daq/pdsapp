#ifndef PdsConfigDb_EpixSector_hh
#define PdsConfigDb_EpixSector_hh

#include <QtGui/QLabel>

class QLineEdit;

namespace Pds_ConfigDb
{
  class EpixSector : public QLabel {
    Q_OBJECT
  public:
    EpixSector(unsigned q, unsigned grouping=1, bool exclusive=false);
  public:
    void     update(unsigned m);
    unsigned value() const;
  protected:
    void mousePressEvent( QMouseEvent* e );
  signals:
    void changed();  
  private:
    unsigned   _quad;
    unsigned   _value;
    unsigned   _grouping;
    bool       _exclusive;
  };
};

#endif
