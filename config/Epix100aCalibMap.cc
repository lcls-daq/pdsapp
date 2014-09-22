// Graphical display of whole detector
//   - sector selection shows graphical display of sector map

// Graphical display of sector gain map
//   - shows proper orientation (square window)
//   - lo value is black, hi value is white

// Mouse click toggles pixel value
// All Clear/Set buttons
// Import/Export file option buttons (space-delimited)
// Save/Close buttons

#include "pdsapp/config/Epix100aCalibMap.hh"
#include "pds/config/EpixConfigType.hh"
#include "ndarray/ndarray.h"

#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QMouseEvent>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QFileDialog>

namespace Pds_ConfigDb {

  class Epix100aCalibDisplay : public QLabel {
    public:
      Epix100aCalibDisplay(ndarray<uint8_t, 2>& m) : _map(m)
    {
        setFrameStyle(QFrame::NoFrame);
        _rows = Epix100aConfigType::CalibrationRowCountPerASIC * Epix100aConfigShadow::ASICsPerCol / 2;
        _cols = Epix100aConfigShadow::ColsPerAsic * Epix100aConfigShadow::ASICsPerRow;
        pixmap = new QPixmap(_cols, _rows);
    }

    public:
      void clear()
      {
        uint8_t* b = (uint8_t*)_map.begin();
        while (b!=_map.end()) {
          *b++ = 0;
        }
        update_map();
      }
      void update_map()
      {

        QRgb fg[4] = {0x0, 0x33ff33, 0xff3333, 0xffff33};
        QPainter painter(pixmap);
        for(unsigned row=0; row<_rows; row++) {
          for(unsigned col=0; col<_cols; col++) {
            painter.setPen(QColor(fg[_map[row][col]&3]));
            painter.drawPoint(col,row);
          }
        }

        setPixmap(*pixmap);
        update();
        setFrameStyle(QFrame::Raised /*NoFrame*/);
      }
      void import_()
      {
        QString file = QFileDialog::getOpenFileName(0,"File to read 100a pixel map from:",
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
              _map[row][col] = v&3;
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
        QString file = QFileDialog::getSaveFileName(0,"File to dump 100a pixel map to:",
            ".","*");
        if (file.isNull())
          return;

        FILE* f = fopen(qPrintable(file),"w");
        if (f) {
          for(unsigned row=0; row<_rows; row++) {
            fprintf(f,"#0  Epix100a row %u\n", row);
            for(unsigned col=0; col<_cols; col++) {
              fprintf(f," %d", _map[row][col]);
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
      ndarray<uint8_t, 2>& _map;
      unsigned              _cols;
      unsigned             _rows;
      QPixmap*            pixmap;
  };
  Epix100aCalibMapDialog::Epix100aCalibMapDialog(ndarray<uint8_t, 2>& m, QWidget *parent)
    : QDialog(parent), _map(m), _display(new Epix100aCalibDisplay(_map))
  {
    char foo[80];
    clearButton  =  new QPushButton(tr("&Clear"));
    exportButton =  new QPushButton(tr("&Export Text File for Calib/Test Map"));
    importButton =  new QPushButton(tr("&Import Text File for Calib/Test Map"));
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
    rightLayout->addWidget(clearButton);
    rightLayout->addWidget(exportButton);
    rightLayout->addWidget(importButton);
    rightLayout->addWidget(quitButton);
    rightLayout->addStretch();

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(rightLayout);
    setLayout(mainLayout);

    sprintf(foo, "Calib Mask/Test Array");
    setWindowTitle(tr(foo));
//    setFixedHeight(sizeHint().height());

    emit (dummy());

  };



  Epix100aCalibMapDialog::~Epix100aCalibMapDialog() {
    if (_display) delete _display;
  }

  void Epix100aCalibMapDialog::inQTthreadPlease()
  {
    printf("inQTthreadPlease()\n");
    _display->update_map();
  }

  void Epix100aCalibMapDialog::clearClicked()
  {
    printf("clearClicked()\n");
    _display->clear();
  }

  void Epix100aCalibMapDialog::exportClicked()
  {
    printf("exportClicked()\n");
    _display->export_();
  }

  void Epix100aCalibMapDialog::importClicked()
  {
    printf("importClicked()\n");
    _display->import_();
  }

  void Epix100aCalibMapDialog::show_map()
  {
    printf("show_map()\n");
    _display->update_map();
  }


}


