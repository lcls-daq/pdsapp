#include "MainWindow.hh"

#include "pdsapp/control/ConfigSelect.hh"
#include "pdsapp/control/PartitionSelect.hh"
#include "pdsapp/control/NodeSelect.hh"
#include "pdsapp/control/StateSelect.hh"
#include "pdsapp/control/SeqAppliance.hh"
#include "pdsapp/control/PVDisplay.hh"
#include "pdsapp/control/PVManager.hh"
#include "pdsapp/control/RunStatus.hh"
#include "pdsapp/control/ControlLog.hh"

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

#include <stdlib.h>

namespace Pds {
  class CCallback : public ControlCallback {
  public:
    CCallback(MainWindow& w) : _w(w), _apps(0) {}
  public:
    void attached(SetOfStreams& streams) {
      if (_apps) _apps->connect(streams.stream(StreamParams::FrameWork)->inlet());
      _w.log().append("Connected to platform.\n");
    }
    void failed   (Reason reason   ) { _w.platform_error(); }
    void dissolved(const Node& node) {}
  public:
    void add_appliance(Appliance* app) { 
      if (!_apps) _apps = app;
      else        app->connect(_apps); }
  private:
    MainWindow& _w;
    Appliance*  _apps;
  };
  
  class ControlTimeout : public Routine {
  public:
    ControlTimeout(MainWindow& w) : _w(w) {}
    ~ControlTimeout() {}
  public:
    void routine() { _w.controleb_tmo(); }
  private:
    MainWindow& _w;
  };
};

using namespace Pds;
using Pds_ConfigDb::Experiment;

MainWindow::MainWindow(unsigned          platform,
		       const char*       partition,
		       const char*       db_path) :
  QWidget(0),
  _controlcb(new CCallback(*this)),
  _control  (new QualifiedControl(platform, *_controlcb, new ControlTimeout(*this))),
  _config   (new CfgClientNfs(Node(Level::Control,platform).procInfo()))
{
  setAttribute(Qt::WA_DeleteOnClose, true);

  ConfigSelect*     config;
  StateSelect*      state ;
  PVDisplay*        pvs;
  RunStatus*        run;

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(config    = new ConfigSelect   (this, *_control, db_path));
  layout->addWidget(            new PartitionSelect(this, *_control, partition, db_path));
  layout->addWidget(state     = new StateSelect    (this, *_control));
  layout->addWidget(pvs       = new PVDisplay      (this, *_control));
  layout->addWidget(run       = new RunStatus      (this));
  layout->addWidget(_log      = new ControlLog);

  _pvmanager = new PVManager(*pvs);

  //  the order matters
  _controlcb->add_appliance(run);
  _controlcb->add_appliance(new Decoder(Level::Control));
  _controlcb->add_appliance(state);
  _controlcb->add_appliance(new SeqAppliance(*_control,*_config,
					     *_pvmanager));
  _control->attach();

  QObject::connect(state, SIGNAL(allocated())  , config, SLOT(allocated()));
  QObject::connect(state, SIGNAL(deallocated()), config, SLOT(deallocated()));
  QObject::connect(this , SIGNAL(timedout())   , this  , SLOT(handle_timeout()));
  //  QObject::connect(this , SIGNAL(platform_failed()), this, SLOT(handle_platform_error()));
}
  
MainWindow::~MainWindow()
{
  _control->detach(); 
  delete _config;
  delete _control; 
  delete _controlcb; 
  delete _pvmanager;
}

ControlLog& MainWindow::log() { return *_log; }

void MainWindow::controleb_tmo()
{
  emit timedout();
}

void MainWindow::platform_error()
{
  _log->append("Platform failed.\n  Restarting \"source\" application.\n");
  system("restart_source");
  _log->append("Attaching ...\n");
  //  _control->detach();
  _control->attach();
}

void MainWindow::handle_timeout()
{
  QString msg = QString("Timeout waiting for transition %1 -> %2 to complete.\n")
    .arg(_control->current_state())
    .arg(_control->target_state ());
  msg += QString("Failing nodes:\n");
  Allocation alloc = _control->eb().remaining();
  for(unsigned k=0; k<alloc.nnodes(); k++) {
    NodeSelect s(*alloc.node(k));
    msg += s.label() + QString("\n");
  }
  msg += QString("Resetting...\n");

  _log->append(msg);

  QMessageBox::warning(this, "Transition Error", msg);

  system("restart_nodes");


}

