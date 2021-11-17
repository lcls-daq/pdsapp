#ifndef QtChoiceConcealer_hh
#define QtChoiceConcealer_hh

#include <QtCore/QObject>

#include <vector>

class QLayout;
class QWidget;

namespace Pds_ConfigDb {

  class QtChoiceConcealer : public QObject {
    Q_OBJECT
  public:
    QtChoiceConcealer();
    ~QtChoiceConcealer();
  public:
    QLayout* add(QLayout*, int);
    QWidget* add(QWidget*, int);
  public slots:
    void show(int);
    void hide(int);
  private:
    std::vector< std::pair<QLayout*, int> > _layouts;
    std::vector< std::pair<QWidget*, int> > _widgets;
  };
};

#endif
