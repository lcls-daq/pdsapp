#include "MainWindow.hh"

#include "pdsapp/control/ConfigSelect.hh"
#include "pdsapp/control/PartitionSelect.hh"
#include "pdsapp/control/NodeSelect.hh"
#include "pdsapp/control/StateSelect.hh"
#include "pdsapp/control/SeqAppliance.hh"
#include "pdsapp/control/PVDisplay.hh"
#include "pdsapp/control/PVManager.hh"

#include "pds/management/QualifiedControl.hh"
#include "pds/management/ControlCallback.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/client/Decoder.hh"

#include <QtGui/QApplication>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMessageBox>

namespace Pds {
  class CCallback : public ControlCallback {
  public:
    CCallback() : _apps(new Decoder(Level::Control)) {}
  public:
    void attached(SetOfStreams& streams) {
      _apps->connect(streams.stream(StreamParams::FrameWork)->inlet());
    }
    void failed(Reason reason) {}
    void dissolved(const Node& node) {}
  public:
    void add_appliance(Appliance* app) { app->connect(_apps); }
  private:
    Appliance* _apps;
  };
  class ControlTimeout : public Routine {
  public:
    ControlTimeout(MainWindow* w) : _w(w) {}
    ~ControlTimeout() {}
  public:
    void routine() { 
      printf("Timeout waiting for %d -> %d to complete\n",
	     (_w->_control->current_state()),
	     (_w->_control->target_state()));
      _w->controleb_tmo();
    }
  private:
    MainWindow* _w;
  };
};

using namespace Pds;
using Pds_ConfigDb::Experiment;

MainWindow::MainWindow(unsigned          platform,
		       const char*       partition,
		       const char*       db_path) :
  QWidget(0),
  _controlcb(new CCallback),
  _control  (new QualifiedControl(platform, *_controlcb, new ControlTimeout(this))),
  _config   (new CfgClientNfs(Node(Level::Control,platform).procInfo()))
{
  setAttribute(Qt::WA_DeleteOnClose, true);

  ConfigSelect*     config;
  StateSelect*      state ;
  PVDisplay*        pvs;

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(config    = new ConfigSelect   (this, *_control, db_path));
  layout->addWidget(            new PartitionSelect(this, *_control, partition, db_path));
  layout->addWidget(state     = new StateSelect    (this, *_control));
  layout->addWidget(pvs       = new PVDisplay      (this, *_control));

  _pvmanager = new PVManager(*pvs);

  //  the order matters
  _controlcb->add_appliance(state);
  _controlcb->add_appliance(new SeqAppliance(*_control,*_config,
					     *_pvmanager));
  _control->attach();

  QObject::connect(state, SIGNAL(allocated())  , config, SLOT(allocated()));
  QObject::connect(state, SIGNAL(deallocated()), config, SLOT(deallocated()));
  QObject::connect(this , SIGNAL(timedout())   , this  , SLOT(handle_timeout()));
}
  
MainWindow::~MainWindow()
{
  _control->detach(); 
  delete _config;
  delete _control; 
  delete _controlcb; 
  delete _pvmanager;
}

void MainWindow::controleb_tmo()
{
  emit timedout();
}

void MainWindow::handle_timeout()
{
  QString msg = QString("Timeout waiting for\ntransition %1 -> %2 to complete")
    .arg(_control->current_state())
    .arg(_control->target_state());
  Allocation alloc = _control->eb().remaining();
  for(unsigned k=0; k<alloc.nnodes(); k++) {
    NodeSelect s(*alloc.node(k));
    msg += "\n" + s.label();
  }
  QMessageBox::warning(this, "Transition Error", msg);
}
