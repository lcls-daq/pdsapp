#include "MonConsumerFactory.hh"
#include "MonConsumerTH1F.hh"
#include "MonConsumerTH2F.hh"
#include "MonConsumerProf.hh"
#include "MonConsumerImage.hh"

#include "pds/mon/MonDescEntry.hh"
#include "pds/mon/MonEntry.hh"

using namespace Pds;

MonCanvas* MonConsumerFactory::create(QWidget& parent,
				      const MonDesc& clientdesc,
				      const MonDesc& groupdesc,
				      const MonEntry& entry)
{
  MonCanvas* canvas = 0;
  switch (entry.desc().type()) {
  case MonDescEntry::TH1F:
    {
      const MonEntryTH1F& e = (const MonEntryTH1F&)entry;
      canvas = new MonConsumerTH1F(parent, clientdesc, groupdesc, e);
    }
    break;
  case MonDescEntry::TH2F:
    {
      const MonEntryTH2F& e = (const MonEntryTH2F&)entry;
      canvas = new MonConsumerTH2F(parent, clientdesc, groupdesc, e);
    }
    break;
   case MonDescEntry::Prof:
     {
       const MonEntryProf& e = (const MonEntryProf&)entry;
       canvas = new MonConsumerProf(parent, clientdesc, groupdesc, e);
     }
    break;
   case MonDescEntry::Image:
     {
       const MonEntryImage& e = (const MonEntryImage&)entry;
       canvas = new MonConsumerImage(parent, clientdesc, groupdesc, e);
     }
    break;
  }
  return canvas;
}
