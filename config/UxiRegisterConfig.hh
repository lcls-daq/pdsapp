#ifndef PdsConfigDb_UxiRegisterConfig_hh
#define PdsConfigDb_UxiRegisterConfig_hh

#include "pds/config/UxiConfigType.hh"
#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>
#include <QtGui/QWidget>
#include <vector>
#include <map>

class QVBoxLayout;

namespace Pds_ConfigDb {
  class UxiRegister : public QObject, public Parameter {
    Q_OBJECT
  public:
    UxiRegister();
    UxiRegister(const char* name, uint32_t value, bool expert_mode);
    ~UxiRegister();
  public:
    QLayout* initialize(QWidget* parent);
    void update();
    void flush ();
    void enable(bool v);
    bool validate();
    const char* name() const;
    uint32_t value() const;
  signals:
    void delRequest(UxiRegister*);
  private slots:
    void delButtonClicked();
  private:
    bool                 _expert_mode;
    TextParameter        _name;
    NumericInt<uint32_t> _value;
  };

  class UxiRegisterTable : public QObject, public Parameter {
    Q_OBJECT
  public:
    UxiRegisterTable();
    UxiRegisterTable(bool expert_mode);
    ~UxiRegisterTable();
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    QLayout* initialize(QWidget* parent);
    void update();
    void flush ();
    void enable(bool v);
    int pull(const Pds::Uxi::ConfigV4& cfg);
    int push(Pds::Uxi::Register* to);
    bool validate();
    unsigned nregisters() const;
  private:
    void clear();
    void clearLayout(QLayout* layout);
    void addRegister(UxiRegister* reg);
  private slots:
    void addRegister();
    void delRegister(UxiRegister* reg);
  private:
    QWidget*                  _parent;
    bool                      _expert_mode;
    QVBoxLayout*              _layout;
    std::vector<UxiRegister*> _regorder;
    std::map<UxiRegister*, QLayout*> _regs;
  };
}

#endif
