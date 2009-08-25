#ifndef Pds_MonCANVAS_HH
#define Pds_MonCANVAS_HH

#include <QtGui/QGroupBox>

class QMenu;
class QAction;
class QActionGroup;
class QImage;

namespace Pds {

  class MonEntry;
  class MonGroup;
  class MonQtBase;

  class MonCanvas : public QGroupBox {
    Q_OBJECT
  public:
    MonCanvas(QWidget&        parent, 
	      const MonEntry& entry);
    virtual ~MonCanvas();
  
    //    void saveas(const char* type, const char* groupname);
    int writeconfig(FILE* file);
    int readconfig(FILE* file, int color);

    virtual void info();
    virtual void dialog() = 0;
    virtual int update() = 0;
    virtual int reset(const MonGroup& group) = 0;
    virtual unsigned getplots(MonQtBase**, const char** names) = 0;
    virtual const MonQtBase* selected() const = 0;

  signals:
    void redraw();

  public slots:
    void settings();
    void close();
    void save_image();
    void save_data ();
    void show_info();

  public:
    const MonEntry* _entry;

  protected:
    enum Select {Undefined, 
		 Normal,
		 Integrated, Since, Difference, 
		 ProjectionX, ProjectionY,
		 IntegratedX, DifferenceX, 
		 IntegratedY, DifferenceY,
		 Chart, ChartX, ChartY, 
		 SuperimposeCharts, SeparateCharts };
    Select        _selected;
    QMenu*        _select;
    QActionGroup* _select_group;

    void menu_service(Select s, 
		      const char* label, 
		      const char* slot, 
		      bool=false);
    
    virtual void select(Select selection);

  protected slots:
    void setNormal     ();
    void setProjectionX();
    void setProjectionY();
    void setIntegrated ();
    void setSince      ();
    void setDifference ();
    void setChart      ();
    void setIntegratedX();
    void setIntegratedY();
    void setDifferenceX();
    void setDifferenceY();
    void setChartX     ();
    void setChartY     ();
  };
};

#define _menu_service(s,lInit) menu_service(s, #s, SLOT( set ## s () ), lInit)

#endif
