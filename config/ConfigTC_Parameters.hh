#ifndef Pds_CfgParameter_hh
#define Pds_CfgParameter_hh

#include "pds/service/LinkedList.hh"

#include <string.h>

class QWidget;
class QLayout;
class QLineEdit;
class QComboBox;

namespace ConfigGui {

  class Enums {
  public:
    enum Bool     { False, True };
    enum Polarity { Pos, Neg };
    enum Enabled  { Enable, Disable };

    static const char* Bool_Names[];
    static const char* Polarity_Names[];
    static const char* Enabled_Names[];
  };

  class Parameter : public Pds::LinkedList<Parameter> {
  public:
    Parameter(const char* label) : _label(label) {}
    virtual ~Parameter() {}

    virtual QLayout* initialize(QWidget*) = 0;
    virtual void     update() = 0;
    virtual void     flush () = 0;
  protected:
    const char* _label;
  };

  template <class T>
  class NumericInt : public Parameter {
  public:
    NumericInt(const char* label, T val, T vlo, T vhi);
    ~NumericInt();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
  public:
    T value;
    T range[2];
    QLineEdit* _input;
  };

  template <class T>
  class NumericFloat : public Parameter {
  public:
    NumericFloat(const char* label, T val, T vlo, T vhi);
    ~NumericFloat();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
  public:
    T value;
    T range[2];
    QLineEdit* _input;
  };

  template <class T>
  class Enumerated : public Parameter {
  public:
    Enumerated(const char* label, T val, const char** strlist);
    ~Enumerated();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
  public:
    T value;
    const char** labels;
    QComboBox*   _input;
  };

};

#endif
