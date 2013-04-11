#ifndef Pds_Serializer_hh
#define Pds_Serializer_hh

#include "pdsapp/config/Parameters.hh"
#include <QtCore/QString>

class QBoxLayout;
class QWidget;

namespace Pds_ConfigDb {
  class Path;
  class Serializer {
  public:
    Serializer(const char* l);
    virtual ~Serializer();
  public:
    virtual int           readParameters (void* from) = 0;
    virtual int           writeParameters(void* to) = 0;
    virtual int           dataSize() const = 0;
    virtual bool          validate() { return true; }
  public:
    void initialize(QWidget*, QBoxLayout*);
    void flush ();
    void update();
    void setPath(const Path&);
    void name(const char*);
  protected:
    const char*                label;
    Pds::LinkedList<Parameter> pList;
    const Path*                path;
    QString                    _name;
  };

};

#endif
