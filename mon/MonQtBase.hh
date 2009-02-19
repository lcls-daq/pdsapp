#ifndef Pds_MonROOTBASE_HH
#define Pds_MonROOTBASE_HH

class QwtPlot;

namespace Pds {

  class MonDescEntry;

  class MonQtBase {
  public:
    enum Axis {X, Y, Z};
    enum Type {H1, H2, Prof, Chart, Image};

    MonQtBase(Type type, 
	      const MonDescEntry& desc, 
	      const char* name,
	      bool swapaxis=false);

    virtual ~MonQtBase();

    void params(const MonDescEntry& desc);
    bool swapaxis() const {return _swapaxis;}

    double last() const;
    void last(double value);

    //  Derive this to apply settings
    virtual void settings(Axis, float vmin, float vmax,
			  bool autorng, bool islog);

    bool islog(Axis ax) const;
    bool isautorng(Axis ax) const;
    virtual float min(Axis ax) const = 0;
    virtual float max(Axis ax) const = 0;

    virtual void attach(QwtPlot*);
    void apply();

  protected:
    const Type _type;
    const MonDescEntry* _desc;
    const char* _name;
    const char* _xtitle;
    const char* _ytitle;
    const bool _swapaxis;

    double _last;
    unsigned _options;
    QwtPlot* _plot;
  };

  inline double MonQtBase::last() const {return _last;}
  inline void MonQtBase::last(double value) {_last = value;}
};

#endif
