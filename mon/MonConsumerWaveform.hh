#ifndef Pds_MonCONSUMERWaveform_HH
#define Pds_MonCONSUMERWaveform_HH

#include "MonCanvas.hh"
#include "pdsdata/xtc/ClockTime.hh"

class QwtPlot;

namespace Pds {

  class MonDesc;
  class MonDialog;
  class MonGroup;
  class MonEntryWaveform;
  class MonDescWaveform;
  class MonQtWaveform;

  class MonConsumerWaveform : public MonCanvas {
  public:
    MonConsumerWaveform(QWidget& parent,
		    const MonDesc& clientdesc,
		    const MonGroup& groupdesc,
		    const MonEntryWaveform& entry);
    virtual ~MonConsumerWaveform();

    // Implements MonConsumer from MonCanvas
    virtual void info();
    virtual void dialog();
    virtual unsigned getplots(MonQtBase**, const char** names);
    virtual const MonQtBase* selected() const;

    void select(MonCanvas::Select);
    void join  (MonCanvas&) {}

  private:
    virtual int _update();
    virtual int _reset();

  private:
    const MonGroup& _group;
    MonDescWaveform* _desc;
    MonQtWaveform* _hist;
    MonDialog*  _dialog;
    QwtPlot* _plot;
    ClockTime _time;
  };
};

#endif
