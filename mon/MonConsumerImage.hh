#ifndef Pds_MonCONSUMERImage_HH
#define Pds_MonCONSUMERImage_HH

#include "MonCanvas.hh"

class QWidget;
class QwtPlot;
class QStackedWidget;

namespace Pds {

  class MonDesc;
  class MonGroup;
  class MonEntryImage;
  class MonQtTH1F;
  class MonQtImage;
  class MonQtImageDisplay;
  class MonQtChart;

  class MonConsumerImage : public MonCanvas {
  public:
    MonConsumerImage(QWidget& parent,
		    const MonDesc& clientdesc,
		    const MonDesc& groupdesc,
		    const MonEntryImage& entry);
    virtual ~MonConsumerImage();

    // Implements MonConsumer from MonCanvas
    virtual void dialog();
    virtual int update();
    virtual int reset(const MonGroup& group);
    virtual unsigned getplots(MonQtBase**, const char** names);

    void select(MonCanvas::Select);

  private:
    MonEntryImage* _last;
    MonEntryImage* _prev;

  private:
    MonQtImage* _hist;
    MonQtImage* _since;
    MonQtImage* _diff;

    MonQtTH1F* _hist_x;
    MonQtTH1F* _hist_y;
    MonQtTH1F* _diff_x;
    MonQtTH1F* _diff_y;

    MonQtChart* _chartx;
    MonQtChart* _charty;

    QStackedWidget* _stack;
    MonQtImageDisplay* _frame;
    QwtPlot* _plot;
  };
};
#endif
