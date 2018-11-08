#include "pdsapp/config/Epix10kaQuadConfigP.hh"

#include "pdsapp/config/Epix10kaQuad.hh"
#include "pdsapp/config/Epix10kaASICdata.hh"
#include "pdsapp/config/Epix10kaCalibMap.hh"
#include "pdsapp/config/Epix10kaPixelMap.hh"
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

static const int PolarityGroup = 100;

// static const unsigned ASICS   = Pds::CsPad::ASICsPerQuad;
// static const unsigned Columns = Pds::CsPad::ColumnsPerASIC;
// static const unsigned Rows    = Pds::CsPad::MaxRowsPerASIC;

static const unsigned pixelArrayShape[] = {Pds::Epix::Config10ka::_numberOfRowsPerAsic *
                                           Pds::Epix::Config10ka::_numberOfAsicsPerColumn,
                                           Pds::Epix::Config10ka::_numberOfPixelsPerAsicRow *
                                           Pds::Epix::Config10ka::_numberOfAsicsPerRow};

static const unsigned calibArrayShape[] = {Pds::Epix::Config10ka::_calibrationRowCountPerASIC *
                                           Pds::Epix::Config10ka::_numberOfAsicsPerColumn,
                                           Pds::Epix::Config10ka::_numberOfPixelsPerAsicRow *
                                           Pds::Epix::Config10ka::_numberOfAsicsPerRow};

namespace Pds_ConfigDb {
  namespace Epix10kaQuad {
    //  class CspadGainMap;
    class GlobalP;

    class ConfigTable : public Parameter {
    public:
      ConfigTable(const Epix10kaQuadConfigP& c);
      ~ConfigTable();
    public:
      void insert(Pds::LinkedList<Parameter>& pList);
      int  pull  (const Epix10kaQuadConfigType& tc);
      int  push  (void* to) const;
      int  dataSize() const;
      bool validate();
      //
      void update_maskg();
      void update_maskv();
      void pixel_map_dialog();
      void calib_map_dialog();
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
        _evrDaqCode      ( "EVR Acq EventCode"     , 40, 0, 255, Decimal),
        _evrRunDelay     ( "EVR Run Delay [119MHz]", 0, 0, (1U<<31)-1, Decimal),
        _asicMask        ( "ASIC Mask"             , -1U, 0, -1U, Hex)
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
        _evrDaqCode.value = 40;
        _evrRunDelay.value = 0;
      }
      void pull   (const Epix10kaQuadConfigType& p)
      {
        _evrRunCode .value = p.evr().runCode ();
        _evrDaqCode .value = p.evr().daqCode ();
        _evrRunDelay.value = p.evr().runDelay();
        _asicMask   .value = 0;
        for(unsigned i=0; i<4; i++) {
          _asicMask .value |= uint16_t(p.elemCfg(i).asicMask())<<(4*i);
          for(unsigned j=0; j<4; j++)
            _asic[j+4*i].pull(reinterpret_cast<const Epix10kaASIC_ConfigShadow*>(&p.elemCfg(i).asics(j)));
          std::copy(p.elemCfg(i).asicPixelConfigArray().begin(),
                    p.elemCfg(i).asicPixelConfigArray().end(),
                    _pixelArray[i].begin());
          std::copy(p.elemCfg(i).calibPixelConfigArray().begin(),
                    p.elemCfg(i).calibPixelConfigArray().end(),
                    _calibArray[i].begin());
        }
      }
      void push   (Epix10kaQuadConfigType* p)
      {
        Pds::Epix::PgpEvrConfig evr(1U, _evrRunCode.value, _evrDaqCode.value, _evrRunDelay.value);

        Pds::Epix::Config10ka elemCfg[4];
        for(unsigned i=0; i<4; i++) {
          Pds::Epix::Asic10kaConfigV1 asics[4];
          for(unsigned j=0; j<4; j++)
            _asic[j+4*i].push(reinterpret_cast<Epix10kaASIC_ConfigShadow*>(&asics[j]));
          elemCfg[i] = Pds::Epix::Config10ka( 0, 0, (_asicMask.value>>(4*i))&0xf, 
                                              asics, 
                                              _pixelArray[i].begin(),
                                              _calibArray[i].begin() );
        }
        
        __attribute__((unused)) Epix10kaQuadConfigType& c =
        *new (p) Epix10kaQuadConfigType( evr, Epix10kaQuadConfig(), elemCfg );
      }
    public:
      void initialize(QWidget* parent, QVBoxLayout* layout)
      {
        layout->addLayout(_evrRunCode     .initialize(parent));
        layout->addLayout(_evrDaqCode     .initialize(parent));
        layout->addLayout(_evrRunDelay    .initialize(parent));
        layout->addSpacing(40);
        layout->addWidget(_asicMaskMap = new Epix10ka2MMap(1));
        layout->addLayout(_asicMask.initialize(parent));
      }

      void insert(Pds::LinkedList<Parameter>& pList) {
        pList.insert(&_evrRunCode);
        pList.insert(&_evrDaqCode);
        pList.insert(&_evrRunDelay);
        pList.insert(&_asicMask);
      }

    public:
      NumericInt<unsigned>             _evrRunCode;
      NumericInt<unsigned>             _evrDaqCode;
      NumericInt<unsigned>             _evrRunDelay;
      Epix10ka2MMap*                   _asicMaskMap;
      NumericInt<uint16_t>             _asicMask;
      Epix10kaASICdata                 _asic[16];
      ndarray<uint16_t,2>              _pixelArray[4];
      ndarray<uint8_t ,2>              _calibArray[4];
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

using namespace Pds_ConfigDb::Epix10kaQuad;

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

int ConfigTable::pull(const Epix10kaQuadConfigType& tc) {
  _globalP->pull(tc);
  _quadP[0]->pull(tc.quad()); //,_gainMap->quad(q));
  //  _gainMap->flush();

  return sizeof(tc);
}

int ConfigTable::push(void* to) const {

  Epix10kaQuadConfigType& tc = *reinterpret_cast<Epix10kaQuadConfigType*>(to);
  _globalP->push(&tc);
  _quadP[0]->push(&const_cast<Epix10kaQuadConfig&>(tc.quad()));
  return sizeof(tc);
}

int ConfigTable::dataSize() const {
  return sizeof(Epix10kaQuadConfigType);
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
  ::QObject::connect(_globalP->_asicMaskMap, SIGNAL(changed()), _qlink, SLOT(update_maskv()));
  ::QObject::connect(_globalP->_asicMask._input, SIGNAL(editingFinished()), _qlink, SLOT(update_maskg()));
  ::QObject::connect(_pixelMapB, SIGNAL(pressed()), _qlink, SLOT(pixel_map_dialog()));
  ::QObject::connect(_calibMapB, SIGNAL(pressed()), _qlink, SLOT(calib_map_dialog()));

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

// ConfigTableQ::ConfigTableQ(GlobalP& table,
//                                                QWidget* parent) :
//   QObject(parent),
//   _table (table)
// {
// }

// void ConfigTableQ::update_readout() { _table.update_readout(); }

#include "Parameters.icc"


Pds_ConfigDb::Epix10kaQuadConfigP::Epix10kaQuadConfigP():
  Serializer("Epix10kaQuadP_Config"), _table(new ConfigTable(*this))
{
  _table->insert(pList);
}

Pds_ConfigDb::Epix10kaQuadConfigP::~Epix10kaQuadConfigP() {}

int Pds_ConfigDb::Epix10kaQuadConfigP::readParameters(void *from)
{
  return _table->pull(*reinterpret_cast<const Epix10kaQuadConfigType*>(from));
}

int Pds_ConfigDb::Epix10kaQuadConfigP::writeParameters(void *to)
{
  return _table->push(to);
}

int Pds_ConfigDb::Epix10kaQuadConfigP::dataSize() const
{
  return _table->dataSize();
}

bool Pds_ConfigDb::Epix10kaQuadConfigP::validate() 
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


