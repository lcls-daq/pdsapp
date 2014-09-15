#ifndef QtConcealer_hh
#define QtConcealer_hh

#include <QtCore/QObject>

#include <vector>

class QLayout;
class QWidget;

namespace Pds_ConfigDb {

  class QtConcealer : public QObject {
    Q_OBJECT
  public:
    QtConcealer();
    ~QtConcealer();
  public:
    QLayout* add(QLayout*);
    QWidget* add(QWidget*);
  public slots:
    void show(bool);
  private:
    std::vector<QLayout*> _layouts;
    std::vector<QWidget*> _widgets;
  };
};

#endif
