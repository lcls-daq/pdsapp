#include "pdsapp/config/EventLogic.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QLineEdit>

static const char* el0_names[] = { "", "NOT", NULL };

static const char* el_names[] = { "OR", "AND", "OR NOT", "AND NOT", NULL };

using namespace Pds_ConfigDb;

typedef Pds::TimeTool::EventLogic TTEL;

EventLogic::EventLogic(const char* name, 
		       bool lnot, 
		       unsigned code) :
  _name(name),
  _op0 ("", lnot ? Enums::Neg:Enums::Pos, el0_names)
{
  _code.push_back(new NumericInt<unsigned>("",code,0,255));
}

EventLogic::~EventLogic()
{
}

QLayout* EventLogic::initialize(QWidget* p) 
{
  QGridLayout* l = new QGridLayout;
  l->addWidget(new QLabel("Event"),0,0);
  l->addWidget(new QLabel("Op")  ,0,1,Qt::AlignHCenter);
  l->addWidget(new QLabel("Code"),0,2,Qt::AlignHCenter);
  l->addWidget(new QLabel(_name) ,1,0);
  l->addLayout(_op0     .initialize(p) ,1,1);
  l->addLayout(_code[0]->initialize(p) ,1,2);
  l->addWidget(_addB=new QPushButton("+"),2,0,Qt::AlignRight);
  _addB->setMaximumWidth(15);
  QObject::connect(_addB, SIGNAL(clicked()), this, SLOT(add_row()));
  return _layout=l;
}

void EventLogic::add_row()
{
  QPushButton* remB = new QPushButton("-");
  remB->setMaximumWidth(15);
  Enumerated<TTEL::LogicOp>* op =
    new Enumerated<TTEL::LogicOp>(NULL,
				  TTEL::L_AND,
				  el_names);
  NumericInt<unsigned>* code = 
    new NumericInt<unsigned>(NULL,0,0,255);

  int nrows = _layout->rowCount();
  _layout->removeWidget(_addB);
  _layout->addWidget(remB,nrows-1,0,Qt::AlignRight);
  _layout->addLayout(op  ->initialize(0),nrows-1,1);
  _layout->addLayout(code->initialize(0),nrows-1,2);
  _layout->addWidget(_addB,nrows,0,Qt::AlignRight);

  QObject::connect(remB, SIGNAL(clicked()), this, SLOT(remove_row()));
  _op  .push_back(op);
  _code.push_back(code);
}

static void deleteItem(QLayoutItem* item)
{
  if (item->layout()) {
    QLayoutItem* l;
    while( (l=item->layout()->takeAt(0)) )
      deleteItem(l);
  }
  else if (item->widget())
    item->widget()->hide();

  delete item;
}

void EventLogic::remove_row()
{
  QWidget* o = static_cast<QWidget*>(sender());
  int index = _layout->indexOf(o);
  if (index>=0) {
    int row,column,rspan,cspan;
    _layout->getItemPosition(index,&row,&column,&rspan,&cspan);

    for(int i=0; i<_layout->columnCount(); i++) {
      QLayoutItem* item = _layout->itemAtPosition(row,i);
      if (item) {
	_layout->removeItem(item);
	deleteItem(item);
      }
    }

    delete _op  [row-2];
    _op  [row-2] = 0;

    delete _code[row-1];
    _code[row-1] = 0;
  }
}

void EventLogic::update() 
{
  _op0.update();
  for(unsigned i=0; i<_op.size(); i++)
    if (_op[i])
      _op[i]->update();
  for(unsigned i=0; i<_code.size(); i++)
    if (_code[i])
      _code[i]->update();
}

void EventLogic::flush() 
{
  _op0.flush();
  for(unsigned i=0; i<_op.size(); i++)
    if (_op[i])
      _op[i]->flush();
  for(unsigned i=0; i<_code.size(); i++)
    if (_code[i])
      _code[i]->flush();
}

void EventLogic::enable(bool l) 
{
  _op0.enable(l);
  for(unsigned i=0; i<_op.size(); i++)
    if (_op[i])
      _op[i]->enable(l);
  for(unsigned i=0; i<_code.size(); i++)
    if (_code[i])
      _code[i]->enable(l);
}

void EventLogic::set(const ndarray<const TTEL,1>& a)
{
  for(int row=2; row<_layout->rowCount(); row++) {
    for(int i=0; i<_layout->columnCount(); i++) {
      QLayoutItem* item = _layout->itemAtPosition(row,i);
      if (item) {
	_layout->removeItem(item);
	deleteItem(item);
      }
    }
  }

  for(unsigned i=0; i<_op.size(); i++)
    delete _op  [i];
  for(unsigned i=1; i<_code.size(); i++)
    delete _code[i];

  _op  .clear();
  _code.resize(1);

  _op0     .value = (a[0].logic_op()==TTEL::L_OR ||
		     a[0].logic_op()==TTEL::L_AND) ?
    Enums::Pos : Enums::Neg;
  _code[0]->value = a[0].event_code();

  for(unsigned i=1; i<a.size(); i++) {
    QPushButton* remB = new QPushButton("-");
    remB->setMaximumWidth(15);
    Enumerated<TTEL::LogicOp>* op =
      new Enumerated<TTEL::LogicOp>(NULL,
				    a[i].logic_op(),
				    el_names);
    NumericInt<unsigned>* code = 
      new NumericInt<unsigned>(NULL,
			       a[i].event_code(),
			       0,255);
    
    _layout->addWidget(remB,i+1,0,Qt::AlignRight);
    _layout->addLayout(op  ->initialize(0),i+1,1);
    _layout->addLayout(code->initialize(0),i+1,2);

    QObject::connect(remB, SIGNAL(clicked()), this, SLOT(remove_row()));

    _op  .push_back(op);
    _code.push_back(code);
  }

  _layout->addWidget(_addB=new QPushButton("+"),a.size()+1,0);
  _addB->setMaximumWidth(15);
  QObject::connect(_addB, SIGNAL(clicked()), this, SLOT(add_row()));
}

ndarray<const TTEL,1> 
EventLogic::get() const
{
  unsigned count=0;
  for(unsigned i=0; i<_code.size(); i++)
    if (_code[i]) count++;

  ndarray<TTEL,1> a = make_ndarray<TTEL>(count);

  count=0;
  a[count++] = TTEL(_code[0]->value,
		    _op0     .value==Enums::Pos ? TTEL::L_OR : TTEL::L_OR_NOT);
  for(unsigned i=1; i<_code.size(); i++)
    if (_code[i])
      a[count++] = TTEL(_code[i]->value,
			_op[i-1]->value);

  return a;
}

#include "Parameters.icc"

