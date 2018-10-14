#ifndef Pds_Parameters_hh
#define Pds_Parameters_hh

#include "pds/service/LinkedList.hh"

#include "pdsapp/config/ParameterCount.hh"
#include "pdsapp/config/PolyDialog.hh"
#include "pdsapp/config/FilterDialog.hh"

#include <vector>

class QWidget;
class QLayout;
class QLineEdit;
class QComboBox;
class QLabel;
class QFileDialog;
class QPushButton;
class QString;
class QCheckBox;

namespace Pds_ConfigDb {
  class PolyDialog;
  class FilterDialog;

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
    Parameter();
    Parameter(const char* label);
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
    bool allowEdit() const;

    static void readFromData(bool);
    static bool readFromData();
  protected:
    const char* _label;
    bool        _allowEdit;
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

  class ParameterFile : public Parameter {
  public:
    ParameterFile(const char* p);
    virtual ~ParameterFile() {}
    virtual void mport(const QString&) = 0;
    virtual void xport(const QString&) const = 0;
  };

  template <class T>
  class Poly : public ParameterFile {
  public:
    Poly(const char* label);
    ~Poly();
    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
    void     enable(bool);

    void     mport(const QString&);
    void     xport(const QString&) const;
  public:
    std::vector<T> value;
    QLabel*        _display;
    PolyDialog*    _dialog;
    QPushButton*   _import;
    QPushButton*   _export;
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
    enum { MaxSize=256 };
    char value[MaxSize];
    QLineEdit* _input;
    unsigned   _size;
    QLabel*    _display;
  };

  class TextFileParameter: public ParameterFile {
  public:
    TextFileParameter(const char* label, unsigned maxsize);
    TextFileParameter(const char* label, unsigned maxsize, const char* filter);
    ~TextFileParameter();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
    void     enable(bool);

    void     mport(const QString&);
    void     xport(const QString&) const;

    void     set_value(const char* text);
    void     set_value(const char* text, unsigned new_version);
    unsigned length() const;
  public:
    char*          value;
    unsigned       size;
    unsigned       version;
    const char*    _filter;
    unsigned       _maxsize;
    QLabel*        _display;
    FilterDialog*  _dialog;
    QPushButton*   _import;
    QPushButton*   _export;
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

  class CheckValue : public Parameter {
  public:
    CheckValue(const char* label, bool checked);
    ~CheckValue();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
    void     enable(bool);
  public:
    bool       value;
    QCheckBox* _input;
  };
};

#endif
