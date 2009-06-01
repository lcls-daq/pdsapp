#include <stdlib.h>

#include "MonDialog.hh"
#include "MonCanvas.hh"
//#include "MonLayoutHints.hh"
#include "MonQtTH1F.hh"
#include "MonQtTH2F.hh"
#include "MonQtImage.hh"
#include "MonQtProf.hh"
#include "MonQtChart.hh"
#include "MonQtEntry.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

namespace Pds {

#define addAxis( a, opt ) \
  layout()->addWidget(_axis[MonQtBase::a] = new MonDialogAxis(this, MonQtBase::a, hist, opt))

  class MonDialogEntry : public QVWidget {
  public:
    MonDialogEntry(QWidget* p, 
		   const char* name, 
		   MonQtTH1F* hist) :
      QVWidget(p,name),
      _hist(hist)
    {
      addAxis( X, MonDialogAxis::NoLog );
      addAxis( Y, MonDialogAxis::All );
      _axis[MonQtBase::Z] = 0;
    }

    MonDialogEntry(QWidget* p, 
		   const char* name, 
		   MonQtProf* hist) :
      QVWidget(p,name),
      _hist(hist)
    {
      addAxis( X, MonDialogAxis::NoLog );
      addAxis( Y, MonDialogAxis::All );
      _axis[MonQtBase::Z] = 0;
    }

    MonDialogEntry(QWidget* p, 
		   const char* name, 
		   MonQtTH2F* hist) :
      QVWidget(p,name),
      _hist(hist)
    {
      addAxis( X, MonDialogAxis::NoLog );
      addAxis( Y, MonDialogAxis::NoLog );
      addAxis( Z, MonDialogAxis::All );
    }

    MonDialogEntry(QWidget* p, 
		   const char* name, 
		   MonQtImage* hist) :
      QVWidget(p,name),
      _hist(hist)
    {
      _axis[MonQtBase::X] = 0;
      _axis[MonQtBase::Y] = 0;
      addAxis( Z, MonDialogAxis::NoMin );
    }

    MonDialogEntry(QWidget* p, 
		   const char* name, 
		   MonQtChart* hist) :
      QVWidget(p,name),
      _hist(hist)
    {
      addAxis( X, (MonDialogAxis::NoLog | MonDialogAxis::NoMin | MonDialogAxis::NoAuto) );
      addAxis( Y, MonDialogAxis::All );
      _axis[MonQtBase::Z] = 0;
    }

    virtual ~MonDialogEntry() {}

    void apply() 
    {
      for (unsigned i=0; i<3; i++) {
	if (_axis[i])
	  _axis[i]->applied(*_hist, MonQtBase::Axis(i));
      }
      _hist->apply();
    }

  private:
    MonQtBase* _hist;
    MonDialogAxis* _axis[3];
  };
};

using namespace Pds;

static const char* dialogAxisLabel[] = { "X Axis", "Y Axis", "Z Axis" };

//
//  MonDialogAxis
//
MonDialogAxis::MonDialogAxis(QWidget* p, 
			     MonQtBase::Axis ax, 
			     const MonQtBase* hist,
			     unsigned opt) :
  QWidget    (p),
  _min_entry (0),
  _max_entry (0),
  _autorng_bt(0),
  _log_bt    (0)
{
  bool useLog = !(opt & NoLog);
  bool useMin = !(opt & NoMin);
  bool useMax = !(opt & NoMax);
  bool useAuto= !(opt & NoAuto);
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->addWidget(new QLabel(dialogAxisLabel[ax],this));
  if (useMin) layout->addWidget(_min_entry = new MonQtFloatEntry("Minimum",hist->min(ax),this));
  else        layout->addStretch();
  if (useMax) layout->addWidget(_max_entry = new MonQtFloatEntry("Maximum",hist->max(ax),this));
  else        layout->addStretch();
  if (useAuto) { layout->addWidget(_autorng_bt = new QCheckBox("Auto Range",this));
                 _autorng_bt->setChecked(hist->isautorng(ax)); }
  else        layout->addStretch();
  if (useLog)  { layout->addWidget(_log_bt = new QCheckBox("Log scale",this));
                  connect(_log_bt, SIGNAL(stateChanged(int)), this, SLOT(validate_log(int))); }
  else        layout->addStretch();

  setLayout(layout);
  
}

void MonDialogAxis::applied(MonQtBase& hist, MonQtBase::Axis ax)
{
  hist.settings(ax,
		_min_entry ? _min_entry->entry() : 0,
		_max_entry ? _max_entry->entry() : 0,
		(!_autorng_bt || _autorng_bt->checkState() == Qt::Checked),
		(_log_bt && _log_bt->checkState()==Qt::Checked));
}

void MonDialogAxis::validate_log(int state) 
{
  if (state==Qt::Checked) {
    if (_min_entry && _min_entry->entry() <= 0)
      _min_entry->setEntry(0.1);
    if (_max_entry && _max_entry->entry() <= 0)
      _max_entry->setEntry(10.);
  } 
}

//
//  MonDialog
//
MonDialog::MonDialog(MonCanvas* canvas, 
		     MonQtTH1F* hist,
		     MonQtTH1F* since,
		     MonQtTH1F* diff,
		     MonQtChart* chart) :
  QVWidget((QWidget*)0, "1D Histogram Dialog"),
  _canvas(canvas),
  _nentries(0)
{
  _entries[_nentries++] = new MonDialogEntry(this, "Integrated", hist);
  _entries[_nentries++] = new MonDialogEntry(this, "Since",      since);
  _entries[_nentries++] = new MonDialogEntry(this, "Difference", diff);
  _entries[_nentries++] = new MonDialogEntry(this, "Chart", chart);

  addbuttons(canvas);
}

MonDialog::MonDialog(MonCanvas* canvas, 
		     MonQtProf* hist,
		     MonQtProf* since,
		     MonQtProf* diff,
		     MonQtChart* chart) :
  QVWidget((QWidget*)0, "Profile Histogram Dialog"),
  _canvas(canvas),
  _nentries(0)
{
  _entries[_nentries++] = new MonDialogEntry(this, "Integrated", hist);
  _entries[_nentries++] = new MonDialogEntry(this, "Since",      since);
  _entries[_nentries++] = new MonDialogEntry(this, "Difference", diff);
  _entries[_nentries++] = new MonDialogEntry(this, "Chart", chart);

  addbuttons(canvas);
}

MonDialog::MonDialog(MonCanvas* canvas, 
		     MonQtTH2F* hist,
		     MonQtTH2F* diff,
		     MonQtTH1F* histx,
		     MonQtTH1F* histy,
		     MonQtTH1F* diffx,
		     MonQtTH1F* diffy,
		     MonQtChart* chartx,
		     MonQtChart* charty) :
  QVWidget((QWidget*)0, "2D Histogram Dialog"),
  _canvas(canvas),
  _nentries(0)
{
  _entries[_nentries++] = new MonDialogEntry(this, "Integrated", hist);
  _entries[_nentries++] = new MonDialogEntry(this, "Difference", diff);
  _entries[_nentries++] = new MonDialogEntry(this, "Integrated X", histx);
  _entries[_nentries++] = new MonDialogEntry(this, "Integrated Y", histy);
  _entries[_nentries++] = new MonDialogEntry(this, "Difference X", diffx);
  _entries[_nentries++] = new MonDialogEntry(this, "Difference Y", diffy);
  _entries[_nentries++] = new MonDialogEntry(this, "Chart X", chartx);
  _entries[_nentries++] = new MonDialogEntry(this, "Chart Y", charty);

  addbuttons(canvas);
}

MonDialog::MonDialog(MonCanvas* canvas, 
		     MonQtImage* hist,
		     MonQtImage* since,
		     MonQtImage* diff,
		     MonQtTH1F* histx,
		     MonQtTH1F* histy,
		     MonQtTH1F* diffx,
		     MonQtTH1F* diffy,
		     MonQtChart* chartx,
		     MonQtChart* charty) :
  QVWidget((QWidget*)0, "Image Dialog"),
  _canvas(canvas),
  _nentries(0)
{
  _entries[_nentries++] = new MonDialogEntry(this, "Integrated", hist);
  _entries[_nentries++] = new MonDialogEntry(this, "Since"     , since);
  _entries[_nentries++] = new MonDialogEntry(this, "Difference", diff);
  _entries[_nentries++] = new MonDialogEntry(this, "Integrated X", histx);
  _entries[_nentries++] = new MonDialogEntry(this, "Integrated Y", histy);
  _entries[_nentries++] = new MonDialogEntry(this, "Difference X", diffx);
  _entries[_nentries++] = new MonDialogEntry(this, "Difference Y", diffy);
  _entries[_nentries++] = new MonDialogEntry(this, "Chart X", chartx);
  _entries[_nentries++] = new MonDialogEntry(this, "Chart Y", charty);

  setAttribute(Qt::WA_DeleteOnClose);
  addbuttons(canvas);
}

MonDialog::~MonDialog() 
{
}

void MonDialog::addbuttons(QWidget* main)
{
  for (unsigned e=0; e<_nentries; e++) {
    layout()->addWidget(_entries[e]);
  }

  QGroupBox* bt_box = new QGroupBox(this);
  QHBoxLayout* bt_layout = new QHBoxLayout(bt_box);
  QPushButton* apply_bt = new QPushButton("Apply",bt_box);
  QPushButton* close_bt = new QPushButton("Close",bt_box);
  bt_layout->addWidget(apply_bt);
  bt_layout->addWidget(close_bt);
  bt_box->setLayout(bt_layout);
  layout()->addWidget(bt_box);

  connect(apply_bt, SIGNAL(clicked()), this, SLOT(apply()));
  connect(close_bt, SIGNAL(clicked()), this, SLOT(close()));

  move(main->pos());
  show();
}

void MonDialog::apply()
{
  for (unsigned e=0; e<_nentries; e++) {
    _entries[e]->apply();
  }
  _canvas->update();
}

void MonDialog::close()
{
  apply();
  QWidget::close();
}
