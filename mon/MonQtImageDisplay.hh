#ifndef Pds_MonQtImageDisplay_hh
#define Pds_MonQtImageDisplay_hh

#include <QtGui/QWidget>

#include <QtCore/QVector>

class QImage;
class QLabel;
class QSize;

namespace Pds {

  class MonQtImageAxis;

  class MonQtImageDisplay : public QWidget {
    Q_OBJECT
  public:
    MonQtImageDisplay (QWidget*);
    ~MonQtImageDisplay();
  public:
    void resize (const QSize&);
    void display(const QImage&);
    void axis   (const QVector<QRgb>& table, 
		 int   imin, int   imax,
		 float vmin, float vmax);
  public slots:
    void display();
  private:
    QLabel* _image_canvas;
    MonQtImageAxis* _image_axis;
    const QImage* _image;
  };
};

#endif
