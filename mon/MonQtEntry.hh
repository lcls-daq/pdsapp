#ifndef Pds_MonQtEntry_hh
#define Pds_MonQtEntry_hh

#include <QtGui/QWidget>

class QLineEdit;

namespace Pds {

  class MonQtEntry : public QWidget {
  public:
    MonQtEntry(const char* title, QWidget* p);
    ~MonQtEntry() {}
  public:
    void setEntry(const char*);
  protected:
    QLineEdit* _line;
  };

  class MonQtFloatEntry : public MonQtEntry {
  public:
    MonQtFloatEntry(const char* title, float v, QWidget* p);
    ~MonQtFloatEntry() {}
  public:
    float entry() const;
    void  setEntry(float);
  };

  class MonQtIntEntry : public MonQtEntry {
  public:
    MonQtIntEntry(const char* title, int v, QWidget* p);
    ~MonQtIntEntry() {}
  public:
    int  entry() const;
    void setEntry(int);
  };
};

#endif
