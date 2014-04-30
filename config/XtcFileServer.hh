#ifndef Pds_ConfigDb_XtcFileServer_hh
#define Pds_ConfigDb_XtcFileServer_hh

#include <QtGui/QGroupBox>

#include "pdsdata/xtc/Dgram.hh"

#include <errno.h>

using namespace Pds;
using namespace std;

class QComboBox;
class QLabel;
class QPushButton;

namespace Pds_ConfigDb {
  class XtcFileServer : public QGroupBox {
    Q_OBJECT
  public:
    XtcFileServer(const char* curdir);
    ~XtcFileServer();
  public slots:
    void set_cycle(int);
  private slots:
    void selectDir();
    void selectRun(int);
    void updateDirLabel();
    void updateRunCombo();
  signals:
    void _updateDirLabel();
    void _updateRunCombo();
    void file_selected(QString);  // connect to this!
    void prev_cycle();
    void next_cycle();
  private:
    void getPathsForRun(QStringList& list, QString run);
    void setDir(QString dir);
  private:
    QString      _curdir;
    QPushButton* _dirSelect;
    QLabel*      _dirLabel;
    QComboBox*   _runCombo;
    QStringList  _runList;
    QLabel*      _cycle;
  };
}

#endif
