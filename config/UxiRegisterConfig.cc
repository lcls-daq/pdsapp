#include "pdsapp/config/UxiRegisterConfig.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>
#include <QtGui/QLabel>

#include <algorithm>
#include <set>

using namespace Pds_ConfigDb;

typedef std::map<UxiRegister*, QLayout*> RegMap;
typedef RegMap::iterator RegMapIter;
typedef std::vector<UxiRegister*> RegVec;
typedef RegVec::iterator RegVecIter;

UxiRegister::UxiRegister() :
  QObject(),
  _expert_mode(true),
  _name("Name", "", Pds::Uxi::Register::NameSize),
  _value("Value", 0x0, 0x0, 0xffffffff, Hex)
{}

UxiRegister::UxiRegister(const char* name, uint32_t value, bool expert_mode) :
  QObject(),
  _expert_mode(expert_mode),
  _name("Name", name, Pds::Uxi::Register::NameSize),
  _value("Value", value, 0x0, 0xffffffff, Hex)
{}

UxiRegister::~UxiRegister()
{}

QLayout* UxiRegister::initialize(QWidget* parent)
{
  QHBoxLayout *layout = new QHBoxLayout;
  layout->addLayout(_name.initialize(parent), 55);
  // if edit mode but not expert mode disable modifying register name
  if (Parameter::allowEdit() && !_expert_mode) {
    _name.enable(false);
  }
  layout->addLayout(_value.initialize(parent), 35);
  if (Parameter::allowEdit() && _expert_mode) {
    QPushButton *delButton = new QPushButton("X", parent);
    delButton->setMinimumWidth(20);
    layout->addWidget(delButton, 10);
    // connect the delButton to local slot that can then signal parent table
    QObject::connect(delButton, SIGNAL(clicked()), this, SLOT(delButtonClicked()));
  }
  return layout;
}

void UxiRegister::delButtonClicked()
{
  emit delRequest(this);
}

void UxiRegister::update()
{
  _name.update();
  _value.update();
}

void UxiRegister::flush()
{
  _name.flush();
  _value.flush();
}

void UxiRegister::enable(bool v)
{
  _name.enable(v);
  _value.enable(v);
}

bool UxiRegister::validate()
{
  if (strlen(_name.value) == 0) {
    QMessageBox::critical(0, "Error!", "Empty register name");
    return false;
  } else {
    return true;
  }
}

const char* UxiRegister::name() const
{
  return _name.value;
}

uint32_t UxiRegister::value() const
{
  return _value.value;
}

UxiRegisterTable::UxiRegisterTable() :
  QObject(),
  _parent(NULL),
  _expert_mode(false),
  _layout(NULL)
{}

UxiRegisterTable::UxiRegisterTable(bool expert_mode) :
  QObject(),
  _parent(NULL),
  _expert_mode(expert_mode),
  _layout(NULL)
{}

UxiRegisterTable::~UxiRegisterTable()
{
  clear();
}

void UxiRegisterTable::clearLayout(QLayout* layout)
{
  while (QLayoutItem* item = layout->takeAt(0)) {
    if (QWidget* widget = item->widget()) {
      delete widget;
    }
    if (QLayout* childLayout = item->layout()) {
      clearLayout(childLayout);
    }
    delete item;
  }
}

void UxiRegisterTable::clear()
{
  for (RegMapIter it = _regs.begin(); it != _regs.end(); ++it) {
    if (it->first) {
      delete it->first;
    }
    if (it->second) {
      clearLayout(it->second);
      delete it->second;
    }
  }
  _regs.clear();
  _regorder.clear();
}

void UxiRegisterTable::insert(Pds::LinkedList<Parameter>& pList)
{
  pList.insert(this);
}

QLayout* UxiRegisterTable::initialize(QWidget* parent)
{
  _parent = parent;
  _layout = new QVBoxLayout;

  QHBoxLayout *buttonLayout = new QHBoxLayout;
  QLabel      *buttonLabel = new QLabel("Registers:", parent);
  buttonLayout->addWidget(buttonLabel, 75);
  if (Parameter::allowEdit() && _expert_mode) {
    QPushButton *addButton = new QPushButton("Add Registers", parent);
    buttonLayout->addWidget(addButton, 25);
    QObject::connect(addButton, SIGNAL(clicked()), this, SLOT(addRegister()));
  }
  _layout->addLayout(buttonLayout);
  _layout->setSpacing(5);

  return _layout;
}

void UxiRegisterTable::addRegister()
{
  addRegister(new UxiRegister());
}

void UxiRegisterTable::delRegister(UxiRegister* reg)
{
  RegMapIter it = _regs.find(reg);
  if (it != _regs.end()) {
    RegVecIter vit = std::find(_regorder.begin(), _regorder.end(), reg);
    if (vit != _regorder.end()) {
      _regorder.erase(vit);
    }
    if (it->first) {
      delete it->first;
    }
    if (it->second) {
      clearLayout(it->second);
      delete it->second;
    }   
    _regs.erase(it);
  }
}

void UxiRegisterTable::addRegister(UxiRegister* reg)
{
  QLayout* reglayout = reg->initialize(_parent);
  _regs.insert(std::make_pair(reg, reglayout));
  _layout->addLayout(reglayout);
  _regorder.push_back(reg);
  if (Parameter::allowEdit() && _expert_mode) {
    QObject::connect(reg, SIGNAL(delRequest(UxiRegister*)), this, SLOT(delRegister(UxiRegister*)));
  }
}

void UxiRegisterTable::update()
{
  for (RegMapIter it = _regs.begin(); it != _regs.end(); ++it) {
    it->first->update();
  }
}

void UxiRegisterTable::flush()
{
  for (RegMapIter it = _regs.begin(); it != _regs.end(); ++it) {
    it->first->flush();
  }
}

void UxiRegisterTable::enable(bool v)
{
  for (RegMapIter it = _regs.begin(); it != _regs.end(); ++it) {
    it->first->enable(v);
  }
}

int UxiRegisterTable::pull(const UxiConfigType& cfg)
{
  ndarray<const Pds::Uxi::Register, 1> regs = cfg.registers();
  // clear any old register data
  clear();
  for (size_t idx=0; idx<cfg.numberOfRegisters(); idx++) {
    addRegister(new UxiRegister(regs[idx].name(), regs[idx].value(), _expert_mode));
  }

  return _regs.size();
}

int UxiRegisterTable::push(Pds::Uxi::Register* to)
{
  for (size_t idx=0; idx<_regorder.size(); idx++) {
    new(to + idx) Pds::Uxi::Register(_regorder[idx]->name(), _regorder[idx]->value());
  }

  return _regs.size();
}

bool UxiRegisterTable::validate()
{
  bool v = true;
  std::set<std::string> regnames;
  for (RegMapIter it = _regs.begin(); it != _regs.end(); ++it) {
    v &= it->first->validate();
    std::string name = it->first->name();
    if (regnames.count(name) != 0) {
      QString msg = QString("Duplicate register entries for register %1! Is it okay to ignore this?").arg(name.c_str());
      v &= !QMessageBox::warning(0,"Warning!", msg, "Ignore", "Cancel", 0, 0, 1);
    } else {
      regnames.insert(name);
    }
  }

  if (_regs.size() > UxiConfigMaxRegisters) {
    QString msg = QString("Number of requested registers %1 is greater the max of %2!").arg(_regs.size()).arg(UxiConfigMaxRegisters);
    QMessageBox::critical(0, "Error!", msg);
    v = false;
  }

  return v;
}

unsigned UxiRegisterTable::nregisters() const
{
  return _regs.size();
}
