#ifndef Pds_MonTABMENU_HH
#define Pds_MonTABMENU_HH

#include <QtGui/QTabWidget>

#include "pdsdata/xtc/Level.hh"

namespace Pds {

  class MonTab;
  class MonClient;
  class MonGroup;
  class MonEntry;

  class MonTabMenu : public QTabWidget {
  public:
    MonTabMenu(QWidget& parent);
    virtual ~MonTabMenu();

    void reset(MonClient& client);
    void update(const MonClient& client);
    unsigned add(MonClient& client, MonGroup& group);
//     unsigned add(MonClient& client, MonGroup& group, MonEntry& entry);
    //    void layout();

    void configname(const char* name);
    void writeconfig(const char*);
    void readconfig(MonClient& client, const char*);

  private:
    void adjust();
    void create(MonClient& client, MonGroup& group, unsigned id);
    unsigned create(MonClient& client, MonGroup& group);
    int findtab(MonClient& client, MonGroup& group) const;
  
  private:
    unsigned _maxtabs;
    unsigned _used;
    MonTab** _tabs;
    bool  _needread;
  };
};

#endif
