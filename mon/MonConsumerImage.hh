#ifndef Pds_MonCONSUMERImage_HH
#define Pds_MonCONSUMERImage_HH

#include "MonCanvas.hh"

#include "pdsdata/xtc/ClockTime.hh"

class QWidget;
class QwtPlot;
class QStackedWidget;

namespace Pds {

  class MonDesc;
  class MonDialog;
  class MonGroup;
  class MonDescImage;
  class MonEntryImage;
  class MonQtTH1F;
  class MonQtImage;
  class MonQtImageDisplay;
  class MonQtChart;

  class MonConsumerImage : public MonCanvas {
  public:
    MonConsumerImage(QWidget& parent,
		    const MonDesc& clientdesc,
		    const MonGroup& groupdesc,
		    const MonEntryImage& entry);
    virtual ~MonConsumerImage();

    // Implements MonConsumer from MonCanvas
    virtual void dialog();
    virtual int update();
    virtual int reset();
    virtual unsigned getplots(MonQtBase**, const char** names);
    virtual const MonQtBase* selected() const;

    void select(MonCanvas::Select);
    
    void join(MonCanvas&) {}

  private:
    const MonGroup& _group;
    MonDescImage* _desc;

    MonQtImage* _hist;

    MonQtTH1F* _hist_x;
    MonQtTH1F* _hist_y;

    MonQtChart* _chartx;
    MonQtChart* _charty;
    MonDialog*  _dialog;

    ClockTime   _time;

    QStackedWidget* _stack;
    MonQtImageDisplay* _frame;
    QwtPlot* _plot;
  };
};
#endif
