#ifndef Pds_MonQtImage_HH
#define Pds_MonQtImage_HH

#include "MonQtBase.hh"

#include <QtCore/QVector>
#include <QtGui/QPixmap>

class QwtPlot;

namespace Pds {

  class MonQtTH1F;
  class MonDescImage;
  class MonEntryImage;
  class MonQtImageDisplay;

  class MonQtImage : public MonQtBase
  {
  public:
    MonQtImage(const char* name, const MonDescImage& desc);
    virtual ~MonQtImage();

    //    void params(const MonDescImage& desc);
    void setto(const MonEntryImage& entry);
    void setto(const MonEntryImage& curr, const MonEntryImage& prev);
    void stats();

    void projectx(MonQtTH1F* h);
    void projecty(MonQtTH1F* h);

    void attach(QwtPlot*);
    void attach(MonQtImageDisplay*);

    // Implements MonQtBase
    virtual void settings(Axis, float vmin, float vmax,
			  bool autorng, bool islog);
    virtual float min(Axis ax) const;
    virtual float max(Axis ax) const;

    void color(int color);
    int  color() const;
  private:
    int get_shift(int scale);
  private:
    float _xmin, _xmax;
    float _ymin, _ymax;
    float _zmin, _zmax;
    unsigned* _contents;
    int _shift;
    QVector<QRgb>* _color_table;
    QImage* _qimage;
    MonQtImageDisplay* _display;
  };
};
#endif
