#ifndef Pds_MonCONSUMERPROF_HH
#define Pds_MonCONSUMERPROF_HH

#include "MonCanvas.hh"

class QWidget;
class QwtPlot;

namespace Pds {

  class MonDesc;
  class MonDialog;
  class MonGroup;
  class MonEntryProf;
  class MonQtProf;
  class MonQtChart;

  class MonConsumerProf : public MonCanvas {
  public:
    MonConsumerProf(QWidget& parent,
		    const MonDesc& clientdesc,
		    const MonGroup& groupdesc,
		    const MonEntryProf& entry);
    virtual ~MonConsumerProf();

    // Implements MonConsumer from MonCanvas
    virtual void dialog();
    virtual int update();
    virtual int reset();
    virtual unsigned getplots(MonQtBase**, const char** names);
    virtual const MonQtBase* selected() const;

    void select(MonCanvas::Select);
    void join  (MonCanvas&) {}
  private:
    const MonGroup& _group;
    MonEntryProf* _last;
    MonEntryProf* _prev;

  private:
    MonQtProf* _hist;
    MonQtProf* _since;
    MonQtProf* _diff;
    MonQtChart* _chart;
    MonDialog*  _dialog;

    QwtPlot* _plot;
  };
};
#endif
