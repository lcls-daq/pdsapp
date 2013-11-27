#include "pdsapp/config/EvrEventDefault.hh"

using namespace Pds_ConfigDb;

EvrEventDefault::EvrEventDefault() {}

EvrEventDefault::~EvrEventDefault() {}

bool EvrEventDefault::pull(const EventCodeType& e)
{
  if (e.isReadout() || e.isCommand() || e.isLatch()) 
    return false;

  if (e.reportDelay()!=0 || e.reportWidth()!=1)
    return false;

  if (strlen(e.desc())!=0)
    return false;

  _codes.push_back(e.code());
  return true;
}

void EvrEventDefault::push(EventCodeType*& p) const
{
  for(std::list<unsigned>::const_iterator it=_codes.begin();
      it!=_codes.end(); it++)
    *new(p++) EventCodeType(*it,
			    false, false, false,
			    0, 1,
			    0, 0, 0,
			    "",
			    0);
}

void EvrEventDefault::flush()
{
  _codes.sort();

  QString t("Default recording: ");
  std::list<unsigned>::iterator it=_codes.begin();
  while(it!=_codes.end()) {
    unsigned v=*it++;
    t.append(QString::number(v));
    if (it==_codes.end()) break;
    if (*it != v+1)
      t.append(",");
    else {
      t.append("-");
      std::list<unsigned>::iterator s=it; 
      while(s!=_codes.end() && ++v==*s) s++;
      it=--s;
    }
  }
  setText(t);
}

void EvrEventDefault::clear()
{
  _codes.clear();
}
