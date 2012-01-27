// Graphical display of whole detector
//   - sector selection shows graphical display of sector map

// Graphical display of sector gain map
//   - shows proper orientation (square window)
//   - lo value is black, hi value is white

// Mouse click toggles pixel value
// All Clear/Set buttons
// Import/Export file option buttons (space-delimited)
// Save/Close buttons

#include "pdsapp/config/Cspad2x2GainMap.hh"
#include "pds/config/CsPad2x2ConfigType.hh"

#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QMouseEvent>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QFileDialog>

static const unsigned COLS=Pds::CsPad2x2::ColumnsPerASIC; // 185
static const unsigned ROWS=Pds::CsPad2x2::MaxRowsPerASIC; // 194
static const unsigned SPACE = 10;

namespace Pds_ConfigDb {

  class Quad2x2GainMap : public QLabel {
  public:
    Quad2x2GainMap(Cspad2x2GainMap& parent) :
      _parent(parent), _quad(0),
      _gainMap(new Pds::CsPad2x2::CsPad2x2GainMapCfg) {
      setFrameStyle (QFrame::NoFrame);
    }
    ~Quad2x2GainMap() { delete _gainMap; }
  public:
    Pds::CsPad2x2::CsPad2x2GainMapCfg* gainMap() const { return _gainMap; }
  private:
    Cspad2x2GainMap& _parent;
    unsigned      _quad;
    Pds::CsPad2x2::CsPad2x2GainMapCfg* _gainMap;
  };

  class SectionDisplay2x2 : public QLabel {
  public:
    SectionDisplay2x2() : QLabel(0)
    {
      setFrameStyle(QFrame::NoFrame);
    }
  public:
    void update_map(Pds::CsPad2x2::CsPad2x2GainMapCfg* map)
    {
      const unsigned SIZE = ROWS*2 + 8;
      QPixmap* pixmap = new QPixmap(COLS*2+8+SPACE,SIZE);

      QRgb bg = QPalette().color(QPalette::Window).rgb();
      pixmap->fill(bg);

      for (unsigned section=0; section < 2; section++) {
        const unsigned col0 = 4 + section*(COLS + SPACE);
        const unsigned row0 = SIZE/2+1+ROWS;
        const unsigned row1 = SIZE/2-1;
        unsigned       asic0 =(section<<1);
        unsigned       asic1 =(section<<1)+1;
        QRgb fg;
        QPainter painter(pixmap);
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
      }

      setPixmap(*pixmap);
      setFrameStyle(QFrame::Raised /*NoFrame*/);
    }
  };
}

using namespace Pds_ConfigDb;

Cspad2x2GainMap::Cspad2x2GainMap() : _display(0) {}

Cspad2x2GainMap::~Cspad2x2GainMap() { if (_display) delete _display; delete _quad; }

void Cspad2x2GainMap::insert(Pds::LinkedList<Parameter>& pList) {
}

Pds::CsPad2x2::CsPad2x2GainMapCfg* Cspad2x2GainMap::quad() {
  return _quad[0]->gainMap();
}

void Cspad2x2GainMap::initialize(QWidget* parent, QVBoxLayout* layout)
{
  QGroupBox* box = new QGroupBox("140K GainMap");
  QVBoxLayout* l = new QVBoxLayout;
  //  Add Import,Export buttons
  QPushButton* importB = new QPushButton("Import Text File for 140K GainMap");
  QPushButton* exportB = new QPushButton("Export Text File for 140K GainMap");
  l->addWidget(importB);
  l->addWidget(exportB);
//  QGridLayout* gl = new QGridLayout;
//  gl->addWidget(_quad[0] = new Quad2x2GainMap(*this),0,0,::Qt::AlignVCenter|::Qt::AlignHCenter);
//  l->addLayout(gl);
  _quad[0] = new Quad2x2GainMap(*this);
  l->addWidget(_display = new SectionDisplay2x2);
  box->setLayout(l);
  layout->addWidget(box);

  connect(importB, SIGNAL(clicked()), this, SLOT(import_()));
  connect(exportB, SIGNAL(clicked()), this, SLOT(export_()));

  show_map(0);
}

void Cspad2x2GainMap::show_map(unsigned s)
{ 
  _display->update_map(_quad[0]->gainMap());
}

void Cspad2x2GainMap::flush()
{
  _display->update_map(_quad[0]->gainMap());
}

void Cspad2x2GainMap::import_()
{
  QString file = QFileDialog::getOpenFileName(0,"File to read 140K gain map from:",
                                              ".","*");
  if (file.isNull())
    return;

  FILE* f = fopen(qPrintable(file),"r");
  if (f) {
    size_t line_sz = 16*1024;
    char* line = (char *)malloc(line_sz);
    char* lptr;
    for(unsigned s=0; s<4; s++) {
      uint16_t* v = &((*_quad[0]->gainMap()->map())[0][0]);
      for(unsigned c=0; c<COLS; c++) {
        lptr = line;
        do {
          if (getline(&lptr,&line_sz,f)==-1) {
            printf("Encountered EOF at section %d column %d\n", s,c);
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
    if (line) {
      free(line);
    }
    fclose(f);
    _display->update_map(_quad[0]->gainMap());
  }
  else {
    printf("Error opening %s\n",qPrintable(file));
  }
}

void Cspad2x2GainMap::export_()
{
  QString file = QFileDialog::getSaveFileName(0,"File to dump 140K gain map to:",
                                              ".","*");
  if (file.isNull())
    return;

  FILE* f = fopen(qPrintable(file),"w");
  if (f) {
    for(unsigned s=0; s<4; s++) {
      uint16_t* v = &((*_quad[0]->gainMap()->map())[0][0]);
      fprintf(f,"#0  ASIC %d  Column 0\n", s);
      for(unsigned c=0; c<COLS; c++) {
        for(unsigned r=0; r<ROWS; r++,v++) {
          fprintf(f," %d",((*v)>>s)&1);
        }
        fprintf(f,"\n");
      }
    }
    fclose(f);
  }
  else {
    printf("Error opening %s\n",qPrintable(file));
  }
}

