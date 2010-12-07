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
	   const QString& file);
    Dialog(QWidget* parent,
	   Serializer& s,
	   const QString& read_dir,
	   const QString& write_dir);
    Dialog(QWidget* parent,
	   Serializer& s,
	   const QString& read_dir,
	   const QString& write_dir,
	   const QString& file);
    ~Dialog();
  public:
    const QString& file() const { return _file; }
    void  showEvent(QShowEvent*);
  public slots:
    void replace  ();
    void append   ();
    void write    ();
    void set_cycle   (int);
    void insert_cycle();
    void remove_cycle();
  private:
    void layout();
    void append(const QString&);
  private:
    Serializer&         _s;
    const QString&      _read_dir;
    const QString&      _write_dir;
    QString             _file;
    QComboBox*          _cycleBox;
    std::vector<Cycle*> _cycles;
    unsigned            _current;
  };

};

#endif
