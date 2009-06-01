#include "MonQtImageDisplay.hh"

#include "MonQtImage.hh"

#include <QtGui/QLabel>
#include <QtGui/QPixmap>
#include <QtGui/QWidget>
#include <QtGui/QPainter>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>

namespace Pds {
  class MonQtImageAxis : public QWidget {
  public:
    enum { Width=10, Height = 256 };
    MonQtImageAxis(QWidget* parent) :
      QWidget(parent),
      _axis_image(new QImage(Width, Height, QImage::Format_Indexed8))
    {
      QGridLayout* layout = new QGridLayout(this);
      layout->addWidget(_axis_max    = new QLabel("zmax" ,this),0,1,Qt::AlignLeft | Qt::AlignTop);
      layout->addWidget(_axis_canvas = new QLabel("zaxis",this),0,0,2,1,Qt::AlignTop);
      layout->addWidget(_axis_min    = new QLabel("zmin" ,this),1,1,Qt::AlignLeft | Qt::AlignBottom);
      layout->setColumnStretch(2,1);
      layout->setVerticalSpacing(0);
      setLayout(layout);
    }
    MonQtImageAxis() { delete _axis_image; }

    void resize(const QSize& size) 
    {
//       printf("resizing from %d,%d to %d,%d [%g,%g]\n",
// 	     _axis_image->size().width(),
// 	     _axis_image->size().height(),
// 	     size.width(),
// 	     size.height(),
// 	     _axis_image->dotsPerMeterX(),
// 	     _axis_image->dotsPerMeterY());
      QSize sz(Width,size.height());
      *_axis_image = _axis_image->scaled(sz);
    }

    void axis(const QVector<QRgb>& table, 
	      int   imin, int   imax,
	      float vmin, float vmax) 
    {
      _axis_min->setText(QString::number(vmin,'g',4));
      _axis_max->setText(QString::number(vmax,'g',4));
    
      const QSize&   sz  = _axis_image->size();
      unsigned char* dst = _axis_image->bits();
      double         dv  = double(imax-imin)/double(sz.height());
      double          v  = imax;
      for(int k=0; k<sz.height(); k++) {
	unsigned char uv(v);
	for(int j=0; j<sz.width(); j++)
	  *dst++ = uv;
	v -= dv;
      }
      _axis_image->setColorTable(table);
    }

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
  layout->addWidget(_image_canvas = new QLabel(this),0);
  layout->addWidget(_image_axis   = new MonQtImageAxis(this),0);
  layout->setSizeConstraint(QLayout::SetFixedSize);
  setLayout(layout);
}


MonQtImageDisplay::~MonQtImageDisplay()
{
}

void MonQtImageDisplay::resize(const QSize& size)
{
  _image_canvas->resize(size);
  _image_axis  ->resize(size);
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
{ _image_axis->axis(table,imin,imax,vmin,vmax); }

const QImage* MonQtImageDisplay::image() const
{
  return _image;
}
