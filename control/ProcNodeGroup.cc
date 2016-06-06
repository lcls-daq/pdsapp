#include "pdsapp/control/ProcNodeGroup.hh"
#include "pdsapp/control/Preferences.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QPushButton>
#include <QtGui/QFileDialog>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPalette>

using namespace Pds;

static const char* cTruth = "T";
static const char* cFalse = "F";

ProcNodeGroup::ProcNodeGroup(const QString& label, 
			     QWidget*       parent, 
			     unsigned       platform, 
			     int            iUseReadoutGroup, 
			     bool           useTransient) :
  NodeGroup(label, parent, platform,
	    iUseReadoutGroup, useTransient),
  _triggered(false),
  _l3f_unbiasv(0)
{
  QGridLayout* l = new QGridLayout;
  l->addWidget(_l3f_box    = new QCheckBox("L3F"), 0, 0, 2, 1);
  l->addWidget(_l3f_data   = new QPushButton("[data_file]"), 0, 1, Qt::AlignRight);
  l->addWidget(_l3f_action = new QComboBox, 0, 2, Qt::AlignLeft);
  l->addWidget(_l3f_unbiasl= new QLabel("Unbiased Fraction"), 1, 1, Qt::AlignRight);
  l->addWidget(_l3f_unbias = new QLineEdit("0"), 1, 2, Qt::AlignLeft);
  _l3f_layout = l;

  _l3f_action->addItem("Tag");
  _l3f_action->addItem("Veto");
  _l3f_action->setCurrentIndex(0);

  _l3f_unbiasl->setVisible(false);
  _l3f_unbias ->setVisible(false);

  connect(_l3f_data, SIGNAL(clicked()), this, SLOT(select_file()));
  connect(_l3f_box, SIGNAL(stateChanged(int)), this, SLOT(action_change(int)));
  connect(_l3f_action, SIGNAL(currentIndexChanged(int)), this, SLOT(action_change(int)));
  connect(_l3f_unbias, SIGNAL(editingFinished()), this, SLOT(unbias_change()));

  _palette[0] = new QPalette;
  _palette[1] = new QPalette(Qt::green);
  _palette[2] = new QPalette(Qt::yellow);

  _read_pref();
}

ProcNodeGroup::~ProcNodeGroup() 
{
  delete _palette[0];
  delete _palette[1];
  delete _palette[2];
}

void ProcNodeGroup::_read_pref()
{
  QString t = QString("%1_l3f").arg(title());
  Preferences p(qPrintable(t),_platform,"r");
  QStringList s;
  QList<bool> v;
  p.read(s,v,cTruth);

  if (v.size()>0)  _l3f_box->setChecked(v[0]);
  if (v.size()>1)  _l3f_action->setCurrentIndex(v[1]?1:0);
  if (v.size()>2) {
    _l3f_data->setText(s[2]);
  }
  if (v.size()>3) {
    _l3f_unbias->setText(s[3]);
    _l3f_unbiasv = QString(s[3]).toDouble();
  }
}

void ProcNodeGroup::_write_pref() const
{
  QString t = QString("%1_l3f").arg(title());
  Preferences p(qPrintable(t),_platform,"w");
  p.write(QString("L3FEnabled"),_l3f_box->isChecked()?cTruth:cFalse);
  p.write(QString("L3FVeto"),_l3f_action->currentIndex()>0?cTruth:cFalse);
  p.write(inputData());
  p.write(_l3f_unbias->text());
}

bool ProcNodeGroup::useL3F() const
{
  return _triggered &&
    _l3f_box->isChecked(); 
}

QString ProcNodeGroup::inputData() const { return _l3f_data->text(); }

bool ProcNodeGroup::useVeto() const { return _l3f_action->currentIndex()>0; }

float ProcNodeGroup::unbiased_fraction() const 
{
  return _l3f_unbias->text().toFloat();
}

void ProcNodeGroup::select_file()
{
  QFileDialog* input_data = new QFileDialog(_l3f_data);
  input_data->setFileMode(QFileDialog::ExistingFile);
  input_data->selectFile(QString("%1/.").arg(getenv("HOME")));

  if (input_data->exec()) {
    QStringList ofiles = input_data->selectedFiles();
    if (ofiles.length()>0)
      _l3f_data->setText(ofiles[0]);
  }
  delete input_data;
}

void ProcNodeGroup::action_change(int)
{
  int pIndex = 0;
  if (_l3f_box->isChecked())
    pIndex = _l3f_action->currentIndex()==0 ? 1 : 2;

  _l3f_action->setPalette( *_palette[pIndex] );

  _l3f_unbiasl->setVisible( pIndex==2 );
  _l3f_unbias ->setVisible( pIndex==2 );
}

void ProcNodeGroup::unbias_change()
{
  bool lok;
  float v = _l3f_unbias->text().toDouble(&lok);
  lok &= (v>=0 && v<=1);
  if (lok)
    _l3f_unbiasv=v;
  else
    _l3f_unbias->setText(QString::number(_l3f_unbiasv));
}

int ProcNodeGroup::order(const NodeSelect& node, const QString& text)
{
  if (node.node().triggered() && !_triggered) {
    _triggered=true;
    static_cast<QVBoxLayout*>(layout())->insertLayout(1,_l3f_layout);
  }
    
  return NodeGroup::order(node,text);
}

