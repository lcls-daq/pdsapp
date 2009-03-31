#include <errno.h>

#include "MonTabMenu.hh"
#include "MonTab.hh"
//#include "MonLayoutHints.hh"

#include "pds/mon/MonClientManager.hh"
#include "pds/mon/MonClient.hh"
#include "pds/mon/MonGroup.hh"

#include <QtGui/QScrollArea>

static const unsigned Width = 900;
static const unsigned Height = 900;
static const unsigned Step = 8;
static const unsigned Empty = 0xffffffff;

using namespace Pds;

MonTabMenu::MonTabMenu(QWidget& parent) : 
  QTabWidget(&parent),
  _maxtabs(Step),
  _tabs(new MonTab*[_maxtabs])
{
  memset(_tabs, 0, _maxtabs*sizeof(MonTab*));
}

MonTabMenu::~MonTabMenu() 
{
}

void MonTabMenu::reset(MonClient& client)
{
  //  if (_needread) readconfig(client);
  for (unsigned tabid=0; tabid<_maxtabs; tabid++) {
    if (_tabs[tabid]) { 
      removeTab(0);
      delete _tabs[tabid];
      _tabs[tabid]=0;
    }
  }
  for(unsigned tabid=0; tabid<client.cds().ngroups(); tabid++) {
    MonGroup* gr = client.cds().group(tabid);
    add(client, *gr);
  }
  //  layout();
}

void MonTabMenu::update(const MonClient& client)
{
  for (unsigned tabid=0; tabid<_maxtabs; tabid++) {
    if (_tabs[tabid]) {
      if (&_tabs[tabid]->client() == &client) {
	_tabs[tabid]->update();
      }
    }
  }
}

unsigned MonTabMenu::add(MonClient& client, 
			 MonGroup& group)
{
  unsigned tabid = create(client, group);
//   for (unsigned short e=0; e<group.nentries(); e++) {
//     MonEntry* entry = group.entry(e);
//     _tabs[tabid]->add(*entry);
//   }
  return tabid;
}

// unsigned MonTabMenu::add(MonClient& client, 
// 			 MonGroup& group,
// 			 MonEntry& entry)
// {
//   unsigned tabid = create(client, group);
//   _tabs[tabid]->add(entry);
//   return tabid;
// }

void MonTabMenu::adjust()
{
  unsigned short maxtabs = _maxtabs + Step;
  MonTab** tabs = new MonTab*[maxtabs];
  memset(tabs+_maxtabs, 0, Step*sizeof(MonTab*));
  memcpy(tabs, _tabs, _maxtabs*sizeof(MonTab*));
  delete [] _tabs;
  _tabs = tabs;
  _maxtabs = maxtabs;
}

void MonTabMenu::create(MonClient& client, MonGroup& group,
			unsigned tabid)
{
  _tabs[tabid] = new MonTab(client, group);
  QScrollArea* area = new QScrollArea(0);
  area->setWidget(_tabs[tabid]);
  area->setWidgetResizable(true);
  addTab(area,group.desc().name());
}

unsigned MonTabMenu::create(MonClient& client, MonGroup& group)
{
  int found = findtab(client, group);
  if (found >= 0) return found;
  for (unsigned tabid=0; tabid<_maxtabs; tabid++) {
    if (!_tabs[tabid]) {
      create(client, group, tabid);
      return tabid;
    }
  }
  adjust();
  create(client, group, _maxtabs);
  return _maxtabs;
}

int MonTabMenu::findtab(MonClient& client, MonGroup& group) const
{
  for (unsigned tabid=0; tabid<_maxtabs; tabid++) {
    if (_tabs[tabid]) {
      if (&_tabs[tabid]->client() == &client && 
	  &_tabs[tabid]->group () == &group) {
	return tabid;
      }
    }
  }
  return -1;
}

static const unsigned MaxLen=256;

void MonTabMenu::writeconfig(const char* configname)
{
  if (!configname) {
    printf("No config filename given\n");
    return;
  }

  FILE* fp = fopen(configname, "w");
  if (!fp) { 
    printf("Error opening %s\n",configname);
    return;
  }

  for (unsigned tabid=0; tabid<_maxtabs; tabid++) {
    if (_tabs[tabid]) {
      char tabname[MaxLen];
      snprintf(tabname, MaxLen, "%s_%s", 
	       _tabs[tabid]->client().cds().desc().name(),
	       _tabs[tabid]->group().desc().name());
      fprintf(fp,"%s\n",tabname);
      if (_tabs[tabid]->writeconfig(fp) < 0) {
	printf("*** MonTabMenu %s failed writing configuration\n",tabname);
	break;
      }
    }
  }

  fclose(fp);
}

void MonTabMenu::readconfig(MonClient& client, const char* configname)
{
  if (!configname) {
    printf("No config filename given\n");
    return;
  }

  FILE* fp = fopen(configname, "r");
  if (!fp) { 
    printf("Error opening %s\n",configname);
    return;
  }

  do {
    char line   [MaxLen];
    char entry  [MaxLen];
    if (!fgets(line, MaxLen, fp)) break;
    sscanf(line,"%s",entry);
    for (unsigned g=0; g<client.cds().ngroups(); g++) {
      MonGroup* group = const_cast<MonGroup*>(client.cds().group(g));
      char tabname[MaxLen];
      snprintf(tabname, MaxLen, "%s_%s", 
	       client.cds().desc().name(),
	       group->desc().name());
      if (strncmp(entry,tabname,MaxLen)) continue;
      int found = findtab(client, *group);
      if (found < 0) found = add(client, *group);
      if (_tabs[found]->readconfig(fp) < 0) {
	printf("*** MonTabMenu %s: failed reading configuration\n",
	       tabname);
	fclose(fp);
	return;
      }
      break;
    }
  } while(!feof(fp));

  fclose(fp);
}

