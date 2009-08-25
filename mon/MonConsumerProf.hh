#ifndef Pds_MonCONSUMERPROF_HH
#define Pds_MonCONSUMERPROF_HH

#include "MonCanvas.hh"

class QWidget;
class QwtPlot;

namespace Pds {

  class MonDesc;
  class MonGroup;
  class MonEntryProf;
  class MonQtProf;
  class MonQtChart;

  class MonConsumerProf : public MonCanvas {
  public:
    MonConsumerProf(QWidget& parent,
		    const MonDesc& clientdesc,
		    const MonDesc& groupdesc,
		    const MonEntryProf& entry);
    virtual ~MonConsumerProf();

    // Implements MonConsumer from MonCanvas
    virtual void dialog();
    virtual int update();
    virtual int reset(const MonGroup& group);
    virtual unsigned getplots(MonQtBase**, const char** names);
    virtual const MonQtBase* selected() const;

    void select(MonCanvas::Select);

  private:
    MonEntryProf* _last;
    MonEntryProf* _prev;

  private:
    MonQtProf* _hist;
    MonQtProf* _since;
    MonQtProf* _diff;
    MonQtChart* _chart;

    QwtPlot* _plot;
  };
};
#endif
