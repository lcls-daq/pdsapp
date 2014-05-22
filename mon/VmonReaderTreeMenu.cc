#include "VmonReaderTreeMenu.hh"
#include "MonTabs.hh"
#include "MonUtils.hh"

#include "pds/service/Task.hh"

#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonUsage.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonEntryTH2F.hh"
#include "pds/mon/MonEntryWaveform.hh"

#include "pds/utility/Transition.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QGroupBox>
#include <QtGui/QRadioButton>
#include <QtGui/QFileDialog>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtCore/QDateTime>

using namespace Pds;

static bool buttonLessThan(const QAbstractButton* b1,
                           const QAbstractButton* b2)
{
  return b1->text() < b2->text();
}

VmonReaderTreeMenu::VmonReaderTreeMenu(QWidget&        p, 
				       MonTabs& tabs,
				       const char*     path) :
  QGroupBox (&p),
  _tabs     (tabs),
  _path     (path),
  _reader   (0)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  QGroupBox* control = new QGroupBox("Control", this);
  { QVBoxLayout* clayout = new QVBoxLayout(control);
    QPushButton* browB = new QPushButton("Browse");
    QPushButton* execB = new QPushButton("Execute");
    clayout->addWidget(browB);
    clayout->addWidget(_recent = new QComboBox);
    clayout->addWidget(execB);
    clayout->addWidget(_start_slider = new QSlider(Qt::Horizontal));
    { QHBoxLayout* hlayout = new QHBoxLayout;
      hlayout->addWidget(new QLabel("Start"));
      hlayout->addStretch();
      hlayout->addWidget(_start_label = new QLabel);
      clayout->addLayout(hlayout); }
    clayout->addWidget(_stop_slider = new QSlider(Qt::Horizontal));
    { QHBoxLayout* hlayout = new QHBoxLayout;
      hlayout->addWidget(new QLabel("Stop"));
      hlayout->addStretch();
      hlayout->addWidget(_stop_label = new QLabel);
      clayout->addLayout(hlayout); }
    connect(browB,   SIGNAL(clicked()), this, SLOT(browse()));
    connect(execB,   SIGNAL(clicked()), this, SLOT(execute()));
    connect(_recent, SIGNAL(activated(int)), this, SLOT(select(int)));
    control->setLayout(clayout); 
  }
  layout->addWidget(control);
  layout->addStretch();

  _client_bg     = new QButtonGroup(this);
  _client_bg_box = new QGroupBox("Display", this);
  QRadioButton* summary_btn = new QRadioButton("Summary",0);
  summary_btn->setChecked(false);
  { QVBoxLayout* l = new QVBoxLayout;
    l->addWidget(summary_btn);
    _client_bg_box->setLayout(l); }
  layout->addWidget(_client_bg_box);
  _client_bg->addButton(summary_btn,0);

  connect(_client_bg, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(set_tree(QAbstractButton*)));
  connect(_start_slider, SIGNAL(sliderMoved(int)), this, SLOT(set_start_time(int)));
  connect(_stop_slider , SIGNAL(sliderMoved(int)), this, SLOT(set_stop_time (int)));

  setLayout(layout);
}

VmonReaderTreeMenu::~VmonReaderTreeMenu() 
{
}

void VmonReaderTreeMenu::set_tree(QAbstractButton* b)
{
  _tabs.clear();
  const QString& name = b->text();
  for(unsigned i=0; i<_reader->sources().size(); i++) {
    const Src& src = _reader->sources()[i];
    const MonCds& cds = *_reader->cds(src);
    if (QString(cds.desc().name())==name) {
      _tabs.setup(cds,0);
      return;
    }
  }
  // Summary
  for(unsigned i=0; i<_reader->sources().size(); i++) {
    const Src& src = _reader->sources()[i];
    const MonCds& cds = *_reader->cds(src);
    unsigned j=1;
    QList<QAbstractButton*> l = _client_bg->buttons();
    for(QList<QAbstractButton*>::iterator it=l.begin()+1; it!=l.end(); it++,j++)
      if ((*it)->text() == QString(cds.desc().name())) {
        _tabs.setup(cds,j);
        break;
      }
  }
}

void VmonReaderTreeMenu::add(const MonCds& cds)
{
  QRadioButton* button = new QRadioButton(cds.desc().name(),0);
  button->setChecked( false );

  //  Re-sort the buttons
  QList<QAbstractButton*> l = _client_bg->buttons();
  for(QList<QAbstractButton*>::iterator it=l.begin()+1; it!=l.end(); it++) {
    _client_bg->removeButton(*it);
    _client_bg_box->layout()->removeWidget(*it);
  }
  l.push_back(button);

  qSort(l.begin()+1,l.end(),buttonLessThan);

  unsigned i=1, n=l.size();
  MonUtils::ncolors(n+1);
  for(QList<QAbstractButton*>::iterator it=l.begin()+1; it!=l.end(); it++,i++) {
    QColor c = MonUtils::color(i);
    QPalette p;
    p.setColor(QPalette::BrightText,c);
    p.setColor(QPalette::ButtonText,c);
    p.setColor(QPalette::WindowText,c);
    (*it)->setPalette(p);
    _client_bg_box->layout()->addWidget(*it);
    _client_bg->addButton(*it);
  }

  //  MonTree* tree = new MonTree(_tabs, client, 0);
  //  _trees.push_back(tree);
  //  _map.insert(std::pair<MonClient*,MonTree*>(&client,tree));
}

void VmonReaderTreeMenu::clear()
{
  _tabs.clear();
  //  _map.clear();

  QList<QAbstractButton*> buttons = _client_bg->buttons();
  for(QList<QAbstractButton*>::iterator iter = buttons.begin()+1;
      iter != buttons.end(); iter++) {
    _client_bg->removeButton(*iter);
    _client_bg_box->layout()->removeWidget(*iter);
    delete *iter;
  }
  
//   for(list<MonTree*>::iterator iter = _trees.begin();
//       iter != _trees.end(); iter++) 
//     delete (*iter);
//   _trees.clear();

  update();
}

void VmonReaderTreeMenu::browse()
{
  QString fname = QFileDialog::getOpenFileName(this,"Vmon Archive (.dat)",
					       _path, "vmon_*.dat");
  if (!fname.isEmpty()) {
    int index = _recent->findText(fname);
    if (index >= 0) 
      select(index);
    else {
      _recent->insertItem(0,fname);
      int n = _recent->count();
      while( n > 5 )
        _recent->removeItem(--n);
      preface();
    }
  }
}

void VmonReaderTreeMenu::select(int index)
{
  QString s = _recent->itemText(index);
  if (index!=0) {
    _recent->removeItem(index);
    _recent->insertItem(0,s);
  }
  preface();
}

void VmonReaderTreeMenu::preface()
{
  QString sel;
  { QAbstractButton* b = _client_bg->checkedButton();
    if (b)
      sel = b->text();
  }

  _recent->setCurrentIndex(0);

  clear();

  if (_reader) delete _reader;

  _reader = new VmonReader(qPrintable(_recent->currentText()));
  
  const vector<Src>& sources = _reader->sources();
  unsigned i=0;
  for(vector<Src>::const_iterator it = sources.begin(); it!=sources.end(); it++,i++) {
    const MonCds& cds = *_reader->cds(*it);
    add(cds);
  }

  _start_time = _reader->begin();
  _stop_time  = _reader->end();

  unsigned dtime = _stop_time.seconds()-_start_time.seconds()-1;
  _start_slider->setRange(0,dtime); _start_slider->setValue(0);
  _stop_slider ->setRange(0,dtime); _stop_slider ->setValue(dtime);

  update_times();

  QList<QAbstractButton*> bl = _client_bg->buttons();
  for(QList<QAbstractButton*>::iterator b=bl.begin(); b!=bl.end(); b++)
    if ((*b)->text()==sel) {
      set_tree(*b);
      break;
    }
}

void VmonReaderTreeMenu::execute()
{
  const QString& name = _client_bg->checkedButton()->text();
  for(unsigned i=0; i<_reader->sources().size(); i++) {
    const Src& src = _reader->sources()[i];
    const MonCds& cds = *_reader->cds(src);
    if (QString(cds.desc().name())==name) {
      MonUsage usage;
      const Src& src = _reader->sources()[i];
      const MonCds& cds = *_reader->cds(src);
      for(int g=0; g<cds.ngroups(); g++) {
        const MonGroup* gr = cds.group(g);
        for(int k=0; k<gr->nentries(); k++)
          usage.use((g<<16)|k);
      }
      _tabs.reset(_reader->nrecords(_start_time,_stop_time));
      _reader->reset();
      _reader->use(src,usage);
      
      _reader->process(*this, _start_time, _stop_time);
      
      _tabs.update(true);
      return;
    }
  }

  _tabs.reset(_reader->nrecords(_start_time,_stop_time));
  _reader->reset();

  MonUsage* vu = new MonUsage[_reader->sources().size()];
  for(unsigned i=0; i<_reader->sources().size(); i++) {
    const Src& src = _reader->sources()[i];
    const MonCds& cds = *_reader->cds(src);
    MonUsage& usage = vu[i];
    for(int g=0; g<cds.ngroups(); g++) {
      const MonGroup* gr = cds.group(g);
      for(int k=0; k<gr->nentries(); k++)
        usage.use((g<<16)|k);
    }
    _reader->use(src,usage);
  }
  _reader->process(*this, _start_time, _stop_time);
  _tabs.update(true);
  delete[] vu;
}

void VmonReaderTreeMenu::process(const ClockTime&  t,
				 const Src&        src,
				 int               signature,
				 const MonStats1D& stats)
{
  MonCds& cds = *const_cast<MonCds*>(_reader->cds(src));
  MonEntry* entry = cds.entry(signature);
  entry->time(t);
#define SET_STATS(t)							\
  case MonDescEntry::t:							\
    { MonEntry##t* e = static_cast<MonEntry##t*>(entry);		\
      *static_cast<MonStats1D*>(e) = stats;				\
    } break;							      

  switch(entry->desc().type()) {
    SET_STATS(TH1F)
    SET_STATS(Waveform)
    default: break;
  }
#undef SET_STATS
}

void VmonReaderTreeMenu::process(const ClockTime&  t,
				 const Src&        src,
				 int               signature,
				 const MonStats2D& stats)
{
  MonCds& cds = *const_cast<MonCds*>(_reader->cds(src));
  MonEntry* entry = cds.entry(signature);
  entry->time(t);
#define SET_STATS(t)							\
  case MonDescEntry::t:							\
    *static_cast<MonStats2D*>(static_cast<MonEntry##t*>(entry)) = stats; \
  break;							      

  switch(entry->desc().type()) {
    SET_STATS(TH2F)
    default: break;
  }
#undef SET_STATS
}

void VmonReaderTreeMenu::end_record()
{
  _tabs.update(false);
}

void VmonReaderTreeMenu::set_start_time(int dt)
{
  if (_reader) {
    _start_time = ClockTime(_reader->begin().seconds()+dt,
			    _reader->begin().nanoseconds());
    if (_start_time > _stop_time) {
      _stop_time = ClockTime(_start_time.seconds()+1,
			     _reader->end().nanoseconds());
      _stop_slider->setValue(dt);
    }
    update_times();
  }
}

void VmonReaderTreeMenu::set_stop_time(int dt)
{
  if (_reader) {
    _stop_time = ClockTime(_reader->begin().seconds()+dt+1,
			   _reader->end().nanoseconds());
    if (_start_time > _stop_time) {
      _start_time = ClockTime(_stop_time.seconds()-1,
			      _reader->begin().nanoseconds());
      _start_slider->setValue(dt);
    }
    update_times();
  }
}

void VmonReaderTreeMenu::update_times()
{
  QDateTime datime;
  { ClockTime t = _start_time;
    datime.setTime_t(t.seconds());
    _start_label->setText(QString("%1.%2")
			  .arg(datime.toString("yyyy MMM dd hh:mm:ss"))
			  .arg(QString::number(t.nanoseconds()/1000000),3,QChar('0'))); }
  { ClockTime t = _stop_time;
    datime.setTime_t(t.seconds());
    _stop_label ->setText(QString("%1.%2")
			  .arg(datime.toString("yyyy MMM dd hh:mm:ss"))
			  .arg(QString::number(t.nanoseconds()/1000000),3,QChar('0'))); }
}
