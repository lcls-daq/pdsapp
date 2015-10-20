#ifndef Pds_VmonReaderTreeMenu_hh
#define Pds_VmonReaderTreeMenu_hh

#include <QtGui/QGroupBox>
#include "pds/mon/MonConsumerClient.hh"
#include "pds/vmon/VmonReader.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <list>
#include <map>
using std::list;

class QAbstractButton;
class QLineEdit;
class QButtonGroup;
class QGroupBox;
class QPushButton;
class QLabel;
class QComboBox;
class QSlider;

namespace Pds {

  class VmonReader;
  class MonTabs;

  class VmonReaderTreeMenu : public QGroupBox,
			     public VmonReaderCallback {
    Q_OBJECT
  public:
    VmonReaderTreeMenu(QWidget& p, 
		       MonTabs& tabs,
		       const char* path);
    virtual ~VmonReaderTreeMenu();

  signals:
  public slots:
    void set_tree(QAbstractButton*);

    void set_start_time(int);
    void set_stop_time (int);
    void update_times();

    void browse();
    void preface();
    void execute();
    void select(int);
  private:
    void add(const MonCds&);
    void clear();
  public:
    void process(const ClockTime& t,
		 const Src&       src,
		 int,
		 const MonStatsScalar& stats);
    void process(const ClockTime& t,
		 const Src&       src,
		 int,
		 const MonStats1D& stats);
    void process(const ClockTime& t,
		 const Src&       src,
		 int,
		 const MonStats2D& stats);
    void end_record();
  private:
    MonTabs&      _tabs;
    const char*   _path;

    QPushButton*  _execB;

    QGroupBox*    _client_bg_box;
    QButtonGroup* _client_bg;

    QComboBox*    _recent;
    QLabel*       _start_label;
    QSlider*      _start_slider;
    ClockTime     _start_time;

    QLabel*       _stop_label;
    QSlider*      _stop_slider;
    ClockTime     _stop_time;

    VmonReader*   _reader;
    MonCds*       _cds;
  };
};

#endif
