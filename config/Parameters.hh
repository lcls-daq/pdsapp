#ifndef Pds_Parameters_hh
#define Pds_Parameters_hh

#include "pds/service/LinkedList.hh"

#include "pdsapp/config/ParameterCount.hh"

class QWidget;
class QLayout;
class QLineEdit;
class QComboBox;
class QLabel;

namespace Pds_ConfigDb {

  class Enums {
  public:
    enum Bool     { False, True };
    enum Polarity { Pos, Neg };
    enum Enabled  { Enable, Disable };
    enum Disabled { Disabled_Disable, Disabled_Enable };

    static const char* Bool_Names[];
    static const char* Polarity_Names[];
    static const char* Enabled_Names[];
    static const char* Disabled_Names[];
  };

  class Parameter : public Pds::LinkedList<Parameter> {
  public:
    Parameter() : _label(0) {}
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
    //
    //  Enable/Disable editing on this object
    //
    virtual void     enable(bool) = 0;

    static void allowEdit(bool);
    static bool allowEdit();
  protected:
    const char* _label;
  };

  enum IntMode { Decimal, Hex, Scaled };

  class ParameterSet;

  template <class T>
  class NumericInt : public Parameter,
         public ParameterCount {
  public:
    NumericInt(const char* label, T val, T vlo, T vhi, IntMode mo=Decimal, double sca=1.);
    ~NumericInt();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
    void     enable(bool);
  public:
    bool     connect(ParameterSet&);
    unsigned count  ();
  public:
    QWidget* widget();
  public:
    T       value;
    T       range[2];
    IntMode mode;
    double  scale;
    QLineEdit* _input;
    QLabel*    _display;
  };

  template <class T>
  class NumericFloat : public Parameter {
  public:
    NumericFloat();
    NumericFloat(const char* label, T val, T vlo, T vhi);
    ~NumericFloat();

    NumericFloat& operator=(const NumericFloat&);

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
    void     enable(bool);
  public:
    T value;
    T range[2];
    QLineEdit* _input;
    QLabel*    _display;
  };

  template <class T>
  class Enumerated : public Parameter {
  public:
    Enumerated(const char* label, T val, const char** strlist);
    Enumerated(const char* label, T val, const char** strlist, const T* vals);
    ~Enumerated();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
    void     enable(bool);
  public:
    T value;
    const char** labels;
    QComboBox*   _input;
    QLabel*      _display;
    const T*     values;
  };

  class TextParameter : public Parameter {
  public:
    TextParameter(const char* label, const char* val, unsigned size);
    ~TextParameter();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
    void     enable(bool);
  public:
    QWidget* widget();
  public:
    enum { MaxSize=128 };
    char value[MaxSize];
    QLineEdit* _input;
    unsigned   _size;
    QLabel*    _display;
  };

  template <class T, int N>
  class NumericIntArray : public Parameter {
  public:
    NumericIntArray(const char* label, T val, T vlo, T vhi, IntMode mo=Decimal, double sca=1.);
    ~NumericIntArray();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
    void     enable(bool);
  public:
    void     setWidth(unsigned);
  public:
    T       value[N];
    T       range[2];
    IntMode mode;
    double  scale;
    QLineEdit* _input[N];
    QLabel*    _display[N];
  };

};

#endif
