// Graphical display of whole detector

// Graphical display of sector gain mask/test
//   - shows proper orientation (square window)
//   - lo value is black
// Import/Export file option buttons (space-delimited)
// Save/Close buttons

#include "pdsapp/config/Epix10kaPixelMap.hh"
#include "pds/config/EpixConfigType.hh"
#include "ndarray/ndarray.h"

#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QStaticText>
#include <QtGui/QMouseEvent>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QFileDialog>

static const int _height  = 31;
static const int _width   = 30;
static const int asic_xo[] = { 4, 4, _width+5, _width+5 };
static const int asic_yo[] = { 11, _height+12, _height+12, 11 }; 

namespace Pds_ConfigDb {

  class Epix10kaAsicLayout : public QLabel {
  public:
    Epix10kaAsicLayout() : QLabel(0)
    {
      QPixmap* image = new QPixmap(70, 75);
      QRgb bg = QPalette().color(QPalette::Window).rgb();
      image->fill(bg);
      QPainter painter(image);
      for(unsigned j=0; j<4; j++) {
        QRgb fg = bg;
        painter.setBrush(QColor(fg));
        int xo = asic_xo[j%4];
        int yo = asic_yo[j%4];
        painter.drawRect(xo, yo, _width, _height);
        painter.setBrush(QColor(qRgb(0,0,0)));
        QString label = QString("%1").arg(j);
        painter.drawStaticText(xo+_width/2-4,yo+_height/2-6,QStaticText(label));
      }
      QTransform transform(QTransform().rotate(180));
      setPixmap(image->transformed(transform));
    }
  };

  class Epix10kaPixelDisplay : public QLabel {
    public:
      Epix10kaPixelDisplay(ndarray<uint16_t, 2>& m) : _map(m)
    {
        setFrameStyle(QFrame::NoFrame);
        _rows = m.shape()[0]; // Epix10kaConfigShadow::RowsPerAsic * Epix10kaConfigShadow::ASICsPerCol;
        _cols = m.shape()[1]; // Epix10kaConfigShadow::ColsPerAsic * Epix10kaConfigShadow::ASICsPerRow;
        pixmap = new QPixmap(_cols, _rows);
    }

    public:
      void clear()
      {
        uint16_t* b = (uint16_t*)_map.begin();
        while (b!=_map.end()) {
          *b++ = 0;
        }
        update_map();
      }
      void update_map()
      {
        QRgb fg[16] = {0x800000, 0x881818, 0x882838, 0x8f3840,
                       0x8f4848, 0x8f5858, 0x8f6868, 0x8f7878,
                       0x8f8868, 0x9f986f, 0xafa85f, 0xbfb84f,
                       0xcfc83f, 0xdfd82f, 0xefe818, 0xffff0f};
        QPainter painter(pixmap);
        for(unsigned row=0; row<_rows; row++) {
          for(unsigned col=0; col<_cols; col++) {
            painter.setPen(QColor(fg[_map(row,col)&15]));
            painter.drawPoint(col,row);
          }
        }

        setPixmap(*pixmap);
        update();
        setFrameStyle(QFrame::Raised /*NoFrame*/);
      }
      void import_()
      {
        QString file = QFileDialog::getOpenFileName(0,"File to read 10ka pixel map from:",
            ".","*");
        if (file.isNull())
          return;

        FILE* f = fopen(qPrintable(file),"r");
        if (f) {
          unsigned v = 0;
          size_t line_sz = 16*1024;
          char* line = (char *)malloc(line_sz);
          char* lptr;
          for(unsigned row=0; row<_rows; row++) {
            lptr = line;
            do {
              if (getline(&lptr,&line_sz,f)==-1) {
                printf("Encountered EOF at row %d\n", row);
                if (line) {
                  free(line);
                }
                fclose(f);
                return;
              }
            } while(lptr[0]=='#');
            for(unsigned col=0; col<_cols; col++) {
              v = strtoul(lptr,&lptr,0);
              _map(row,col) = v&15;
            }
          }
          if (line) {
            free(line);
          }
          fclose(f);
          update_map();
        }
        else {
          printf("Error opening %s\n",qPrintable(file));
        }
      }
      void export_()
      {
        QString file = QFileDialog::getSaveFileName(0,"File to dump 10ka pixel map to:",
            ".","*");
        if (file.isNull())
          return;

        FILE* f = fopen(qPrintable(file),"w");
        if (f) {
          for(unsigned row=0; row<_rows; row++) {
            fprintf(f,"#  Epix10ka row %u\n", row);
            for(unsigned col=0; col<_cols; col++) {
              fprintf(f," %d", _map(row,col));
            }
            fprintf(f,"\n");
          }
          fclose(f);
        }
        else {
          printf("Error opening %s\n",qPrintable(file));
        }
      }
    private:
      ndarray<uint16_t, 2>& _map;
      unsigned              _cols;
      unsigned              _rows;
      QPixmap*              pixmap;
  };
  Epix10kaPixelMapDialog::Epix10kaPixelMapDialog(ndarray<uint16_t, 2>& m, QWidget *parent)
    : QDialog(parent), _map(m), _display(new Epix10kaPixelDisplay(_map))
  {
    char foo[80];
    clearButton  =  new QPushButton(tr("&Clear"));
    exportButton =  new QPushButton(tr("&Export Text File for Pixel/Test Map"));
    importButton =  new QPushButton(tr("&Import Text File for Pixel/Test Map"));
    quitButton   =  new QPushButton(tr("&Return"));

    connect(clearButton,   SIGNAL(clicked()), this, SLOT(clearClicked()));
    connect(exportButton,  SIGNAL(clicked()), this, SLOT(exportClicked()));
    connect(importButton,  SIGNAL(clicked()), this, SLOT(importClicked()));
    connect(quitButton,    SIGNAL(clicked()), this, SLOT(reject()));
    connect(this,          SIGNAL(dummy()),   this, SLOT(inQTthreadPlease()));

    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->addWidget(_display);

    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->addStretch();
    { QHBoxLayout* hl = new QHBoxLayout;
      hl->addStretch();
      hl->addWidget(new Epix10kaAsicLayout);
      hl->addStretch();
      rightLayout->addLayout(hl); }
    rightLayout->addStretch();
    rightLayout->addWidget(clearButton);
    rightLayout->addWidget(exportButton);
    rightLayout->addWidget(importButton);
    rightLayout->addWidget(quitButton);
    rightLayout->addStretch();

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(rightLayout);
    setLayout(mainLayout);

    sprintf(foo, "Pixel Mask/Test Array");
    setWindowTitle(tr(foo));
//    setFixedHeight(sizeHint().height());

    emit (dummy());

  };



  Epix10kaPixelMapDialog::~Epix10kaPixelMapDialog() {
    if (_display) delete _display;
  }

  void Epix10kaPixelMapDialog::inQTthreadPlease()
  {
    _display->update_map();
  }

  void Epix10kaPixelMapDialog::clearClicked()
  {
    _display->clear();
  }

  void Epix10kaPixelMapDialog::exportClicked()
  {
    _display->export_();
  }

  void Epix10kaPixelMapDialog::importClicked()
  {
    _display->import_();
  }

  void Epix10kaPixelMapDialog::show_map()
  {
    _display->update_map();
  }

}


