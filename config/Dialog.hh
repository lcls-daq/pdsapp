#ifndef Pds_Dialog_hh
#define Pds_Dialog_hh

#include <QtGui/QDialog>

#include <vector>

class QComboBox;
class QShowEvent;

namespace Pds_ConfigDb {

  class Serializer;
  class Cycle;

  class Dialog : public QDialog {
    Q_OBJECT
  public:
    Dialog(QWidget* parent,
	   Serializer& s,
	   const void* p,
           unsigned    sz);
    Dialog(QWidget* parent,
	   Serializer& s,
	   const QString& file);
    Dialog(QWidget* parent,
	   Serializer& s,
	   const QString& read_dir,
	   const QString& write_dir,
	   bool lEdit);
    Dialog(QWidget* parent,
	   Serializer& s,
	   const QString& read_dir,
	   const QString& write_dir,
	   const QString& file,
	   bool lEdit);
    ~Dialog();
  public:
    const QString& file() const { return _file; }
    void  showEvent(QShowEvent*);
  public slots:
    void replace  ();
    void append   ();
    void write    ();
  private:
    void layout(bool);
    void append(const QString&);
    void append(const void*, unsigned);
  private:
    Serializer&         _s;
    const QString&      _read_dir;
    const QString&      _write_dir;
    QString             _file;
    std::vector<Cycle*> _cycles;
    unsigned            _current;
  };

};

#endif
