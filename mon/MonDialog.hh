#ifndef Pds_MonDialog_hh
#define Pds_MonDialog_hh

#include "MonQtBase.hh"

#include <QtGui/QGroupBox>
#include <QtGui/QVBoxLayout>

class QCheckBox;

namespace Pds {

  class MonQtFloatEntry;
  class MonQtTH1F;
  class MonQtTH2F;
  class MonQtImage;
  class MonQtProf;
  class MonQtChart;
  class MonDialogEntry;
  class MonCanvas;

  class QVWidget : public QGroupBox {
  public:
    QVWidget(QWidget* p,
	     const char* name) :
      QGroupBox(name,p)
    {
      setLayout(new QVBoxLayout(this)); 
      layout()->setContentsMargins(0,0,0,0);
    }
  };

  class MonDialogAxis : public QWidget {
    Q_OBJECT
  public:
    enum Options { All=0, NoLog=1, NoMin=2, NoMax=4, NoAuto=8 };

    MonDialogAxis(QWidget* p, 
		  MonQtBase::Axis ax, 
		  const MonQtBase* hist,
		  unsigned);
    virtual ~MonDialogAxis() {}
  public:
    void applied(MonQtBase&, MonQtBase::Axis);
  public slots:
    void validate_log(int);
  private:
    MonQtFloatEntry* _min_entry;
    MonQtFloatEntry* _max_entry;
    QCheckBox* _autorng_bt;
    QCheckBox* _log_bt;
  };

  class MonDialog : public QVWidget {
    Q_OBJECT
  public:
    MonDialog(MonCanvas* canvas, 
	      MonQtTH1F* hist,
	      MonQtTH1F* since,
	      MonQtTH1F* diff,
	      MonQtChart* chart);
    MonDialog(MonCanvas* canvas, 
	      MonQtProf* hist,
	      MonQtProf* since,
	      MonQtProf* diff,
	      MonQtChart* chart);
    MonDialog(MonCanvas* canvas, 
	      MonQtTH2F* hist,
	      MonQtTH2F* diff,
	      MonQtTH1F* histx,
	      MonQtTH1F* histy,
	      MonQtTH1F* diffx,
	      MonQtTH1F* diffy,
	      MonQtChart* chartx,
	      MonQtChart* charty);
    MonDialog(MonCanvas* canvas, 
	      MonQtImage* hist,
	      MonQtImage* since,
	      MonQtImage* diff,
	      MonQtTH1F* histx,
	      MonQtTH1F* histy,
	      MonQtTH1F* diffx,
	      MonQtTH1F* diffy,
	      MonQtChart* chartx,
	      MonQtChart* charty);
    ~MonDialog();

  private:
    void addbuttons(QWidget* main);

  public slots:
    void apply();
    void close();

  private:
    MonCanvas* _canvas;
    unsigned _nentries;
    enum {MaxEntries=10};
    MonDialogEntry* _entries[MaxEntries];
  };
};
#endif
