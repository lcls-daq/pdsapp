#ifndef QrLabel_hh
#define QrLabel_hh

#include <QtGui/QLabel>

namespace Pds_ConfigDb {
  class QrLabel : public QLabel {
    Q_OBJECT
  public:
    QrLabel();
    QrLabel(const QString& s);
  protected:
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void  paintEvent(QPaintEvent*);
  public slots:
    void  setText(const QString&);
  };
};

#endif
