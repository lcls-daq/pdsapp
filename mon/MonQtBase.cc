#include <time.h>

#include "MonQtBase.hh"
#include "pds/mon/MonDescEntry.hh"

#include "qwt_plot.h"

const unsigned Seconds_1970_to_1997 = 852076800;

using namespace Pds;

MonQtBase::MonQtBase(Type type, 
		     const MonDescEntry& desc, 
		     const char* name,
		     bool swapaxis) : 
  _type(type),
  _desc(&desc),
  _name(name),
  _xtitle(0),
  _ytitle(0),
  _swapaxis(swapaxis),
  _last(Seconds_1970_to_1997),
  _options(0),
  _plot(0),
  _color(0)
{
  params(desc);
}

MonQtBase::~MonQtBase() {}

void MonQtBase::params(const MonDescEntry& desc) 
{
  _desc = &desc;
  _xtitle = desc.xtitle();
  _ytitle = desc.ytitle();
  if (_type == H1) {
    if (desc.type() == MonDescEntry::TH2F) {
      _ytitle = "";
      if (_swapaxis) {
	_xtitle = desc.ytitle();
      }
    }
  } else if (_type == Chart) {
    _xtitle = "Time [seconds]";
    if (_swapaxis) {
      _ytitle = desc.xtitle();
    }
  }
}


enum Option {Log, Min, Max};

static void opt(MonQtBase::Axis ax, Option o, 
		bool isset, unsigned& options)
{
  if (isset) options |=  (1<<((o<<1)+ax));
  else       options &= ~(1<<((o<<1)+ax));
}

static bool opt(MonQtBase::Axis ax, Option o, unsigned options) 
{
  return options & (1<<((o<<1)+ax));
}

void MonQtBase::settings(Axis ax, float vmin, float vmax,
			 bool autorng, bool islog)
{
  opt(ax, Min, autorng, _options);
  opt(ax, Max, autorng, _options);
  opt(ax, Log, islog  , _options);
}

bool MonQtBase::islog(Axis ax) const {return opt(ax, Log, _options);}
bool MonQtBase::isautorng(Axis ax) const {return opt(ax, Max, _options);}

void MonQtBase::attach(QwtPlot* plot) { _plot=plot; }

void MonQtBase::apply()
{
  if (_plot) {
    QwtPlot* plot(_plot);
    attach(NULL);
    attach(plot);
    plot->replot();
  }
}
