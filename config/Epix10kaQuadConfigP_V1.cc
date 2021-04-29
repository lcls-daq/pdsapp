#include "pdsapp/config/Epix10kaQuadConfigP_V1.hh"

#include "pdsapp/config/Epix10kaQuad_V1.hh"
#include "pdsapp/config/Epix10kaASICdata.hh"
#include "pdsapp/config/Epix10kaCalibMap.hh"
#include "pdsapp/config/Epix10kaPixelMap.hh"
#include "pdsapp/config/Epix10ka2MGainMap.hh"
#include "pdsapp/config/Epix10kaQuadGainMap.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/CopyBox.hh"
#include "pds/config/EpixConfigType.hh"

#include <stdio.h>
#include <QtCore/QObject>

#include "pdsapp/config/Epix10ka2MMap.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QCheckBox>
#include <QtGui/QMessageBox>
#include <QtGui/QTabWidget>
#include <QtGui/QGroupBox>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>

static const int PolarityGroup = 100;

// static const unsigned ASICS   = Pds::CsPad::ASICsPerQuad;
// static const unsigned Columns = Pds::CsPad::ColumnsPerASIC;
// static const unsigned Rows    = Pds::CsPad::MaxRowsPerASIC;

static const unsigned pixelArrayShape[] = {Epix10kaElemConfig::_numberOfRowsPerAsic *
                                           Epix10kaElemConfig::_numberOfAsicsPerColumn,
                                           Epix10kaElemConfig::_numberOfPixelsPerAsicRow *
                                           Epix10kaElemConfig::_numberOfAsicsPerRow};

static const unsigned calibArrayShape[] = {Epix10kaElemConfig::_calibrationRowCountPerASIC *
                                           Epix10kaElemConfig::_numberOfAsicsPerColumn,
                                           Epix10kaElemConfig::_numberOfPixelsPerAsicRow *
                                           Epix10kaElemConfig::_numberOfAsicsPerRow};

namespace Pds_ConfigDb {
  namespace V1 {
    namespace Epix10kaQuad {
      //  class CspadGainMap;
      class GlobalP;

      class ConfigTable : public Parameter {
      public:
        ConfigTable(const Epix10kaQuadConfigP& c);
        ~ConfigTable();
      public:
        void insert(Pds::LinkedList<Parameter>& pList);
        int  pull  (const Pds::Epix::Config10kaQuadV1& tc);
        int  push  (void* to) const;
        int  dataSize() const;
        bool validate();
        //
        void update_maskg();
        void update_maskv();
        void pixel_map_dialog();
        void calib_map_dialog();
        void set_gain(int asic);
        void set_all_gain();
      public:
        QLayout* initialize(QWidget* parent);
        void     flush     ();
        void     update    ();
        void     enable    (bool);
      public:
        const Epix10kaQuadConfigP&      _cfg;
        Pds::LinkedList<Parameter>      _pList;
        GlobalP*                        _globalP;
        Epix10kaQuad::QuadP*            _quadP[4];
        ConfigTableQ*                   _qlink;
        Epix10ka2MMap*                  _pixelMap;
        QPushButton*                    _pixelMapB;
        QPushButton*                    _calibMapB;
        Epix10kaPixelMapDialog*         _pixelMapD [16];
        Epix10kaCalibMapDialog*         _calibMapD [16];
      };

      class GlobalP {
      public:
        GlobalP() :
          _evrRunCode      ( "EVR Run EventCode"     , 40, 0, 255, Decimal),
          _evrRunDelay     ( "EVR Run Delay [119MHz]", 0, 0, (1U<<31)-1, Decimal),
          _asicMask        ( "ASIC Mask"             , uint16_t(-1U), 0, uint16_t(-1U), Hex)
        {
          for(unsigned a=0; a<4; a++) {
            _pixelArray[a] = ndarray<uint16_t,2>(pixelArrayShape);
            _calibArray[a] = ndarray<uint8_t ,2>(calibArrayShape);
          }
        }
      public:
        void enable(bool v)
        {
        }
        void reset ()
        {
          _evrRunCode.value = 40;
          _evrRunDelay.value = 0;
        }
        void pull   (const Pds::Epix::Config10kaQuadV1& p)
        {
          _evrRunCode .value = p.evr().runCode ();
          _evrRunDelay.value = p.evr().runDelay();
          _asicMask   .value = 0;
          for(unsigned i=0; i<4; i++) {
            _asicMask .value |= uint16_t(p.elemCfg(i).asicMask())<<(4*i);
            for(unsigned j=0; j<4; j++)
              _asic[j+4*i].pull(*reinterpret_cast<const Epix10kaASIC_ConfigShadow*>(&p.elemCfg(i).asics(j)));
            std::copy(p.elemCfg(i).asicPixelConfigArray().begin(),
                      p.elemCfg(i).asicPixelConfigArray().end(),
                      _pixelArray[i].begin());
            std::copy(p.elemCfg(i).calibPixelConfigArray().begin(),
                      p.elemCfg(i).calibPixelConfigArray().end(),
                      _calibArray[i].begin());
          }
        }
        void push   (Pds::Epix::Config10kaQuadV1* p)
        {
          Pds::Epix::PgpEvrConfig evr(1U, _evrRunCode.value, 0, _evrRunDelay.value);

          Epix10kaElemConfig elemCfg[4];
          for(unsigned i=0; i<4; i++) {
            Pds::Epix::Asic10kaConfigV1 asics[4];
            for(unsigned j=0; j<4; j++)
              _asic[j+4*i].push(reinterpret_cast<Epix10kaASIC_ConfigShadow*>(&asics[j]));
            elemCfg[i] = Epix10kaElemConfig( 0, 0, (_asicMask.value>>(4*i))&0xf,
                                                asics,
                                                _pixelArray[i].begin(),
                                                _calibArray[i].begin() );
          }

          __attribute__((unused)) Pds::Epix::Config10kaQuadV1& c =
          *new (p) Pds::Epix::Config10kaQuadV1( evr, Epix10kaQuadConfigV1(), elemCfg );
        }
      public:
        void initialize(QWidget* parent, QVBoxLayout* layout)
        {
          layout->addLayout(_evrRunCode     .initialize(parent));
          layout->addLayout(_evrRunDelay    .initialize(parent));
          layout->addSpacing(40);
          layout->addWidget(_asicGainMap = new Epix10ka2MGainMap(1, _pixelArray, _asic));
          layout->addSpacing(40);

          _gainB = new QButtonGroup;
          _gainB->addButton(new QRadioButton("High")   ,Epix10kaQuadGainMap::_HIGH_GAIN);
          _gainB->addButton(new QRadioButton("Medium") ,Epix10kaQuadGainMap::_MEDIUM_GAIN);
          _gainB->addButton(new QRadioButton("Low")    ,Epix10kaQuadGainMap::_LOW_GAIN);
          _gainB->addButton(new QRadioButton("Auto HL"),Epix10kaQuadGainMap::_AUTO_HL_GAIN);
          _gainB->addButton(new QRadioButton("Auto ML"),Epix10kaQuadGainMap::_AUTO_ML_GAIN);
          for(unsigned i=0; i<5; i++) {
            QPalette p;
            p.setColor(QPalette::ButtonText,
                       QColor(Epix10kaQuadGainMap::rgb(Epix10kaQuadGainMap::GainMode(i))));
            p.setColor(QPalette::Text,
                       QColor(Epix10kaQuadGainMap::rgb(Epix10kaQuadGainMap::GainMode(i))));
            QAbstractButton* b = _gainB->button(i);
            b->setPalette(p);
            layout->addWidget(b);
          }
          layout->addWidget(_allGainB = new QPushButton("Set Gain for All"));
        }

        void initialize_mask(QWidget* parent, QVBoxLayout* layout)
        {
          layout->addWidget(_asicMaskMap = new Epix10ka2MMap(1));
          layout->addLayout(_asicMask.initialize(parent));
        }

        void connect(ConfigTableQ* qlink) {
          ::QObject::connect(_asicMaskMap, SIGNAL(changed()), qlink, SLOT(update_maskv()));
          ::QObject::connect(_allGainB   , SIGNAL(pressed()), qlink, SLOT(set_all_gain()));
          ::QObject::connect(_asicGainMap, SIGNAL(clicked(int)), qlink, SLOT(set_gain(int)));
        }

        void insert(Pds::LinkedList<Parameter>& pList) {
          pList.insert(&_evrRunCode);
          pList.insert(&_evrRunDelay);
          pList.insert(&_asicMask);
          for(unsigned j=0; j<16; j++)
            pList.insert(&_asic[j]);
        }

        void set_all_gain() {
          int id = _gainB->checkedId();
          if (id < 0) return;
          Epix10kaQuadGainMap::GainMode gm = Epix10kaQuadGainMap::GainMode(id);
          unsigned mapv  = 0;
          unsigned trbit = 0;
          switch(gm) {
          case Epix10kaQuadGainMap::_HIGH_GAIN   : mapv = 0xc; trbit = 1; break;
          case Epix10kaQuadGainMap::_MEDIUM_GAIN : mapv = 0xc; trbit = 0; break;
          case Epix10kaQuadGainMap::_LOW_GAIN    : mapv = 0x8; trbit = 0; break;
          case Epix10kaQuadGainMap::_AUTO_HL_GAIN: mapv = 0x0; trbit = 1; break;
          case Epix10kaQuadGainMap::_AUTO_ML_GAIN: mapv = 0x0; trbit = 0; break;
          default: break;
          }
          for(unsigned i=0; i<4; i++)
            for(ndarray<uint16_t,2>::iterator it=_pixelArray[i].begin(); it!=_pixelArray[i].end(); it++)
              *it = mapv;
          for(unsigned j=0; j<16; j++) {
            _asic[j]._reg[Epix10kaASIC_ConfigShadow::trbit]->value = trbit;
            _asic[j].flush();
          }
          _asicGainMap->update();
        }

        void set_gain(int asic) {
          int id = _gainB->checkedId();
          if (id < 0) return;
          Epix10kaQuadGainMap::GainMode gm = Epix10kaQuadGainMap::GainMode(id);
          unsigned mapv  = 0;
          unsigned trbit = 0;
          switch(gm) {
          case Epix10kaQuadGainMap::_HIGH_GAIN   : mapv = 0xc; trbit = 1; break;
          case Epix10kaQuadGainMap::_MEDIUM_GAIN : mapv = 0xc; trbit = 0; break;
          case Epix10kaQuadGainMap::_LOW_GAIN    : mapv = 0x8; trbit = 0; break;
          case Epix10kaQuadGainMap::_AUTO_HL_GAIN: mapv = 0x0; trbit = 1; break;
          case Epix10kaQuadGainMap::_AUTO_ML_GAIN: mapv = 0x0; trbit = 0; break;
          default: break;
          }
          unsigned i = asic>>2;
          ndarray<uint16_t,2> pa = _pixelArray[i];
          unsigned yo = pa.shape()[0]/2;
          unsigned xo = pa.shape()[1]/2;
          switch(asic&3) {
          case 0:
            { for(unsigned y=yo; y<yo*2; y++)
                for(unsigned x=xo; x<xo*2; x++)
                  pa[y][x] = mapv; } break;
          case 1:
            { for(unsigned y=0; y<yo; y++)
                for(unsigned x=xo; x<xo*2; x++)
                  pa[y][x] = mapv; } break;
          case 2:
            { for(unsigned y=0; y<yo; y++)
                for(unsigned x=0; x<xo; x++)
                  pa[y][x] = mapv; } break;
          case 3:
            { for(unsigned y=yo; y<yo*2; y++)
                for(unsigned x=0; x<xo; x++)
                  pa[y][x] = mapv; } break;
          }
          _asic[asic]._reg[Epix10kaASIC_ConfigShadow::trbit]->value = trbit;
          _asic[asic].flush();
          _asicGainMap->update();
        }

      public:
        NumericInt<unsigned>             _evrRunCode;
        NumericInt<unsigned>             _evrRunDelay;
        Epix10ka2MGainMap*               _asicGainMap;
        Epix10ka2MMap*                   _asicMaskMap;
        NumericInt<uint16_t>             _asicMask;
        Epix10kaASICdata                 _asic[16];
        ndarray<uint16_t,2>              _pixelArray[4];
        ndarray<uint8_t ,2>              _calibArray[4];
        QButtonGroup*                    _gainB;
        QPushButton*                     _allGainB;
      };

      class AdcCopyBox : public CopyBox {
      public:
        AdcCopyBox(ConfigTable& table) :
          CopyBox("Copy ADCs","Q","",1,10),
          _table(table) {}
      public:
        void initialize() { _table.update(); }
        void finalize  () { _table.flush(); }
        void copyItem(unsigned fromRow,
                      unsigned fromColumn,
                      unsigned toRow,
                      unsigned toColumn)
        {
          _table._quadP[toRow]->_adc[toColumn] = _table._quadP[fromRow]->_adc[fromColumn];
        }
      private:
        ConfigTable& _table;
      };

      class AsicCopyBox : public CopyBox {
      public:
        AsicCopyBox(ConfigTable& table) :
          CopyBox("Copy ASICs","Q","A",1,16),
          _table(table) {}
      public:
        void initialize() { _table.update(); }
        void finalize  () { _table.flush(); }
        void copyItem(unsigned fromRow,
                      unsigned fromColumn,
                      unsigned toRow,
                      unsigned toColumn)
        {
          _table._globalP->_asic[toColumn+16*toRow].copy(_table._globalP->_asic[fromColumn+16*fromRow]);
        }
      private:
        ConfigTable& _table;
      };
    };
  };
};

using namespace Pds_ConfigDb::V1::Epix10kaQuad;

ConfigTable::ConfigTable(const Epix10kaQuadConfigP& c) :
  Parameter(NULL),
  _cfg(c)
{
  _globalP = new GlobalP;
  //  _gainMap = new Epix10ka2MGainMap;
  for(unsigned q=0; q<1; q++)
    _quadP[q] = new Epix10kaQuad::QuadP;
  for(unsigned a=0; a<4; a++) {
    _pixelMapD[a] = new Epix10kaPixelMapDialog(_globalP->_pixelArray[a]);
    _calibMapD[a] = new Epix10kaCalibMapDialog(_globalP->_calibArray[a]);
  }
}

ConfigTable::~ConfigTable()
{
}

void ConfigTable::insert(Pds::LinkedList<Parameter>& pList)
{
  pList.insert(this);

  _globalP->insert(_pList);
  //  _gainMap->insert(_pList);
  for(unsigned q=0; q<1; q++)
    _quadP[q]->insert(_pList);
}

int ConfigTable::pull(const Pds::Epix::Config10kaQuadV1& tc) {
  _globalP->pull(tc);
  _quadP[0]->pull(tc.quad()); //,_gainMap->quad(q));
  //  _gainMap->flush();

  return tc._sizeof();
}

int ConfigTable::push(void* to) const {

  Pds::Epix::Config10kaQuadV1& tc = *reinterpret_cast<Pds::Epix::Config10kaQuadV1*>(to);
  _globalP->push(&tc);
  _quadP[0]->push(&const_cast<Epix10kaQuadConfigV1&>(tc.quad()));
  return tc._sizeof();
}

int ConfigTable::dataSize() const {
  return sizeof(Pds::Epix::Config10kaQuadV1);
}

bool ConfigTable::validate()
{
  return true;
}

QLayout* ConfigTable::initialize(QWidget* parent)
{
#define ADDTAB(layout,title) { \
    QWidget*      w = new QWidget; \
    QVBoxLayout* lv = new QVBoxLayout; lv->addLayout(layout); lv->addStretch(1); \
    QHBoxLayout* lh = new QHBoxLayout; lh->addLayout(lv); lh->addStretch(1); \
    w->setLayout(lh); tab->addTab(w,title); }

  QHBoxLayout* layout = new QHBoxLayout;
  { QTabWidget* tab = new QTabWidget;
    { QVBoxLayout* gl = new QVBoxLayout;
      _globalP->initialize(parent,gl);
      ADDTAB(gl,"Global");
    { QGridLayout* gl_sys = new QGridLayout;
      QGridLayout* gl_acq = new QGridLayout;
      QGridLayout* gl_sco = new QGridLayout;
      QGridLayout* gl_adc[10];  for(unsigned i=0; i<10; i++) gl_adc[i] = new QGridLayout;
      Epix10kaASICdata::setColumns(4);
      QLayout* gl_asi[16];
      for(unsigned a=0; a<16; a++)
        gl_asi[a] = _globalP->_asic[a].initialize(parent);

      Epix10kaQuad::QuadP::initialize_tabs(parent,gl_sys,gl_acq,gl_sco,gl_adc,gl_asi,1);
      for(unsigned q=0; q<1; q++)
        _quadP[q]->initialize(parent,gl_sys,gl_acq,gl_sco,&gl_adc[q*10],&gl_asi[q*16],q);
      ADDTAB(gl_sys,"System");
      ADDTAB(gl_acq,"Acq+Rdo");
      ADDTAB(gl_sco,"Scope");

      QTabWidget* tab_adc = new QTabWidget;
      tab_adc->setTabPosition(QTabWidget::West);
      for(unsigned q=0; q<1; q++) {
        QTabWidget* quadTab = new QTabWidget;
        quadTab->setTabPosition(QTabWidget::North);
        for(unsigned ch=0; ch<10; ch++) {
          QWidget* w = new QWidget;
          w->setLayout(gl_adc[q*10+ch]);
          quadTab->addTab(w,QString("Chip %1").arg(ch));
        }
        tab_adc->addTab(quadTab,QString("Quad %1").arg(q));
      }
      { QVBoxLayout* vl = new QVBoxLayout;
        vl->addWidget(tab_adc);
        vl->addWidget(new AdcCopyBox(*this));
        vl->addStretch(1);
        ADDTAB(vl, "ADC"); }
      QTabWidget* tab_asi = new QTabWidget;
      tab_asi->setTabPosition(QTabWidget::West);
      for(unsigned q=0; q<1; q++) {
        QTabWidget* quadTab = new QTabWidget;
        quadTab->setTabPosition(QTabWidget::North);
        for(unsigned ch=0; ch<16; ch++) {
          QWidget* w = new QWidget;
          w->setLayout(gl_asi[q*16+ch]);
          quadTab->addTab(w,QString("A%1").arg(ch));
        }
        tab_asi->addTab(quadTab,QString("Quad %1").arg(q));
      }
      { QVBoxLayout* vl = new QVBoxLayout;
        vl->addWidget(tab_asi);
        vl->addWidget(new AsicCopyBox(*this));
        ADDTAB(vl, "ASICs"); }
      { QVBoxLayout* vl = new QVBoxLayout;
        _globalP->initialize_mask(parent,vl);
        ADDTAB(vl, "ASIC Mask"); }
    }
    { QVBoxLayout* vl = new QVBoxLayout;
      vl->addWidget(_pixelMap  = new Epix10ka2MMap(1,2,true));
      vl->addWidget(_pixelMapB = new QPushButton("Pixel Maps"));
      vl->addWidget(_calibMapB = new QPushButton("Calib Maps"));
      ADDTAB(vl, "Maps"); }
    }
#undef ADDTAB
    layout->addWidget(tab); }

  update_maskg();
  _pixelMap->update(0);

  _qlink = new ConfigTableQ(*this,parent);
  _globalP->connect(_qlink);
  ::QObject::connect(_pixelMapB, SIGNAL(pressed()), _qlink, SLOT(pixel_map_dialog()));
  ::QObject::connect(_calibMapB, SIGNAL(pressed()), _qlink, SLOT(calib_map_dialog()));
  if (allowEdit())
    ::QObject::connect(_globalP->_asicMask._input, SIGNAL(editingFinished()), _qlink, SLOT(update_maskg()));

  return layout;
#undef ADDTAB
}

void ConfigTable::update() {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->update();
    p = p->forward();
  }
  update_maskg();
}

void ConfigTable::flush () {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->flush();
    p = p->forward();
  }
  update_maskg();
}

void ConfigTable::enable(bool)
{
}

void ConfigTable::update_maskg()
{
  _globalP->_asicMaskMap->update(_globalP->_asicMask.value);
  _globalP->_asicGainMap->update();
}

void ConfigTable::update_maskv()
{
  _globalP->_asicMask.value = _globalP->_asicMaskMap->value();
  _globalP->_asicMask.flush();
}

void ConfigTable::pixel_map_dialog()
{
  uint64_t v = _pixelMap->value();
  for(unsigned i=0; i<16; i++)
    if ( ((v>>(4*i))&0xf)!=0 ) {
      _pixelMapD[i]->show_map();
      _pixelMapD[i]->exec();
      return;
    }
}

void ConfigTable::calib_map_dialog()
{
  uint64_t v = _pixelMap->value();
  for(unsigned i=0; i<4; i++)
    if ( ((v>>(16*i))&0xffff)!=0 ) {
      _calibMapD[i]->show_map();
      _calibMapD[i]->exec();
      return;
    }
}

void ConfigTable::set_gain(int a) { _globalP->set_gain(a); }
void ConfigTable::set_all_gain() { _globalP->set_all_gain(); }

// ConfigTableQ::ConfigTableQ(GlobalP& table,
//                                                QWidget* parent) :
//   QObject(parent),
//   _table (table)
// {
// }

// void ConfigTableQ::update_readout() { _table.update_readout(); }

#include "Parameters.icc"


Pds_ConfigDb::V1::Epix10kaQuadConfigP::Epix10kaQuadConfigP():
  Serializer("Epix10kaQuadP_Config"), _table(new ConfigTable(*this))
{
  _table->insert(pList);
}

Pds_ConfigDb::V1::Epix10kaQuadConfigP::~Epix10kaQuadConfigP() {}

int Pds_ConfigDb::V1::Epix10kaQuadConfigP::readParameters(void *from)
{
  return _table->pull(*reinterpret_cast<const Pds::Epix::Config10kaQuadV1*>(from));
}

int Pds_ConfigDb::V1::Epix10kaQuadConfigP::writeParameters(void *to)
{
  return _table->push(to);
}

int Pds_ConfigDb::V1::Epix10kaQuadConfigP::dataSize() const
{
  return _table->dataSize();
}

bool Pds_ConfigDb::V1::Epix10kaQuadConfigP::validate()
{
  return _table->validate();
}


ConfigTableQ::ConfigTableQ(ConfigTable& table, QObject* parent) :
  QObject(parent),
  _table(table) {}

void ConfigTableQ::update_maskv    () { _table.update_maskv    (); }
void ConfigTableQ::update_maskg    () { _table.update_maskg    (); }
void ConfigTableQ::pixel_map_dialog() { _table.pixel_map_dialog(); }
void ConfigTableQ::calib_map_dialog() { _table.calib_map_dialog(); }

void ConfigTableQ::set_all_gain    () { _table.set_all_gain(); }
void ConfigTableQ::set_gain   (int a) { _table.set_gain(a); }

