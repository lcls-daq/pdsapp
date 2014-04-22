#ifndef Pds_MonCONSUMERTH2F_HH
#define Pds_MonCONSUMERTH2F_HH

#include "MonCanvas.hh"

class QWidget;
class QwtPlot;

namespace Pds {

  class MonDesc;
  class MonDialog;
  class MonGroup;
  class MonEntryTH2F;
  class MonQtTH1F;
  class MonQtTH2F;
  class MonQtChart;

  class MonConsumerTH2F : public MonCanvas {
  public:
    MonConsumerTH2F(QWidget& parent,
		    const MonDesc& clientdesc,
		    const MonGroup&,
		    const MonEntryTH2F& entry);
    virtual ~MonConsumerTH2F();

    // Implements MonConsumer from MonCanvas
    virtual void dialog();
    virtual unsigned getplots(MonQtBase**, const char** names);
    virtual const MonQtBase* selected() const;

    void select(MonCanvas::Select);
    void join  (MonCanvas&);

  private:
    virtual int _update();
    virtual int _reset();

  private:
    const MonGroup& _group;
    MonEntryTH2F* _last;

  private:
    MonQtTH2F* _hist;
    MonQtTH2F* _diff;

    MonQtTH1F* _hist_x;
    MonQtTH1F* _hist_y;
    MonQtTH1F* _diff_x;
    MonQtTH1F* _diff_y;

    MonQtChart* _chartx;
    MonQtChart* _charty;

    MonDialog*  _dialog;

    QwtPlot* _plot;
  };
};
#endif
