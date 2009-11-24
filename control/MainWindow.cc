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
#include "pdsapp/control/MySqlRunAllocator.hh"

#include "pds/offlineclient/OfflineClient.hh"
#include "pds/management/QualifiedControl.hh"
#include "pds/management/ControlCallback.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/client/DamageBrowser.hh"
#include "pds/client/Decoder.hh"

#include <QtGui/QApplication>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMessageBox>
#include <QtCore/QTime>

#include <stdlib.h>

// forward declaration
static int setup_unix_signal_handlers();

// Unix signal support
static int sigintFd[2];
static int sigtermFd[2];

namespace Pds {
  class CCallback : public ControlCallback {
  public:
    CCallback(MainWindow& w) : _w(w), _apps(0) {}
  public:
    void attached(SetOfStreams& streams) {
      if (_apps) _apps->connect(streams.stream(StreamParams::FrameWork)->inlet());
      _w.log().appendText("Connected to platform.\n");
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
    void routine() { 
      _w.controleb_tmo(); 
    }
  private:
    MainWindow& _w;
  };

  class ControlDamage : public Appliance {
  public:
    ControlDamage(MainWindow& w) : _w(w) {}
    ~ControlDamage() {}
  public:
    Transition* transitions(Transition* tr) { return tr; }
    InDatagram* events     (InDatagram* dg) 
    { if (dg->datagram().xtc.damage.value())
	_w.transition_damaged(*dg);
      return dg;
    }
  private:
    MainWindow& _w;
  };

  class FileReport : public Appliance {
    enum { NotRecording=0xffffffff };
  public:
    FileReport(ControlLog& log) :
      _log(log),
      _experiment_number(0),
      _run_number(0)
    {
    }
    ~FileReport() {}

  public:
    Transition* transitions(Transition* tr)
    {
      if (tr->id()==TransitionId::BeginRun) {
	if (tr->size() == sizeof(Transition)) {  // No RunInfo
	  _run_number = NotRecording;
	}
	else {
          RunInfo& rinfo = *reinterpret_cast<RunInfo*>(tr);
          _run_number = rinfo.run();
          _experiment_number = rinfo.experiment();
	}
      }
    return tr;
    }

    InDatagram* events     (InDatagram* dg) 
    { 
      if (dg->datagram().seq.service()==TransitionId::BeginRun) {
	if (_run_number == NotRecording) {
	  _log.appendText(QString("Not recording."));
	}
	else {
	  char fname[256];
	  sprintf(fname, "e%d-r%04d-sNN-cNN.xtc", 
		  _experiment_number, _run_number);
	  _log.appendText(QString("Data file: %1\n").arg(fname));
	}
      }
      return dg;
    }
  private:
    ControlLog& _log;
    unsigned int _experiment_number;
    unsigned int _run_number;
  };

};

using namespace Pds;
using Pds_ConfigDb::Experiment;

MainWindow::MainWindow(unsigned          platform,
		       const char*       partition,
		       const char*       db_path,
		       const char*       offlinerc,
		       const char*       experiment) :
  QWidget(0),
  _controlcb(new CCallback(*this)),
  _control  (new QualifiedControl(platform, *_controlcb, new ControlTimeout(*this))),
  _config   (new CfgClientNfs(Node(Level::Control,platform).procInfo()))
{
  setAttribute(Qt::WA_DeleteOnClose, true);

  ConfigSelect*     config;
  PartitionSelect*  ps ;
  StateSelect*      state ;
  PVDisplay*        pvs;
  RunStatus*        run;
  unsigned int      experiment_number = 0;

  if (offlinerc) {
    // offline database
    _offlineclient = new OfflineClient(offlinerc, partition, experiment);
    _runallocator = new MySqlRunAllocator(_offlineclient);
    experiment_number = _offlineclient->GetExperimentNumber();
    _control->set_experiment(experiment_number);
    printf("MainWindow(): GetExperimentNumber() returned %u\n", experiment_number);
  } else {
    _runallocator = new RunAllocator;
    // NULL offline database
    _offlineclient = (OfflineClient*)NULL;
  }
  _control->set_runAllocator(_runallocator);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(config    = new ConfigSelect   (this, *_control, db_path));
  layout->addWidget(ps        = new PartitionSelect(this, *_control, partition, db_path));
  layout->addWidget(state     = new StateSelect    (this, *_control));
  layout->addWidget(pvs       = new PVDisplay      (this, *_control));
  layout->addWidget(run       = new RunStatus      (this, *ps));
  layout->addWidget(_log      = new ControlLog);

  _pvmanager = new PVManager(*pvs);

  //  the order matters
  _controlcb->add_appliance(run);    // must be first
  _controlcb->add_appliance(new Decoder(Level::Control));
  _controlcb->add_appliance(new ControlDamage(*this));
  _controlcb->add_appliance(new FileReport(*_log));
  _controlcb->add_appliance(state);
  _controlcb->add_appliance(new SeqAppliance(*_control,*_config,
					     *_pvmanager));
  _control->attach();

  QObject::connect(state , SIGNAL(allocated())  , config, SLOT(allocated()));
  QObject::connect(state , SIGNAL(deallocated()), config, SLOT(deallocated()));
  QObject::connect(this  , SIGNAL(transition_failed(const QString&, bool))   , 
		   this  , SLOT(handle_failed_transition(const QString&, bool)));
  //  QObject::connect(this , SIGNAL(platform_failed()), this, SLOT(handle_platform_error()));

  // Unix signal support
  if (setup_unix_signal_handlers() ||
      (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd)) ||
      (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))) {
    // setup of Unix signal handlers failed
    printf("Couldn't set up Unix signal handlers\n");
  } else {
    // create socket notifiers to trigger Qt signals from Unix signals
    snInt = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, this);
    connect(snInt, SIGNAL(activated(int)), this, SLOT(handle_sigint()));
    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
    connect(snTerm, SIGNAL(activated(int)), this, SLOT(handle_sigterm()));
  }
}
  
MainWindow::~MainWindow()
{
  _control->detach(); 
  delete _config;
  delete _control; 
  delete _controlcb; 
  delete _pvmanager;
  delete _offlineclient;
}

ControlLog& MainWindow::log() { return *_log; }

void MainWindow::controleb_tmo()
{
  Allocation alloc = _control->eb().remaining();
  // kludge: weaver  we get some timeouts even though the transition is complete
  if (alloc.nnodes()) {
    QString msg = QString("Timeout waiting for transition %1 -> %2 to complete.\n")
      .arg(PartitionControl::name(_control->current_state()))
      .arg(PartitionControl::name(_control->target_state ()));
    msg += QString("Failing nodes:\n");
    for(unsigned k=0; k<alloc.nnodes(); k++) {
      NodeSelect s(*alloc.node(k));
      msg += s.label() + QString("\n");
    }
    msg += QString("Need to restart.\n");
    
    emit transition_failed(msg,true);
  }
}

void MainWindow::transition_damaged(const InDatagram& dg)
{
  DamageBrowser b(dg);
  const std::list<Xtc>& damaged = b.damaged();

  QString msg = QString("%1 damaged (0x%2):").
    arg(TransitionId::name(dg.datagram().seq.service())).
    arg(QString::number(dg.datagram().xtc.damage.value(),16));
  for(std::list<Xtc>::const_iterator it=damaged.begin(); it!=damaged.end(); it++) {
    const Xtc& xtc = *it;
    if (xtc.src.level()==Level::Source) {
      const DetInfo& info = static_cast<const DetInfo&>(xtc.src);
      msg += QString("\n  %1 : 0x%2").arg(DetInfo::name(info)).arg(QString::number(xtc.damage.value(),0,16));
    }
    else {
      const ProcInfo& info = static_cast<const ProcInfo&>(xtc.src);
      struct in_addr inaddr;
      inaddr.s_addr = ntohl(info.ipAddr());
      msg += QString("\n  %1 : %2 : %3 : 0x%4").
	arg(Level::name(xtc.src.level())).
	arg(inet_ntoa(inaddr)).
	arg(info.processId()).
	arg(QString::number(xtc.damage.value(),16));
    }
  }


  if (dg.datagram().xtc.damage.value() & (1<<Pds::Damage::UserDefined)) {
    msg += QString("\n  Need to restart DAQ");
    emit transition_failed(msg,true);
  }
  else
    emit transition_failed(msg,false);
}

void MainWindow::platform_error()
{
  /*
  **  No automatic method for restarting the source level
  **
  _log->appendText("Platform failed.\n  Restarting \"source\" application.\n");
  system("restart_source");
  _log->appendText("Attaching ...\n");
  //  _control->detach();
  _control->attach();
  */
  QString msg("Partition failed.  Need to restart \"source\" application.");
  _log->appendText(msg);
  QMessageBox::critical(this, "Platform Error", msg);
}

void MainWindow::handle_failed_transition(const QString& msg, bool critical)
{
  QString t = QString("%1: %2")
    .arg(QTime::currentTime().toString("hh:mm:ss"))
    .arg(msg);
  printf("%s\n",qPrintable(t));
  _log->appendText(t);

  if (critical)
    QMessageBox::critical(this, "Transition Failed", msg);
}


//
// In the slot functions connected to the QSocket::activated signals,
// read the byte.  Now safely back in Qt with signal, and can do all
// the Qt stuff not allowed to do in the Unix signal handler.
//
void MainWindow::handle_sigterm()
{
    snTerm->setEnabled(false);
    char tmp;
    ::read(sigtermFd[1], &tmp, sizeof(tmp));

    printf("SIGTERM received.  Closing all windows.\n");

    // do Qt stuff
    QApplication::closeAllWindows();

    snTerm->setEnabled(true);
}

void MainWindow::handle_sigint()
{
    snInt->setEnabled(false);
    char tmp;
    ::read(sigintFd[1], &tmp, sizeof(tmp));

    printf("SIGINT received.  Closing all windows.\n");

    // do Qt stuff
    QApplication::closeAllWindows();

    snInt->setEnabled(true);
}

//
// setup_unix_signal_handlers -
//
// RETURNS: 0 on success, non-0 on error.
//
static int setup_unix_signal_handlers()
{
    struct sigaction int_action, term_action;

    int_action.sa_handler = MainWindow::intSignalHandler;
    sigemptyset(&int_action.sa_mask);
    int_action.sa_flags = 0;
    int_action.sa_flags |= SA_RESTART;

    if (sigaction(SIGINT, &int_action, 0) > 0) {
        return 1;
    }

    term_action.sa_handler = MainWindow::termSignalHandler;
    sigemptyset(&term_action.sa_mask);
    term_action.sa_flags = 0;
    term_action.sa_flags |= SA_RESTART;

    if (sigaction(SIGTERM, &term_action, 0) > 0) {
        return 2;
    }

    return 0;
}

//
// In Unix signal handlers, write a byte to the write end of a socket
// pair and return.  This will cause the corresponding QSocketNotifier
// to emit its activated() signal, which will in turn cause the appropriate
// Qt slot function to run.
//
void MainWindow::intSignalHandler(int)
{
    char a = 1;
    ::write(sigintFd[0], &a, sizeof(a));
}

void MainWindow::termSignalHandler(int)
{
    char a = 1;
    ::write(sigtermFd[0], &a, sizeof(a));
}
