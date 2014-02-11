#ifndef Pds_Dialog_hh
#define Pds_Dialog_hh

#include "pdsdata/xtc/TypeId.hh"

#include <QtGui/QDialog>

#include <vector>

class QComboBox;
class QShowEvent;

namespace Pds_ConfigDb {

  class Serializer;

  class Dialog : public QDialog {
    Q_OBJECT
  public:
    Dialog(QWidget* parent,
	   Serializer& s,
           const QString&,
           bool        edit=false);
    Dialog(QWidget* parent,
	   Serializer& s,
           const QString&,
	   const void* p,
           unsigned    sz,
           bool        edit=false);
    ~Dialog();
  public:
    const QString& name() const { return _name; }
    const char*    payload() const { return _payload; }
    unsigned       payload_size() const { return _payload_sz; }
  public:
    void  showEvent(QShowEvent*);
  public slots:
  //    void append   ();
    void write    ();
  private:
    void layout(bool);
    void append(const void*, unsigned);
  private:
    Serializer&         _s;
    QString             _name;
    char*               _payload;
    unsigned            _payload_sz;
  };

};

#endif
