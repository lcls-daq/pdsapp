#ifndef Pds_CfgDialog_hh
#define Pds_CfgDialog_hh

#include <QtGui/QDialog>

namespace ConfigGui {
  
  class Serializer;

  class Dialog : public QDialog {
    Q_OBJECT
  public:
    Dialog(QWidget* parent,
	   Serializer& s);
    ~Dialog();
  public slots:
    void read();
    void write();
  private:
    Serializer& _s;
  };

};

#endif
