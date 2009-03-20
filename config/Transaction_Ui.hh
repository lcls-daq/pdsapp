#ifndef Pds_ConfigDb_Transaction_Ui_hh
#define Pds_ConfigDb_Transaction_Ui_hh

#include <QtGui/QGroupBox>

namespace Pds_ConfigDb {
  
  class Experiment;

  class Transaction_Ui : public QGroupBox {
    Q_OBJECT
  public:
    Transaction_Ui(QWidget*    parent,
		   Experiment& expt);
    ~Transaction_Ui();
  public slots:
    void db_clear();
    void db_commit();
    void db_update();
  signals:
    void db_changed();
  private:
    Experiment& _expt;
  };

};

#endif
