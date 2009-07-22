#ifndef Pds_Parameters_hh
#define Pds_Parameters_hh

#include "pds/service/LinkedList.hh"

class QWidget;
class QLayout;
class QLineEdit;
class QComboBox;

namespace Pds_ConfigDb {

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

    //
    //  Create the QWidgets used for input/output
    //
    virtual QLayout* initialize(QWidget*) = 0;
    //
    //  Update the internal value from the widgets
    //
    virtual void     update() = 0;
    //
    //  Update the widgets from the internal value
    //
    virtual void     flush () = 0;

    static void allowEdit(bool);
    static bool allowEdit();
  protected:
    const char* _label;
  };

  enum IntMode { Decimal, Hex };

  template <class T>
  class NumericInt : public Parameter {
  public:
    NumericInt(const char* label, T val, T vlo, T vhi, IntMode mo=Decimal);
    ~NumericInt();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
  public:
    T       value;
    T       range[2];
    IntMode mode;
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

  class TextParameter : public Parameter {
  public:
    TextParameter(const char* label, const char* val, unsigned size);
    ~TextParameter();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
  public:
    enum { MaxSize=128 };
    char value[MaxSize];
    QLineEdit* _input;
    unsigned   _size;
  };
};

#endif
