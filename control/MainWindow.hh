#ifndef Pds_MainWindow_hh
#define Pds_MainWindow_hh

#include <QtGui/QWidget>

namespace Pds {
  class CCallback;
  class CfgClientNfs;
  class QualifiedControl;
  class PVManager;

  class MainWindow : public QWidget {
    Q_OBJECT
  public:
    MainWindow(unsigned          platform,
	       const char*       partition,
	       const char*       dbpath);
    ~MainWindow();
  private:
    friend class ControlTimeout;
    CCallback*        _controlcb;
    QualifiedControl* _control;
    CfgClientNfs*     _config;
    PVManager*        _pvmanager;
  };
};

#endif
