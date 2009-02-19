#ifndef Pds_MonTAB_HH
#define Pds_MonTAB_HH

#include <QtGui/QWidget>
#include <QtCore/QSize>

namespace Pds {

  class MonEntry;
  class MonCanvas;
  class MonClient;
  class MonGroup;

  class MonTab : public QWidget {
    Q_OBJECT
  public:
    MonTab(MonClient& client,
	   const MonGroup& group);
    virtual ~MonTab();

    virtual QSize sizeHint() const;

    int reset();

    //    void add(MonEntry& entry);
    void update();
    void layout();
    void current(bool iscurrent);

    const MonClient& client() const;
    const MonGroup& group() const;
    //    const char* name() const;

    int writeconfig(FILE* fp);
    int readconfig (FILE* fp);

  private:
    /*
    void destroy(int short which);
    void adjust();
    */
    MonCanvas* getcanvas(const char* name);
  private:
    bool _iscurrent;

  private:  
    MonClient& _client;
    unsigned short _maxcanvases;
    unsigned short _used;
    MonCanvas** _canvases;
    char* _groupname;
    const MonGroup* _group;
    int _color;
  };
};

#endif
