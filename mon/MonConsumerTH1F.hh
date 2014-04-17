#ifndef Pds_MonCONSUMERTH1F_HH
#define Pds_MonCONSUMERTH1F_HH

#include "MonCanvas.hh"

class QwtPlot;

namespace Pds {

  class MonDesc;
  class MonDialog;
  class MonGroup;
  class MonEntryTH1F;
  class MonQtTH1F;
  class MonQtChart;
  class MonStats1D;

  class MonConsumerTH1F : public MonCanvas {
  public:
    MonConsumerTH1F(QWidget& parent,
		    const MonDesc& clientdesc,
		    const MonGroup& group,
		    const MonEntryTH1F& entry);
    virtual ~MonConsumerTH1F();

    // Implements MonConsumer from MonCanvas
    virtual void info();
    virtual void dialog();
    virtual int update();
    virtual int replot();
    virtual int reset();
    virtual unsigned getplots(MonQtBase**, const char** names);
    virtual const MonQtBase* selected() const;
    virtual void join(MonCanvas&);
    virtual void set_plot_color(unsigned);

    void select(MonCanvas::Select);

    void archive_mode (unsigned);

  private:
    const MonGroup& _group;
    MonEntryTH1F* _last;
    MonEntryTH1F* _prev;

  private:
    MonQtTH1F* _hist;
    MonQtTH1F* _since;
    MonQtTH1F* _diff;
    MonQtChart* _chart;
    MonStats1D* _last_stats;
    MonDialog*  _dialog;

    bool     _archive_mode;

    QwtPlot* _plot;
  };
};

#endif
