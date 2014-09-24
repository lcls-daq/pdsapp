#include "pdsapp/control/PVManager.hh"
#include "pdsapp/control/PVMonitor.hh"
#include "pdsapp/control/PVControl.hh"

using namespace Pds;

PVManager::PVManager(PVRunnable& runnable) : 
  _pvrunnable(runnable),
  _pvmonitor (new PVMonitor(*this)),
  _pvcontrol (new PVControl(*this))
{
}

PVManager::~PVManager() 
{
  delete _pvmonitor;
  delete _pvcontrol;
}

void PVManager::runnable_change(bool r)
{
  _pvrunnable.runnable_change( _pvmonitor->runnable() && _pvcontrol->runnable() );
}

void PVManager::configure      (const ControlConfigType& tc)
{
  _pvmonitor->configure(tc);
  _pvcontrol->configure(tc);
}

void PVManager::unconfigure    ()
{
  _pvmonitor->unconfigure();
  _pvcontrol->unconfigure();
}
