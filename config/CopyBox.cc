#include "pdsapp/config/CopyBox.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QPushButton>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>

namespace Pds_ConfigDb {
  class CopyBox::PrivateData {
  public:
    PrivateData(unsigned r, unsigned c) : _rows(r), _columns(c) {}
  public:
    unsigned                        _rows;
    unsigned                        _columns;
    QButtonGroup*                   _fromGroup;
    QButtonGroup*                   _toGroup;
    QPushButton*                    _copyB;
    QPushButton*                    _noneB;
    QPushButton*                    _allB;
  };
};

using namespace Pds_ConfigDb;

CopyBox::CopyBox(const char* title, 
                 const char* rowLabel,
                 const char* columnLabel,
                 unsigned    rows, 
                 unsigned    columns) : 
  QGroupBox(title), 
  _private(new PrivateData(rows,columns))
{
  QGroupBox* copyBox = this;
  (_private->_fromGroup = new QButtonGroup)->setExclusive(true);
  (_private->_toGroup   = new QButtonGroup)->setExclusive(false);
  QHBoxLayout* hl = new QHBoxLayout;
  { QGridLayout* from = new QGridLayout;
    QGridLayout* to   = new QGridLayout;
    from->addWidget(new QLabel("Copy From"),0,0,1,11);
    to  ->addWidget(new QLabel("Copy To"  ),0,0,1,5);
    to  ->addWidget(_private->_noneB=new QPushButton("None"),0,4,1,3);
    to  ->addWidget(_private->_allB =new QPushButton("All" ),0,7,1,3);
    to  ->addWidget(_private->_copyB=new QPushButton("COPY"),0,10,1,4);
    for(unsigned q=0; q<rows; q++) {
      from->addWidget(new QLabel(QString("%1%2").arg(rowLabel).arg(q)),q+2,0);
      to  ->addWidget(new QLabel(QString("%1%2").arg(rowLabel).arg(q)),q+2,0);
    }
    for(unsigned c=0; c<columns; c++) {
      from->addWidget(new QLabel(QString("%1%2").arg(columnLabel).arg(c)),1,c+1);
      to  ->addWidget(new QLabel(QString("%1%2").arg(columnLabel).arg(c)),1,c+1);
    }
    for(unsigned q=0; q<rows; q++) 
      for(unsigned c=0; c<columns; c++) {
        QRadioButton* b = new QRadioButton;
        from->addWidget(b,q+2,c+1);
        _private->_fromGroup->addButton(b,q*columns+c);
        b = new QRadioButton;
        to->addWidget(b,q+2,c+1);
        _private->_toGroup->addButton(b,q*columns+c);
      }
    hl->addLayout(from);
    hl->addLayout(to);
  }
  copyBox->setLayout(hl);

  ::QObject::connect(_private->_noneB, SIGNAL(pressed()), this, SLOT(none()));
  ::QObject::connect(_private->_allB , SIGNAL(pressed()), this, SLOT(all ()));
  ::QObject::connect(_private->_copyB, SIGNAL(pressed()), this, SLOT(copy()));
}

CopyBox::~CopyBox() { delete _private; }

void CopyBox::all()
{
  QList<QAbstractButton*> b = _private->_toGroup->buttons();
  for(QList<QAbstractButton*>::iterator it=b.begin(); it!=b.end(); it++)
    (*it)->setChecked(true);
}

void CopyBox::none()
{
  QList<QAbstractButton*> b = _private->_toGroup->buttons();
  for(QList<QAbstractButton*>::iterator it=b.begin(); it!=b.end(); it++)
    (*it)->setChecked(false);
}

void CopyBox::copy()
{
  const unsigned rows    = _private->_rows;
  const unsigned columns = _private->_columns;
  int id = _private->_fromGroup->checkedId();
  if (id >= 0) {
    initialize();
    for(unsigned i=0; i<rows; i++)
      for(unsigned j=0; j<columns; j++)
        if (_private->_toGroup->button(i*columns+j)->isChecked()) 
          copyItem( id/columns, id%columns, i, j);
    finalize();
  }
}

