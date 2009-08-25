#ifndef Pds_MonCONSUMERWaveform_HH
#define Pds_MonCONSUMERWaveform_HH

#include "MonCanvas.hh"
#include "pdsdata/xtc/ClockTime.hh"

class QwtPlot;

namespace Pds {

  class MonDesc;
  class MonGroup;
  class MonEntryWaveform;
  class MonDescWaveform;
  class MonQtWaveform;

  class MonConsumerWaveform : public MonCanvas {
  public:
    MonConsumerWaveform(QWidget& parent,
		    const MonDesc& clientdesc,
		    const MonDesc& groupdesc,
		    const MonEntryWaveform& entry);
    virtual ~MonConsumerWaveform();

    // Implements MonConsumer from MonCanvas
    virtual void info();
    virtual void dialog();
    virtual int update();
    virtual int reset(const MonGroup& group);
    virtual unsigned getplots(MonQtBase**, const char** names);
    virtual const MonQtBase* selected() const;

    void select(MonCanvas::Select);

  private:
    MonDescWaveform* _desc;
    MonQtWaveform* _hist;
    QwtPlot* _plot;
    ClockTime _time;
  };
};

#endif
