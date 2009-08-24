#ifndef Pds_ConfigDb_Info_Ui_hh
#define Pds_ConfigDb_Info_Ui_hh

#include <QtGui/QGroupBox>

namespace Pds_ConfigDb {
  
  class Experiment;

  class Info_Ui : public QGroupBox {
    Q_OBJECT
  public:
    Info_Ui(QWidget*    parent,
	    Experiment& expt);
    ~Info_Ui();
  public slots:
    void db_browse();
    void db_current();
  private:
    Experiment& _expt;
  };

};

#endif
