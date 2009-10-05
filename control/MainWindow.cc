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
#include "pds/client/DamageBrowser.hh"
#include "pds/client/Decoder.hh"

#include <QtGui/QApplication>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMessageBox>

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
  public:
    FileReport(ControlLog& log) : _log(log) {}
    ~FileReport() {}
  public:
    Transition* transitions(Transition* tr) { return tr; }
    InDatagram* events     (InDatagram* dg) 
    { 
      if (dg->datagram().seq.service()==TransitionId::BeginRun) {
	char fname[256];
	char dtime[64];
	time_t tm = dg->datagram().seq.clock().seconds();
	strftime(dtime,64,"%Y%m%d-%H%M%S",gmtime(&tm));
	sprintf(fname,"%s-0.xtc",dtime);
	_log.append(QString("Data will be written to %1\n").arg(fname));
      }
      return dg;
    }
  private:
    ControlLog& _log;
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
  QObject::connect(this  , SIGNAL(transition_failed(const QString&))   , 
		   this  , SLOT(handle_failed_transition(const QString&)));
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
}

ControlLog& MainWindow::log() { return *_log; }

void MainWindow::controleb_tmo()
{
  Allocation alloc = _control->eb().remaining();
  // kludge: weaver  we get some timeouts even though the transition is complete
  if (alloc.nnodes()) {
    QString msg = QString("Timeout waiting for transition %1 -> %2 to complete.\n")
      .arg(_control->current_state())
      .arg(_control->target_state ());
    msg += QString("Failing nodes:\n");
    for(unsigned k=0; k<alloc.nnodes(); k++) {
      NodeSelect s(*alloc.node(k));
      msg += s.label() + QString("\n");
    }
    msg += QString("Need to restart.\n");
    
    emit transition_failed(msg);
  }
}

void MainWindow::transition_damaged(const InDatagram& dg)
{
  DamageBrowser b(dg);
  const std::list<Xtc>& damaged = b.damaged();

  QString msg = QString("%1 damaged:").arg(TransitionId::name(dg.datagram().seq.service()));
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

  emit transition_failed(msg);
}

void MainWindow::platform_error()
{
  /*
  **  No automatic method for restarting the source level
  **
  _log->append("Platform failed.\n  Restarting \"source\" application.\n");
  system("restart_source");
  _log->append("Attaching ...\n");
  //  _control->detach();
  _control->attach();
  */
  QString msg("Partition failed.  Need to restart \"source\" application.");
  _log->append(msg);
  QMessageBox::critical(this, "Platform Error", msg);
}

void MainWindow::handle_failed_transition(const QString& msg)
{
  printf("%s\n",qPrintable(msg));
  _log->append(msg);
//   QMessageBox::critical(this, "Transition Failed", msg);
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
