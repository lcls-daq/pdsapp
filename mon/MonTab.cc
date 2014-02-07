#include "MonTab.hh"
#include "MonCanvas.hh"
#include "MonConsumerFactory.hh"

#include "pds/mon/MonClient.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonDescEntry.hh"

#include <QtGui/QHBoxLayout>

static const unsigned Step = 64;

using namespace Pds;

MonTab::MonTab(MonClient& client,
	       const MonGroup& group) :
  QWidget(0),
  _client(client),
  _used(0),
  _canvases(new MonCanvas*[group.nentries()]),
  _group(&group)
{
  //
  //  The configuration file could help us here
  //
  QGridLayout* layout = new QGridLayout(this);
  unsigned n = group.nentries();
  unsigned rows = (n+2)/3;
  unsigned columns = rows ? (n+rows-1)/rows : 0;

  for(_used = 0; _used < group.nentries(); _used++) {
    const MonEntry& entry = *group.entry(_used);
    MonCanvas* canvas = MonConsumerFactory::create(*this,
						   client.cds().desc(),
						   group.desc(),
						   entry);
    
    _canvases[_used] = canvas;
    _client.use(entry.desc().signature());
    layout->addWidget(canvas, _used/columns, _used%columns);
  }
  setLayout(layout);
}

MonTab::~MonTab() 
{
}

QSize MonTab::sizeHint() const
{
  return QSize(500,480);
}

int MonTab::reset() 
{
  /*
  const MonGroup* newgroup = _client.cds().group(_groupname);
  if (newgroup) {
    _group = newgroup;
    for (int short e=_used-1; e>=0; e--) {
      MonCanvas* canvas = _canvases[e];
      if (!canvas->reset(*_group)) {
	destroy(e);
      } else {
	_client.use(canvas->entry().desc().signature());
      }
    }
    layout();
    return 1;
  }
  _group = 0;
  */
  return 0;
}

// void MonTab::add(MonEntry& entry) 
// {
//   /*
//   if (getcanvas(entry.desc().name())) return;
//   if (_used == _maxcanvases) adjust();
//   MonCanvas* canvas = 
//     MonConsumerFactory::create(*this, _client.cds().desc(), 
// 				   _group->desc(), entry);
//   AddFrame(canvas);
//   _client.use(entry.desc().signature());
//   _canvases[_used] = canvas;
//   _used++;
//   */
// }

void MonTab::current(bool iscurrent) {_iscurrent = iscurrent;}

void MonTab::update() 
{
  //  if (!_iscurrent) return;

  for (unsigned short e=0; e<_used; e++)
    _canvases[e]->update();

  //  QWidget::update();
}

/*
void MonTab::destroy(int short which) 
{
  MonCanvas* canvas = _canvases[which];
  canvas->DestroyWindow();
  RemoveFrame(canvas);
  delete canvas;
  int short howmany = _used-which-1;
  if (howmany > 0) {
    MonCanvas** where = _canvases+which;
    memmove(where, where+1, howmany*sizeof(MonCanvas*));
  }
  _used--;
}

void MonTab::adjust()
{
  unsigned short maxcanvases = _maxcanvases + Step;
  MonCanvas** canvases = new MonCanvas*[maxcanvases];
  memcpy(canvases, _canvases, _used*sizeof(MonCanvas*));
  delete [] _canvases;
  _canvases = canvases;
  _maxcanvases = maxcanvases;
}
*/

const MonClient& MonTab::client() const {return _client;}
const MonGroup& MonTab::group() const {return *_group;}
//const char* MonTab::name() const {return _groupname;}

MonCanvas* MonTab::getcanvas(const char* name)
{
  for (unsigned short e=0; e<_used; e++) {
    if (strcmp(_canvases[e]->_entry->desc().name(), name) == 0) {
      return _canvases[e];
    }
  }
  return 0;
}

int MonTab::writeconfig(FILE* fp)
{
  fprintf(fp, "Entries %d\n", _used);
  for (unsigned short e=0; e<_used; e++) {
    fprintf(fp, "%s\n", _canvases[e]->_entry->desc().name());
    if (_canvases[e]->writeconfig(fp) < 0) return -1;
  }
  return 0;
}

int MonTab::readconfig(FILE* fp)
{
  unsigned MaxLine = 256;
  char line[MaxLine], eat[MaxLine], entryname[MaxLine];
  unsigned nentries;

  if (!fgets(line, MaxLine, fp)) return -1;
  if (sscanf(line, "%s %d", eat, &nentries)!=2) {
    printf("Error parsing line: %s\n",line);
    return -1;
  }

  for (unsigned c=0; c<nentries; c++) {
    if (!fgets(entryname, MaxLine, fp)) return -1;
    else entryname[strlen(entryname)-1] = 0;
    MonCanvas* canvas = getcanvas(entryname);
    if (canvas) {
      if (canvas->readconfig(fp,_color) < 0) return -1;
    }
  }
  return 0;
}

