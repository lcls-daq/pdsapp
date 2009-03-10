#include "MonQtImageDisplay.hh"

#include "MonQtImage.hh"

#include <QtGui/QLabel>
#include <QtGui/QPixmap>
#include <QtGui/QWidget>
#include <QtGui/QPainter>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

namespace Pds {
  class MonQtImageAxis : public QWidget {
  public:
    enum { Width=10, Height = 256 };
    MonQtImageAxis(QWidget* parent) :
      QWidget(parent),
      _axis_image(new QImage(Width, Height, QImage::Format_Indexed8))
    {
      QVBoxLayout* layout = new QVBoxLayout(this);
      layout->addWidget(_axis_max    = new QLabel("zmax" ,this),0,Qt::AlignBottom);
      layout->addWidget(_axis_canvas = new QLabel("zaxis",this),0);
      layout->addWidget(_axis_min    = new QLabel("zmin" ,this),1,Qt::AlignTop);
      setLayout(layout);
    }
    MonQtImageAxis() { delete _axis_image; }

    QLabel* _axis_canvas;
    QLabel* _axis_min;
    QLabel* _axis_max;
    QImage* _axis_image;
  };
};

using namespace Pds;

MonQtImageDisplay::MonQtImageDisplay(QWidget* parent) :
  QWidget(parent)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->addWidget(_image_canvas = new QLabel(this),0,Qt::AlignTop);
  layout->addWidget(_image_axis   = new MonQtImageAxis(this),0,Qt::AlignTop);
  setLayout(layout);
}


MonQtImageDisplay::~MonQtImageDisplay()
{
}

void MonQtImageDisplay::resize(const QSize& size)
{
  _image_canvas->resize(size);
}

void MonQtImageDisplay::display(const QImage& image)
{
  _image = &image;
}

//
//  This must execute in the QApp thread, i.e. in a "slot".
//
void MonQtImageDisplay::display()
{
  _image_canvas->setPixmap(QPixmap::fromImage(*_image));
  _image_axis->_axis_canvas->setPixmap(QPixmap::fromImage(*_image_axis->_axis_image));
}

void MonQtImageDisplay::axis(const QVector<QRgb>& table, 
			     int   imin, int   imax,
			     float vmin, float vmax)
{
  _image_axis->_axis_min->setText(QString::number(vmin,'g',4));
  _image_axis->_axis_max->setText(QString::number(vmax,'g',4));

  unsigned char* dst = _image_axis->_axis_image->bits();
  float dv = float(imax-imin)/float(MonQtImageAxis::Height);
  for(unsigned k=0; k<MonQtImageAxis::Height; k++) {
    unsigned char v = imax - k*dv;
    for(unsigned j=0; j<MonQtImageAxis::Width; j++)
      *dst++ = v;
  }
  _image_axis->_axis_image->setColorTable(table);
}

const QImage* MonQtImageDisplay::image() const
{
  return _image;
}
