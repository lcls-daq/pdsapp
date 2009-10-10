#ifndef Pds_MainWindow_hh
#define Pds_MainWindow_hh

#include <QtGui/QWidget>

// signal support
#include <QtCore/qsocketnotifier.h>
#include "signal.h"
#include "pds/offlineclient/OfflineClient.hh"

namespace Pds {
  class CCallback;
  class ControlLog;
  class CfgClientNfs;
  class QualifiedControl;
  class PVManager;
  class InDatagram;
  class RunAllocator;

  class MainWindow : public QWidget {
    Q_OBJECT
  public:
    MainWindow(unsigned          platform,
	       const char*       partition,
	       const char*       dbpath,
	       const char*       offlinerc,
	       const char*       experiment);
    ~MainWindow();

    // Unix signal handlers.
    static void intSignalHandler(int unused);
    static void termSignalHandler(int unused);

  signals:
    void transition_failed(const QString&);
    //    void platform_failed();
  public slots:
    void handle_failed_transition(const QString&);
    void handle_sigint();
    void handle_sigterm();
    //    void handle_platform_error();
  public:
    ControlLog& log();
    void controleb_tmo();
    void transition_damaged(const InDatagram&);
    void platform_error();
  private:
    friend class ControlTimeout;
    CCallback*        _controlcb;
    QualifiedControl* _control;
    CfgClientNfs*     _config;
    PVManager*        _pvmanager;
    ControlLog*       _log;
    OfflineClient*    _offlineclient;
    RunAllocator*     _runallocator;

    // signal handler support
    QSocketNotifier *snInt;
    QSocketNotifier *snTerm;
  };
};

#endif
