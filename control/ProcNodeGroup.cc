#include "pdsapp/control/ProcNodeGroup.hh"
#include "pdsapp/control/Preferences.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QPushButton>
#include <QtGui/QFileDialog>
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
  _triggered(false)
{
  QHBoxLayout* l = new QHBoxLayout;
  l->addWidget(_l3f_box    = new QCheckBox("L3F"));
  l->addWidget(_l3f_data   = new QPushButton("<data_file>"));
  l->addWidget(_l3f_action = new QComboBox);
  _l3f_layout = l;

  _l3f_action->addItem("Tag");
  _l3f_action->addItem("Veto");
  _l3f_action->setCurrentIndex(0);

  _input_data = new QFileDialog(_l3f_data);
  _input_data->setFileMode(QFileDialog::ExistingFile);
  _input_data->selectFile(QString("%1/.").arg(getenv("HOME")));

  connect(_l3f_data, SIGNAL(clicked()), this, SLOT(select_file()));
  connect(_l3f_box, SIGNAL(stateChanged(int)), this, SLOT(action_change(int)));
  connect(_l3f_action, SIGNAL(currentIndexChanged(int)), this, SLOT(action_change(int)));

  _palette[0] = new QPalette;
  _palette[1] = new QPalette(Qt::green);
  _palette[2] = new QPalette(Qt::yellow);

  _read_pref();
}

ProcNodeGroup::~ProcNodeGroup() 
{
  delete _input_data;
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
    _input_data->selectFile(s[2]);
    _l3f_data->setText(s[2]);
  }
}

void ProcNodeGroup::_write_pref() const
{
  QString t = QString("%1_l3f").arg(title());
  Preferences p(qPrintable(t),_platform,"w");
  p.write(QString("L3FEnabled"),_l3f_box->isChecked()?cTruth:cFalse);
  p.write(QString("L3FVeto"),_l3f_action->currentIndex()>0?cTruth:cFalse);
  p.write(inputData());
}

bool ProcNodeGroup::useL3F() const
{
  return _triggered &&
    _l3f_box->isChecked(); 
}

QString ProcNodeGroup::inputData() const { return _l3f_data->text(); }

bool ProcNodeGroup::useVeto() const { return _l3f_action->currentIndex()>0; }

void ProcNodeGroup::select_file()
{
  if (_input_data->exec()) {
    QStringList ofiles = _input_data->selectedFiles();
    if (ofiles.length()>0)
      _l3f_data->setText(ofiles[0]);
  }
}

void ProcNodeGroup::action_change(int)
{
  int pIndex = 0;
  if (_l3f_box->isChecked())
    pIndex = _l3f_action->currentIndex()==0 ? 1 : 2;

  _l3f_action->setPalette( *_palette[pIndex] );
}

int ProcNodeGroup::order(const NodeSelect& node, const QString& text)
{
  if (node.node().triggered() && !_triggered) {
    _triggered=true;
    static_cast<QVBoxLayout*>(layout())->insertLayout(1,_l3f_layout);
  }
    
  return NodeGroup::order(node,text);
}

