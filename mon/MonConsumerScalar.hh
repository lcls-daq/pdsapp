#ifndef Pds_MonCONSUMERScalar_HH
#define Pds_MonCONSUMERScalar_HH

#include "MonCanvas.hh"

class QwtPlot;

namespace Pds {

  class MonDesc;
  class MonDialog;
  class MonGroup;
  class MonEntryScalar;
  class MonQtScalar;
  class MonQtChart;
  class MonStats1D;

  class MonConsumerScalar : public MonCanvas {
  public:
    MonConsumerScalar(QWidget& parent,
                      const MonDesc& clientdesc,
                      const MonGroup& group,
                      const MonEntryScalar& entry);
    virtual ~MonConsumerScalar();

    // Implements MonConsumer from MonCanvas
    virtual void info();
    virtual void dialog();
    virtual unsigned getplots(MonQtBase**, const char** names);
    virtual const MonQtBase* selected() const;
    virtual void join(MonCanvas&);
    virtual void set_plot_color(unsigned);

    void select(MonCanvas::Select);

  private:
    virtual int _update();
    virtual int _replot();
    virtual int _reset();
    virtual void _archive_mode (unsigned);

  private:
    const MonGroup& _group;
    MonEntryScalar* _last;
    MonEntryScalar* _prev;

  private:
    MonQtChart* _hist;
    MonQtChart* _since;
    MonQtChart* _diff;
    MonDialog*  _dialog;

    bool     _barchive_mode;

    QwtPlot* _plot;
  };
};

#endif
