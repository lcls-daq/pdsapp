// Graphical display of whole detector
//   - sector selection shows graphical display of sector map

// Graphical display of sector gain map
//   - shows proper orientation (square window)
//   - lo value is black, hi value is white

// Mouse click toggles pixel value
// All Clear/Set buttons
// Import/Export file option buttons (space-delimited)
// Save/Close buttons

#include "pdsapp/config/CspadGainMap.hh"
#include "pds/config/CsPadConfigType.hh"

#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QMouseEvent>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QFileDialog>

static const unsigned COLS=Pds::CsPad::ColumnsPerASIC; // 185
static const unsigned ROWS=Pds::CsPad::MaxRowsPerASIC; // 194
static const int _length  = 62;
static const int _width   = 30;
static const int xo[] = { _length+8, _length+_width+10, 
			  4, 4,
			  _width+6, 4,
			  _length+8, _length+8 };
static const int yo[] = { _length+8, _length+8,
			  _length+8, _length+_width+10,
			  4, 4,
			  4, _width+6 };
static const int frame = 134;
static const unsigned rotation[] = { 0, 1, 2, 1 };

static void setChildrenVisible(QLayout* l, bool v)
{
  for(int i=0; i<l->count(); i++) {
    QLayoutItem* item = l->itemAt(i);
    if (item->widget())
      item->widget()->setVisible(v);
    else if (item->layout())
      setChildrenVisible(item->layout(), v);
  }
}

namespace Pds_ConfigDb {

  class QuadGainMap : public QLabel {
  public:
    QuadGainMap(CspadGainMap& parent, unsigned q) : 
      _parent(parent), _quad(q),
      _gainMap(new Pds::CsPad::CsPadGainMapCfg) 
    { setFrameStyle (QFrame::NoFrame);
      memset(_gainMap, 0, sizeof(*_gainMap));
      update_sections(0); }
    ~QuadGainMap() { delete _gainMap; }
  public:
    Pds::CsPad::CsPadGainMapCfg* gainMap() const { return _gainMap; }
    void update_sections(unsigned rm)
    {
      QPixmap* image = new QPixmap(frame, frame);
      
      QRgb bg = QPalette().color(QPalette::Window).rgb();
      image->fill(bg);
      
      QPainter painter(image);
      for(unsigned j=0; j<8; j++) {
        QRgb fg = (rm   &(1<<j)) ? qRgb(0,0,255) : bg;
        painter.setBrush(QColor(fg));
        painter.drawRect(xo[j],yo[j], (j&2) ? _length : _width, (j&2) ? _width : _length);
      }
      
      QTransform transform(QTransform().rotate(90*_quad));
      setPixmap(image->transformed(transform));
    }
    void mousePressEvent( QMouseEvent* e ) 
    {
      int x,y;
      switch(_quad) {
      case 0: x=      e->x(); y=      e->y(); break;
      case 1: x=      e->y(); y=frame-e->x(); break;
      case 2: x=frame-e->x(); y=frame-e->y(); break;
      case 3: x=frame-e->y(); y=      e->x(); break;
      default: return;
      }
      
      for(unsigned j=0; j<8; j++) {
        int dx = x-xo[j];
        int dy = y-yo[j];
        if (dx>0 && dy>0) {
          if (( (j&2) && (dx < _length) && (dy < _width) ) ||
              (!(j&2) && (dx < _width ) && (dy < _length))) {
            _parent.show_map(_quad,j);
          }
        }
      }
    }
  private:
    CspadGainMap& _parent;
    unsigned      _quad;
    Pds::CsPad::CsPadGainMapCfg* _gainMap;
  };

  class SectionDisplay : public QLabel {
  public:
    SectionDisplay(QLayout& h, QLayout& v) : QLabel(0), _h(h), _v(v)
    {
      setFrameStyle(QFrame::NoFrame);
    }
  public:
    void update_map(Pds::CsPad::CsPadGainMapCfg* map, 
                                unsigned quad, 
                                unsigned section) 
    {
#if 0
      const unsigned SIZE = ROWS*2 + 8;
      QPixmap* image = new QPixmap(SIZE,SIZE);
      
      QRgb bg = QPalette().color(QPalette::Window).rgb();
      image->fill(bg);

      const unsigned col0=(SIZE-COLS)/2;
      const unsigned row0= SIZE/2+1+ROWS;
      const unsigned row1= SIZE/2-1;
#else
      const unsigned HGT = ROWS*2 + 8;
      const unsigned WDT = COLS + 8;

      QPixmap* image = new QPixmap(WDT,HGT);
      
      QRgb bg = QPalette().color(QPalette::Window).rgb();
      image->fill(bg);

      const unsigned col0=(WDT-COLS)/2;
      const unsigned row0= HGT/2+1+ROWS;
      const unsigned row1= HGT/2-1;
#endif
      unsigned asic0=(section<<1)+0;
      unsigned asic1=(section<<1)+1;
      QRgb fg;
      QPainter painter(image);
      for(unsigned col=0; col<COLS; col++) {
        for(unsigned row=0; row<ROWS; row++) {
          uint16_t v = (*map->map())[col][row];
          fg = (v&(1<<(asic0))) ? qRgb(255,255,255) : qRgb(0,0,0);
          painter.setPen(QColor(fg));
          painter.drawPoint(col+col0,row0-row);
          fg = (v&(1<<(asic1))) ? qRgb(255,255,255) : qRgb(0,0,0);
          painter.setPen(QColor(fg));
          painter.drawPoint(col+col0,row1-row);
        }
      }
      
      unsigned rot = quad + rotation[section>>1];
      QTransform transform(QTransform().rotate(90*rot));
      setPixmap(image->transformed(transform));
      setFrameStyle(QFrame::NoFrame);

      if (rot&1) {
        setChildrenVisible(&_h,true );
        setChildrenVisible(&_v,false);
      }
      else {
        setChildrenVisible(&_h,false);
        setChildrenVisible(&_v,true );
      }
    }
  private:
    QLayout& _h;
    QLayout& _v;
  };
};

using namespace Pds_ConfigDb;

CspadGainMap::CspadGainMap() : _display(0) {}

CspadGainMap::~CspadGainMap() { if (_display) delete _display; delete _quad; }

void CspadGainMap::insert(Pds::LinkedList<Parameter>& pList) {
}

Pds::CsPad::CsPadGainMapCfg* CspadGainMap::quad(int q) { 
  return _quad[q]->gainMap(); 
}

void CspadGainMap::initialize(QWidget* parent, QVBoxLayout* layout) 
{
  QGroupBox* box = new QGroupBox("GainMap");
  QVBoxLayout* l = new QVBoxLayout;
  //  Add Import,Export buttons
  QPushButton* importB = new QPushButton("Import Text File");
  QPushButton* exportB = new QPushButton("Export Text File");
  l->addWidget(importB);
  l->addWidget(exportB);
  l->addStretch();
  { QGridLayout* gl = new QGridLayout;
    gl->addWidget(_quad[0] = new QuadGainMap(*this,0),0,0,::Qt::AlignBottom|::Qt::AlignRight);
    gl->addWidget(_quad[1] = new QuadGainMap(*this,1),0,1,::Qt::AlignBottom|::Qt::AlignLeft);
    gl->addWidget(_quad[3] = new QuadGainMap(*this,3),1,0,::Qt::AlignTop   |::Qt::AlignRight);
    gl->addWidget(_quad[2] = new QuadGainMap(*this,2),1,1,::Qt::AlignTop   |::Qt::AlignLeft);
    l->addLayout(gl); }
  { QGridLayout* gl = new QGridLayout;
    QLayout *hcl, *vcl;
    { QVBoxLayout* vl = new QVBoxLayout;
      vl->addStretch();
      { QPushButton* setB = new QPushButton("Set ASIC");
        QPushButton* clrB = new QPushButton("Clear ASIC");
        connect(setB, SIGNAL(clicked()), this, SLOT(set_asic0()));
        connect(clrB, SIGNAL(clicked()), this, SLOT(clear_asic0()));
        vl->addWidget(setB);
        vl->addWidget(clrB); }
      vl->addStretch();
      { QPushButton* setB = new QPushButton("Set ASIC");
        QPushButton* clrB = new QPushButton("Clear ASIC");
        connect(setB, SIGNAL(clicked()), this, SLOT(set_asic1()));
        connect(clrB, SIGNAL(clicked()), this, SLOT(clear_asic1()));
        vl->addWidget(setB);
        vl->addWidget(clrB); }
      vl->addStretch();
      vcl = vl;
      gl->addLayout(vl,0,1); }
    { QHBoxLayout* hl = new QHBoxLayout;
      hl->addStretch();
      { QVBoxLayout* vl = new QVBoxLayout;
        QPushButton* setB = new QPushButton("Set ASIC");
        QPushButton* clrB = new QPushButton("Clear ASIC");
        connect(setB, SIGNAL(clicked()), this, SLOT(set_asic1()));
        connect(clrB, SIGNAL(clicked()), this, SLOT(clear_asic1()));
        vl->addWidget(setB);
        vl->addWidget(clrB); 
        hl->addLayout(vl); }
      hl->addStretch();
      { QVBoxLayout* vl = new QVBoxLayout;
        QPushButton* setB = new QPushButton("Set ASIC");
        QPushButton* clrB = new QPushButton("Clear ASIC");
        connect(setB, SIGNAL(clicked()), this, SLOT(set_asic0()));
        connect(clrB, SIGNAL(clicked()), this, SLOT(clear_asic0()));
        vl->addWidget(setB);
        vl->addWidget(clrB);
        hl->addLayout(vl); }
      hl->addStretch();
      hcl = hl;
      gl->addLayout(hl,1,0); }
    gl->addWidget(_display = new SectionDisplay(*hcl,*vcl),0,0);
    l->addLayout(gl); 
    l->addStretch(); }
  box->setLayout(l);
  layout->addWidget(box);

  connect(importB, SIGNAL(clicked()), this, SLOT(import_()));
  connect(exportB, SIGNAL(clicked()), this, SLOT(export_()));

  show_map(0,0);
}

void CspadGainMap::show_map(unsigned q, unsigned s)
{ 
  for(unsigned iq=0; iq<4; iq++)
    _quad[iq]->update_sections(iq==q ? (1<<s) : 0);

  _q = q;
  _s = s;

  _display->update_map(_quad[_q]->gainMap(), _q, _s);
}

void CspadGainMap::flush()
{
  _display->update_map(_quad[_q]->gainMap(), _q, _s);
}

void CspadGainMap::import_() 
{
  QString file = QFileDialog::getOpenFileName(0,"File to read from:",
                                              ".","*");
  if (file.isNull())
    return;

  FILE* f = fopen(qPrintable(file),"r");
  if (f) {
    size_t line_sz = 16*1024;
    char* line = (char *)malloc(line_sz);
    char* lptr;
    for(unsigned q=0; q<4; q++) {
      for(unsigned s=0; s<16; s++) {
        uint16_t* v = &((*_quad[q]->gainMap()->map())[0][0]);
        for(unsigned c=0; c<COLS; c++) {
          lptr = line;
          do {
            if (getline(&lptr,&line_sz,f)==-1) {
              printf("Encountered EOF at quad %d section %d column %d\n",
                     q,s,c);
              if (line) {
                free(line);
              }
              fclose(f);
              return;
            }
          } while(lptr[0]=='#');
          for(unsigned r=0; r<ROWS; r++,v++)
            if (strtoul(lptr,&lptr,0))
              *v |= (1<<s);
            else
              *v &= ~(1<<s);
        }
      }
    }
    if (line) {
      free(line);
    }
    fclose(f);

    _display->update_map(_quad[_q]->gainMap(), _q, _s);
  }
  else {
    printf("Error opening %s\n",qPrintable(file));
  }
}

void CspadGainMap::export_() 
{
  QString file = QFileDialog::getSaveFileName(0,"File to dump to:",
                                              ".","*");
  if (file.isNull())
    return;

  FILE* f = fopen(qPrintable(file),"w");
  if (f) {
    for(unsigned q=0; q<4; q++) {
      for(unsigned s=0; s<16; s++) {
        uint16_t* v = &((*_quad[q]->gainMap()->map())[0][0]);
        fprintf(f,"# Quad %d  ASIC %d  Column 0\n", q, s);
        for(unsigned c=0; c<COLS; c++) {
          for(unsigned r=0; r<ROWS; r++,v++) {
            fprintf(f," %d",((*v)>>s)&1);
          }
          fprintf(f,"\n");
        }
      }
    }
    fclose(f);
  }
  else {
    printf("Error opening %s\n",qPrintable(file));
  }
}

void CspadGainMap::set_asic0()
{
  unsigned rot = _q + rotation[_s>>1];
  uint16_t m = rot&2 ? 0x1 : 0x2;
  m <<= (2*_s);

  Pds::CsPad::CsPadGainMapCfg::GainMap& map = *_quad[_q]->gainMap()->map();
  for(unsigned col=0; col<COLS; col++)
    for(unsigned row=0; row<ROWS; row++)
      map[col][row] |= m;

  _display->update_map(_quad[_q]->gainMap(), _q, _s);
}

void CspadGainMap::set_asic1()
{
  unsigned rot = _q + rotation[_s>>1];
  uint16_t m = rot&2 ? 0x2 : 0x1;
  m <<= (2*_s);

  Pds::CsPad::CsPadGainMapCfg::GainMap& map = *_quad[_q]->gainMap()->map();
  for(unsigned col=0; col<COLS; col++)
    for(unsigned row=0; row<ROWS; row++)
      map[col][row] |= m;

  _display->update_map(_quad[_q]->gainMap(), _q, _s);
}

void CspadGainMap::clear_asic0()
{
  unsigned rot = _q + rotation[_s>>1];
  uint16_t m = rot&2 ? 0x1 : 0x2;
  m <<= (2*_s);

  Pds::CsPad::CsPadGainMapCfg::GainMap& map = *_quad[_q]->gainMap()->map();
  for(unsigned col=0; col<COLS; col++)
    for(unsigned row=0; row<ROWS; row++)
      map[col][row] &= ~m;

  _display->update_map(_quad[_q]->gainMap(), _q, _s);
}

void CspadGainMap::clear_asic1()
{
  unsigned rot = _q + rotation[_s>>1];
  uint16_t m = rot&2 ? 0x2 : 0x1;
  m <<= (2*_s);

  Pds::CsPad::CsPadGainMapCfg::GainMap& map = *_quad[_q]->gainMap()->map();
  for(unsigned col=0; col<COLS; col++)
    for(unsigned row=0; row<ROWS; row++)
      map[col][row] &= ~m;

  _display->update_map(_quad[_q]->gainMap(), _q, _s);
}
