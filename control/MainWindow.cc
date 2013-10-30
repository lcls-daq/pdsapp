#include "MainWindow.hh"

#include "pdsapp/control/ConfigSelect.hh"
#include "pdsapp/control/PartitionSelect.hh"
#include "pdsapp/control/NodeSelect.hh"
#include "pdsapp/control/StateSelect.hh"
#include "pdsapp/control/RemoteSeqApp.hh"
#include "pdsapp/control/SeqAppliance.hh"
#include "pdsapp/control/PVDisplay.hh"
#include "pdsapp/control/PVManager.hh"
#include "pdsapp/control/RunStatus.hh"
#include "pdsapp/control/ControlLog.hh"
#include "pdsapp/control/MySqlRunAllocator.hh"
#include "pdsapp/control/FileRunAllocator.hh"

#include "pds/offlineclient/OfflineClient.hh"
#include "pds/ioc/IocControl.hh"
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
#include <string.h>

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
    { if (dg->datagram().xtc.damage.value() && 
          dg->datagram().seq.service()!=TransitionId::L1Accept)
        _w.transition_damaged(*dg);
      return dg;
    }
    Occurrence* occurrences(Occurrence* occ)
    { if (occ->id()==OccurrenceId::UserMessage)
  _w.insert_message(static_cast<UserMessage*>(occ)->msg());
      else if (occ->id()==OccurrenceId::ClearReadout) {
        _w.insert_message("Detector out-of-order.\n  Shutting down.\n  Allocate/Run to continue");
        _w.require_shutdown();
      } else if (occ->id() == OccurrenceId::DataFileError) {
        char msg[256];
        const DataFileError& dfe = *static_cast<const DataFileError*>(occ);
        snprintf(msg, sizeof(msg), "Error writing e%d-r%04d-s%02d-c%02d.xtc.\n"
                "  Shutting down.\n  Fix and Allocate again.",
                dfe.expt, dfe.run, dfe.stream, dfe.chunk);
        _w.insert_message(msg);
        _w.require_shutdown();
      }
      return occ;
    }
  private:
    MainWindow& _w;
  };

  class FileReport : public Appliance {
  public:
    FileReport(ControlLog& log) :
      _log(log),
      _experiment(0)
    {
    }
    ~FileReport() {}

  public:
    Transition* transitions(Transition* tr)
    {
      if (tr->id()==TransitionId::BeginRun) {
  if (tr->size() == sizeof(Transition)) {  // No RunInfo
    char fname[256];
    sprintf(fname, "e%d/e%d-r%04d-sNN-cNN.xtc",
      0, 0, tr->env().value());
    _log.appendText(QString("%1: Not recording. Transient data file: %2\n")
        .arg(QTime::currentTime().toString("hh:mm:ss"))
        .arg(fname));
  }
  else {
          RunInfo& rinfo = *reinterpret_cast<RunInfo*>(tr);
    char fname[256];
    sprintf(fname, "e%d/e%d-r%04d-s00-c00.xtc",
      rinfo.experiment(), rinfo.experiment(), rinfo.run());
    _experiment = rinfo.experiment();
    _log.appendText(QString("%1: Recording run %2. Transient data file: %3\n")
        .arg(QTime::currentTime().toString("hh:mm:ss"))
        .arg(rinfo.run())
        .arg(fname));
  }
      }
    return tr;
    }

    InDatagram* events     (InDatagram* dg) { return dg; }

    Occurrence* occurrences(Occurrence* occ)
    {
      if (occ->id() == OccurrenceId::DataFileOpened) {
        char fname[256];
        const DataFileOpened& dfo = *static_cast<const DataFileOpened*>(occ);
        snprintf(fname, sizeof(fname), "%s:%s", dfo.host, dfo.path);
                _log.appendText(QString("%1: Opened data file %2")
                .arg(QTime::currentTime().toString("hh:mm:ss"))
                .arg(fname));
      }
      return occ;
    }

  private:
    ControlLog& _log;
    unsigned    _experiment;
  };

  class OfflineReport : public Appliance {
  public:
    OfflineReport(PartitionSelect& partition, RunAllocator& runallocator) :
      _partition(partition),
      _runallocator(runallocator),
      _experiment(0),
      _run(0)
    {
    }
    ~OfflineReport() {}

  public:
    Transition* transitions(Transition* tr)
    {
      if ((tr->id()==TransitionId::BeginRun) &&
          (tr->size() > sizeof(Transition))) {
        // RunInfo
        RunInfo& rinfo = *reinterpret_cast<RunInfo*>(tr);
        _experiment = rinfo.experiment();
        _run = rinfo.run();
        std::vector<std::string> names;
        foreach (std::string ss, _partition.deviceNames()) {
          names.push_back(ss);
        }
        _runallocator.reportDetectors(_experiment, _run, names);
      }
    return tr;
    }

    InDatagram* events     (InDatagram* dg) { return dg; }

    Occurrence* occurrences(Occurrence* occ) { return occ; }

  private:
    PartitionSelect& _partition;
    RunAllocator& _runallocator;
    unsigned    _experiment;
    unsigned    _run;
  };

  class ShutdownTest : public Appliance {
  public:
    ShutdownTest(QualifiedControl& c) : _c(c) {}
    ~ShutdownTest() {}
  public:
    Transition* transitions(Transition* tr)
    { if (tr->id()==TransitionId::Unmap)
        _c.enable(PartitionControl::Mapped,true);
      return tr; }
    InDatagram* events     (InDatagram* dg) { return dg; }
  private:
    QualifiedControl& _c;
  };

};

using namespace Pds;
using Pds_ConfigDb::Experiment;

MainWindow::MainWindow(unsigned          platform,
                       const char*       partition,
                       const char*       db_path,
                       const char*       offlinerc,
                       const char*       runNumberFile,
                       const char*       experiment_name,
                       unsigned          sequencer_id,
                       int               slowReadout,
                       unsigned          partition_options,
                       bool              verbose) :
  QWidget(0),
  _controlcb(new CCallback(*this)),
  _control  (new QualifiedControl(platform, *_controlcb, slowReadout, new ControlTimeout(*this))),
  _icontrol (new IocControl),
  _config   (new CfgClientNfs(Node(Level::Control,platform).procInfo())),
  _override_errors(false)
{
  setAttribute(Qt::WA_DeleteOnClose, true);

  ConfigSelect*     config;
  StateSelect*      state ;
  PVDisplay*        pvs;
  RunStatus*        run;
  unsigned int      experiment_number = 0;

  if (offlinerc) {
    // option A: run number maintained in a mysql database
    PartitionDescriptor pd(partition);
    if (pd.valid()) {
      if (experiment_name) {
        // A.1: experiment name passed in
        _offlineclient = new OfflineClient(offlinerc, pd, experiment_name, verbose);
        experiment_number = _offlineclient->GetExperimentNumber();
        if (experiment_number == OFFLINECLIENT_DEFAULT_EXPNUM) {
          fprintf(stderr, "%s: failed to find experiment '%s'\n", __FUNCTION__,
                  experiment_name);
        }
      } else {
        // A.2: current experiment retrieved from database
        _offlineclient = new OfflineClient(offlinerc, pd, verbose);
        experiment_number = _offlineclient->GetExperimentNumber();
        if (experiment_number == OFFLINECLIENT_DEFAULT_EXPNUM) {
          fprintf(stderr, "%s: failed to find current experiment for partition '%s'\n",
                  __FUNCTION__, partition);
        }
      }
      if (experiment_number != OFFLINECLIENT_DEFAULT_EXPNUM) {
        // success: run number allocated from database
        const char *expname = _offlineclient->GetExperimentName();
        const char *instname = _offlineclient->GetInstrumentName();
        unsigned station     = _offlineclient->GetStationNumber();
        printf("%s: instrument '%s:%u' experiment '%s' (#%u)\n", __FUNCTION__,
               instname, station, expname, experiment_number);
        _runallocator = new MySqlRunAllocator(_offlineclient);
      } else {
        // error: run number fixed at 0
        _runallocator = new RunAllocator;
        // NULL offline database
        _offlineclient = (OfflineClient*)NULL;
      }
    } else {
      fprintf(stderr, "%s: partition '%s' is not valid\n",
              __FUNCTION__, partition);
    }
    _control->set_experiment(experiment_number);
  } else if (runNumberFile) {
    // option B: run number maintained in a simple file
    _runallocator = new FileRunAllocator(runNumberFile);
    // NULL offline database
    _offlineclient = (OfflineClient*)NULL;
  } else {
    // option C: run number fixed at 0
    _runallocator = new RunAllocator;
    // NULL offline database
    _offlineclient = (OfflineClient*)NULL;
  }
  _control->set_runAllocator(_runallocator);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(config    = new ConfigSelect   (this, *_control, db_path));
  layout->addWidget(_partition= new PartitionSelect(this, *_control, *_icontrol, partition, db_path, partition_options));
  layout->addWidget(state     = new StateSelect    (this, *_control));
  layout->addWidget(pvs       = new PVDisplay      (this, *_control));
  layout->addWidget(run       = new RunStatus      (this, *_control, *_partition));
  layout->addWidget(_log      = new ControlLog, 1);

  _pvmanager = new PVManager(*pvs);

  //  the order matters
  _controlcb->add_appliance(run);    // must be first
  //  _controlcb->add_appliance(new Decoder(Level::Control));
  _controlcb->add_appliance(new ShutdownTest(*_control));
  _controlcb->add_appliance(_icontrol);
  _controlcb->add_appliance(new ControlDamage(*this));
  _controlcb->add_appliance(new FileReport(*_log));
  if (_offlineclient) {
    _controlcb->add_appliance(new OfflineReport(*_partition, *_runallocator));
  }
  _controlcb->add_appliance(state);
  _controlcb->add_appliance(new SeqAppliance(*_control, *state, *config, *_config,
					     *_pvmanager, sequencer_id));
  _controlcb->add_appliance(new RemoteSeqApp(*_control, *state, *config, *_pvmanager,
					     _config->src(), *run, *_partition));
  _control->attach();

  QObject::connect(state , SIGNAL(configured(bool)), config, SLOT(configured(bool)));
  QObject::connect(state , SIGNAL(state_changed(QString)), _partition, SLOT(change_state(QString)));
  QObject::connect(this  , SIGNAL(message_received(const QString&, bool))   ,
       this  , SLOT(handle_message(const QString&, bool)));
  QObject::connect(config, SIGNAL(assert_message(const QString&, bool))   ,
       this  , SLOT(handle_message(const QString&, bool)));
  QObject::connect(this  , SIGNAL(override_received(const QString&)),
       this  , SLOT(handle_override(const QString&)));
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
  if (_offlineclient) {
    delete _offlineclient;
  }
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

    emit message_received(msg,true);
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
    else if (xtc.src.level()==Level::Segment) {
      const ProcInfo& info = static_cast<const ProcInfo&>(xtc.src);
      DetInfo dinfo(-1,DetInfo::NoDetector,0,DetInfo::NoDevice,0);
      for(int i=0; i<_partition->segments().size();i++) {
  if (_partition->segments().at(i)==info) {
    dinfo = _partition->detectors().at(i);
    break;
  }
      }
      struct in_addr inaddr;
      inaddr.s_addr = ntohl(info.ipAddr());
      msg += QString("\n  %1 [%2 : %3] : 0x%4").
  arg(DetInfo::name(dinfo)).
  arg(inet_ntoa(inaddr)).
  arg(info.processId()).
  arg(QString::number(xtc.damage.value(),16));
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
    if (_override_errors) {
      emit override_received(msg);
    }
    else {
      msg += QString("\n  Shutting down.\n  Fix and Allocate again.");
      emit message_received(msg,true);
      require_shutdown();
    }
  }
  else
    emit message_received(msg,false);
}

void MainWindow::insert_message(const char* msg)
{
  QString m(msg);
  emit message_received(m,true);
}

void MainWindow::platform_error()
{
  QString msg("Partition failed.  Need to restart.");
  _log->appendText(msg);
  QMessageBox::critical(this, "Platform Error", msg);
  close();
}

void MainWindow::handle_message(const QString& msg, bool critical)
{
  QString t = QString("%1: %2")
    .arg(QTime::currentTime().toString("hh:mm:ss"))
    .arg(msg);
  printf("%s\n",qPrintable(t));
  _log->appendText(t);

  if (critical)
    QMessageBox::critical(this, "DAQ Control Error", msg);
}

void MainWindow::require_shutdown()
{
  //  _control->enable(PartitionControl::Mapped,false);
  //
  //  Perform a shutdown
  //
  _control->set_target_state(PartitionControl::Unmapped);
}

void MainWindow::handle_override(const QString& msg)
{
  QString t = QString("%1: %2")
    .arg(QTime::currentTime().toString("hh:mm:ss"))
    .arg(msg);
  printf("%s\n",qPrintable(t));
  _log->appendText(t);

  if (QMessageBox::critical(this, "DAQ Control Error", msg, QMessageBox::Abort | QMessageBox::Ignore) == QMessageBox::Abort)
    require_shutdown();
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

void MainWindow::override_errors(bool l)
{
  _override_errors = l;
}
