#ifndef Pds_MainWindow_hh
#define Pds_MainWindow_hh

#include "pds/offlineclient/OfflineClient.hh"
#include <QtGui/QWidget>

// signal support
#include <QtCore/qsocketnotifier.h>
#include "signal.h"

namespace Pds {
  class CCallback;
  class ControlLog;
  class CfgClientNfs;
  class IocControl;
  class QualifiedControl;
  class PVManager;
  class ExportStatus;
  class InDatagram;
  class RunAllocator;
  class PartitionSelect;
  class UserMessage;

  class MainWindow : public QWidget {
    Q_OBJECT
  public:
    MainWindow(unsigned          platform,
               const char*       partition,
               const char*       dbpath,
               const char*       offlinerc,
               const char*       runNumberFile,
               const char*       experiment,
               unsigned          sequencer_id,
               int               slowReadout,
               unsigned          partition_options,
               bool              verbose,
               const char*       controlrc,
               const char*       status_host_and_port,
               unsigned          pv_ignore_options);
    ~MainWindow();

    // Unix signal handlers.
    static void intSignalHandler(int unused);
    static void termSignalHandler(int unused);

  signals:
    void message_received(const QString&,bool);
    void override_received(const QString&);
    void auto_run_started();
    //    void platform_failed();
  public slots:
    void handle_message(const QString&,bool);
    void handle_sigint();
    void handle_sigterm();
    void handle_override(const QString&);
    //    void handle_platform_error();
  public:
    ControlLog& log();
    void controleb_tmo();
    void transition_damaged(const InDatagram&);
    void insert_message(const char*);
    void platform_error();
    void require_shutdown();
    void override_errors(bool);
    void autorun();
    void attached();
  private:
    friend class ControlTimeout;
    CCallback*        _controlcb;
    QualifiedControl* _control;
    IocControl*       _icontrol;
    CfgClientNfs*     _config;
    PVManager*        _pvmanager;
    ExportStatus*     _exportstatus;
    const char*       _status_host_and_port;
    ControlLog*       _log;
    OfflineClient*    _offlineclient;
    RunAllocator*     _runallocator;
    PartitionSelect*  _partition;

    // signal handler support
    QSocketNotifier *snInt;
    QSocketNotifier *snTerm;

    bool _override_errors;
  };
};

#endif
