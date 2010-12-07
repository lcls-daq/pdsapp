#include "pdsapp/config/SequencerConfig.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/GlobalCfg.hh"
#include "pdsapp/config/PdsDefs.hh"
#include "pdsapp/config/EvrEventCodeTable.hh"
#include "pds/config/EvrConfigType.hh"
#include "pds/config/SeqConfigType.hh"

#include <QtGui/QWidget>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QStackedWidget>
#include <QtGui/QTabWidget>
#include <QtGui/QLabel>
#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QScrollArea>
#include <QtGui/QComboBox>
#include <QtGui/QLineEdit>
#include <QtGui/QImage>
#include <QtGui/QIntValidator>
#include <QtGui/QGroupBox>
#include <QtGui/QApplication>

#include <list>

static const unsigned fiducials[] = { 3, 6, 12, 36, 72, 360, 720, 0 };

static const char* rateList[] = { "120 Hz", "60 Hz", "30 Hz", "10 Hz", 
				  "5 Hz", "1 Hz", "0.5 Hz", "Disable", 
				  NULL };

static void setInputValue(QWidget* w, int v)
{
  if (Pds_ConfigDb::Parameter::allowEdit())
    static_cast<QLineEdit*>(w)->setText(QString::number(v));
  else
    static_cast<QLabel*>(w)->setText(QString::number(v));
}

static int getInputValue(QWidget* w)
{
  int v;
  if (Pds_ConfigDb::Parameter::allowEdit())
    v = static_cast<QLineEdit*>(w)->text().toInt();
  else
    v = static_cast<QLabel*>(w)->text().toInt();
  return v;
}

namespace Pds_ConfigDb {


  //
  //  Fixed rate selection
  //
  class FixedConfig : public QWidget {
  public:
    FixedConfig(const EvrEventCodeTable& t) :
      _code_table (t),
      _readoutRate("Readout Rate", SeqConfigType::r120Hz, rateList)
    { setLayout(_readoutRate.initialize(this)); }
  public:
    void update() { _readoutRate.update(); }
    void flush () { _readoutRate.flush (); }
    void push(void* to) const {
      SeqConfigType::Source rate = _readoutRate.value;
      SeqEntryType entry( _code_table.code_lookup(0),
			  fiducials[rate] );
      new(to) SeqConfigType( SeqConfigType::r0_5Hz,
			     rate, 1, 0, &entry);
    }
  private:
    const EvrEventCodeTable&          _code_table;
    Enumerated<SeqConfigType::Source> _readoutRate;
  };


  class DetailConfig : public QWidget {
  public:
    DetailConfig(const EvrEventCodeTable& t) :
      _code_table (t),
      _syncSource ("Sync Source", SeqConfigType::r0_5Hz, rateList),
      _beamSource ("Beam Source", SeqConfigType::r120Hz, rateList),
      _cycles     ("Cycles"     , 0, 0, 32*1024),
      _seqInput   ( new SeqEntryInput(t) )
    { 
      QVBoxLayout* layout = new QVBoxLayout;
      layout->addStretch();
//       layout->addLayout(_syncSource.initialize(0));
//       layout->addLayout(_beamSource.initialize(0));
      QHBoxLayout* hl = static_cast<QHBoxLayout*>(_cycles.initialize(0));
      hl->addWidget(new QLabel("(0 = continuous)"));
      _cycles.widget()->setMaximumWidth(40);
      layout->addLayout(hl);
      layout->addWidget(_seqInput);
      layout->addStretch();
      setLayout(layout); 
    }
  public:
    unsigned length() const { return _seqInput->nentries(); }
    void pull(const SeqConfigType& c) { _seqInput->pull(c); }
    void push(void* to) const   {
      unsigned n = _seqInput->nentries();
      SeqEntryType* entries = _seqInput->entries();
      new(to) SeqConfigType( SeqConfigType::Source(_syncSource.value),
			     SeqConfigType::Source(_beamSource.value),
			     n, 0, entries);
    }
    bool validate() { return true; }
  public:
    void update() 
    { 
      //      _syncSource.update(); _beamSource.update(); 
      _cycles.update(); _seqInput->update(); 
    }
    void flush () 
    {
      //      _syncSource.flush (); _beamSource.flush (); 
      _cycles.flush (); _seqInput->flush (); 
    }
  private:
    const EvrEventCodeTable&          _code_table;
    Enumerated<SeqConfigType::Source> _syncSource;
    Enumerated<SeqConfigType::Source> _beamSource;
    NumericInt<unsigned>              _cycles;
    SeqEntryInput*                    _seqInput;
  };

};

using namespace Pds_ConfigDb;


SequencerConfig::SequencerConfig(const EvrEventCodeTable& table) :
  Parameter(NULL),
  _code_table   (table),
  _config_buffer(0)
{
}

SequencerConfig::~SequencerConfig()
{
  if (_config_buffer)
    delete _config_buffer;
}

QLayout* SequencerConfig::initialize(QWidget*) {
  QHBoxLayout* hl = new QHBoxLayout;
  { QVBoxLayout* vl = new QVBoxLayout;
    QRadioButton* fixed   = new QRadioButton("Fixed Rate");
    QRadioButton* detail  = new QRadioButton("Details");
    QRadioButton* disable = new QRadioButton("External");
    fixed  ->setEnabled(Parameter::allowEdit());
    detail ->setEnabled(Parameter::allowEdit());
    disable->setEnabled(Parameter::allowEdit());
    _mode = new QButtonGroup;
    _mode->addButton(fixed  ,Fixed);
    _mode->addButton(detail ,Detail);
    _mode->addButton(disable,External);
    vl->addStretch();
    vl->addWidget(fixed);
    vl->addWidget(detail);
    vl->addWidget(disable);
    vl->addStretch();
    hl->addLayout(vl); }
  hl->addStretch();
  { _stack = new QStackedWidget;
    _stack->addWidget(_fixed  = new FixedConfig (_code_table));
    _stack->addWidget(_detail = new DetailConfig(_code_table));
    _stack->addWidget(new QLabel("Sequencer configured externally"));
    hl->addWidget(_stack);
    _mode->button(0)->setChecked(true); _stack->setCurrentIndex(0);
    ::QObject::connect(_mode, SIGNAL(buttonClicked(int)), _stack, SLOT(setCurrentIndex(int))); }

  return hl;
}

void     SequencerConfig::update    () { _fixed->update(); _detail->update(); }

void     SequencerConfig::flush     () { _fixed->flush (); _detail->flush (); }

void     SequencerConfig::enable    (bool) {}

void  SequencerConfig::pull  (const EvrConfigType& evr) {
  const SeqConfigType& cfg = evr.seq_config();
  _detail->pull(cfg);
  if (cfg.sync_source()==SeqConfigType::Disable) {
    _mode->button(External)->setChecked(true);
    _stack->setCurrentIndex(External);
  }
  else {
    _mode->button(Detail)->setChecked(true);
    _stack->setCurrentIndex(Detail); 
  }
}

bool SequencerConfig::validate() { 

  if (_mode->checkedId()==Detail)
    if (!_detail->validate())
      return false;

  if (_config_buffer)
    delete _config_buffer;

  unsigned nentries;
  switch(_mode->checkedId()) {
  case Fixed : nentries=1; break;
  case Detail: nentries=_detail->length(); break;
  case External: 
  default: nentries=0; break;
  };

  _config_buffer = 
    new char[sizeof(SeqConfigType)
             +nentries*sizeof(Pds::EvrData::SequencerEntry)];

  switch(_mode->checkedId()) {
  case Fixed   : _fixed ->push(_config_buffer); break;
  case Detail  : _detail->push(_config_buffer); break;
  case External: // Disabled
    new(_config_buffer) SeqConfigType( SeqConfigType::Disable,
                                       SeqConfigType::Disable,
                                       0, 0, 0 );
    break;
  }

  return true;
}

const EvrConfigType::SeqConfigType& SequencerConfig::result() const
{
  return *reinterpret_cast<const EvrConfigType::SeqConfigType*>(_config_buffer);
}


SeqEntryInput::SeqEntryInput(const EvrEventCodeTable& t) :
  QWidget      (0),
  _code_table  (t),
  _ulength     (0),
  _length      ("Length",0,0,32*1024)
{ 
  QVBoxLayout* vl = new QVBoxLayout;
  { QHBoxLayout* hl = static_cast<QHBoxLayout*>(_length.initialize(0));
    _length.widget()->setMaximumWidth(40);
    hl->addStretch();
    vl->addLayout(hl); }
  { QGridLayout* layout = _elayout = new QGridLayout;
    layout->addWidget(new QLabel("Event")       ,0,0,::Qt::AlignCenter);
    layout->addWidget(new QLabel("Delay\nStep" ),0,1,::Qt::AlignCenter);
    layout->addWidget(new QLabel("Delay\nTotal"),0,2,::Qt::AlignCenter);
    vl->addLayout(layout); }
  setLayout(vl);
  if (Parameter::allowEdit())
    connect(_length._input, SIGNAL(editingFinished()), this, SLOT(update_length()));
  connect(&_code_table, SIGNAL(update_codes(bool)), this, SLOT(update_codes(bool)));
}

void SeqEntryInput::pull(const SeqConfigType&     c) {

  if (Parameter::allowEdit())
    disconnect(_length._input, SIGNAL(editingFinished()), this, SLOT(update_length()));
  _length.value = c.length();

  update_length();

  for(int i=0; i<_elayout->count(); i++) {
    int row, col, rowspan, colspan;
    _elayout->getItemPosition(i, &row, &col, &rowspan, &colspan);
    if (row>0) {
      if (col==0) {
	QComboBox* event = static_cast<QComboBox*>
	  (static_cast<QWidgetItem*>(_elayout->itemAt(i))->widget());
	event->setCurrentIndex(_code_table.code_index(c.entry(row-1).eventcode()));
      }
      else if (col==1) {
        setInputValue(static_cast<QWidgetItem*>(_elayout->itemAt(i))->widget(),
                      c.entry(row-1).delay());
      }
    }
  }

  if (Parameter::allowEdit())
    connect(_length._input, SIGNAL(editingFinished()), this, SLOT(update_length()));

  update_totals();
}

void SeqEntryInput::update_totals() 
{
  if (!_ulength) return;

  unsigned* val = new unsigned[_ulength];
  for(int i=0; i<_elayout->count(); i++) {
    int row, col, rowspan, colspan;
    _elayout->getItemPosition(i, &row, &col, &rowspan, &colspan);
    if (col==1 && row>0) {
      val[row-1] = getInputValue(static_cast<QWidgetItem*>(_elayout->itemAt(i))->widget());
    }
  }

  for(unsigned i=1; i<_ulength; i++)
    val[i] += val[i-1];

  for(int i=0; i<_elayout->count(); i++) {
    int row, col, rowspan, colspan;
    _elayout->getItemPosition(i, &row, &col, &rowspan, &colspan);
    if (col==2 && row>0) {
      QLabel* total = static_cast<QLabel*   >
	(static_cast<QWidgetItem*>(_elayout->itemAt(i))->widget());
      total->setText(QString::number(val[row-1],10));
    }
  }
  delete[] val;
}

void SeqEntryInput::update_length()
{
  //  unsigned len = _length._input->text().toInt();
  unsigned len = _length.value;

  //  remove extra entry widgets
  if (_ulength>len) {
    std::list<QLayoutItem*> items;
    for(int i=0; i<_elayout->count(); i++) {
      int row, col, rowspan, colspan;
      _elayout->getItemPosition(i, &row, &col, &rowspan, &colspan);
      if (row>int(len))
	items.push_back(_elayout->itemAt(i));
    }
    for(std::list<QLayoutItem*>::iterator it=items.begin(); it!=items.end(); it++) {
      QLayoutItem* item = *it;
      _elayout->removeItem(item);
      delete item->widget();
      delete item;
    }
  }

  QStringList code_names = _code_table.code_names();

  int i=_ulength;
  while(i<int(len)) {
    QComboBox* event = new QComboBox;
    event->setMinimumWidth(140);
    event->addItems(code_names);

    if (Parameter::allowEdit()) {
      QLineEdit* step  = new QLineEdit("0");
      step->setMaximumWidth(40);
      step->setValidator(new QIntValidator(0,1024,this));
      QLabel*    total = new QLabel;

      ++i;
      _elayout->addWidget(event, i, 0, ::Qt::AlignCenter);
      _elayout->addWidget(step , i, 1, ::Qt::AlignCenter);
      _elayout->addWidget(total, i, 2, ::Qt::AlignCenter);
      
      event->setCurrentIndex(0);
      connect(event, SIGNAL(currentIndexChanged(int)), this, SLOT(update_codes(int)));
      step ->setText (QString::number(0,10));
      connect(step, SIGNAL(editingFinished()), this, SLOT(update_totals()));
    }
    else {
      QLabel* step  = new QLabel("0");
      step->setMaximumWidth(40);
      QLabel*    total = new QLabel;
      event->setEnabled(false);

      ++i;
      _elayout->addWidget(event, i, 0, ::Qt::AlignCenter);
      _elayout->addWidget(step , i, 1, ::Qt::AlignCenter);
      _elayout->addWidget(total, i, 2, ::Qt::AlignCenter);
      
      event->setCurrentIndex(0);
      connect(event, SIGNAL(currentIndexChanged(int)), this, SLOT(update_codes(int)));
      step ->setText (QString::number(0,10));
    }
  }						   

  _ulength = len;

  update_totals();
}

void SeqEntryInput::update_codes(int) { update_codes(true); }

void SeqEntryInput::update_codes(bool)
{
  QStringList code_names = _code_table.code_names();

  std::list<int> items;
  for(int i=0; i<_elayout->count(); i++) {
    int row, col, rowspan, colspan;
    _elayout->getItemPosition(i, &row, &col, &rowspan, &colspan);
    if (col==0 && row>0)
      items.push_back(i);
  }
  for(std::list<int>::iterator it=items.begin(); it!=items.end(); it++) {
    QLayoutItem* item = _elayout->itemAt(*it);
    
    QComboBox* box = static_cast<QComboBox*>(item->widget());
    QString ch = box->currentText();
    
    disconnect(box, SIGNAL(currentIndexChanged(int)), this, SLOT(update_codes(int)));

    box->clear();
    box->addItems(code_names);

    bool lfound = false;
    for(int i=0; i<code_names.size(); i++)
      if (ch==code_names[i]) {
        lfound = true;
        box->setCurrentIndex(i);
        box->setPalette(QApplication::palette());
        break;
      }

    if (!lfound) 
      box->setPalette(QPalette(Qt::red));

    connect(box, SIGNAL(currentIndexChanged(int)), this, SLOT(update_codes(int)));
  }						   

}

SeqEntryType* SeqEntryInput::entries() const 
{
  unsigned n = nentries();
  SeqEntryType* v = new SeqEntryType[n];
  for(int i=0; i<_elayout->count(); i++) {
    int row, col, rowspan, colspan;
    _elayout->getItemPosition(i, &row, &col, &rowspan, &colspan);
    if (row>0) {
      if (col==0) {
	QComboBox* event = static_cast<QComboBox*>
	  (static_cast<QWidgetItem*>(_elayout->itemAt(i))->widget());
	v[row-1] = SeqEntryType(_code_table.code_lookup(event->currentIndex()), 
                                v[row-1].delay());
      }
      else if (col==1) {
	v[row-1] = SeqEntryType(v[row-1].eventcode(), 
                                getInputValue(static_cast<QWidgetItem*>(_elayout->itemAt(i))->widget()));
      }
    }
  }
  return v;
}

unsigned     SeqEntryInput::nentries() const { return _ulength; }

void SeqEntryInput::update() { _length.update(); }

void SeqEntryInput::flush () { _length.flush(); }

#include "Parameters.icc"


