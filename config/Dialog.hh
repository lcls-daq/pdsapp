#ifndef Pds_Dialog_hh
#define Pds_Dialog_hh

#include <QtGui/QDialog>

namespace Pds_ConfigDb {
  
  class Serializer;

  class Dialog : public QDialog {
    Q_OBJECT
  public:
    Dialog(QWidget* parent,
	   Serializer& s,
	   const QString& read_dir =".",
	   const QString& write_dir="*.xtc");
    ~Dialog();
  public:
    const QString& file() const { return _file; }
  public slots:
    void read();
    void write();
  private:
    Serializer& _s;
    const QString& _read_dir;
    const QString& _write_dir;
    QString     _file;
  };

};

#endif
