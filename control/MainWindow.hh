#ifndef Pds_MainWindow_hh
#define Pds_MainWindow_hh

#include <QtGui/QWidget>

namespace Pds {
  class CCallback;
  class CfgClientNfs;
  class PartitionControl;

  class MainWindow : public QWidget {
    Q_OBJECT
  public:
    MainWindow(unsigned          platform,
	       const char*       partition,
	       const char*       dbpath);
    ~MainWindow();
  private:
    CCallback*        _controlcb;
    PartitionControl* _control;
    CfgClientNfs*     _config;
  };
};

#endif
