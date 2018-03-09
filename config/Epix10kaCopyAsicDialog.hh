#ifndef EPIX10kaCOPYASICDIALOG_H
#define EPIX10kaCOPYASICDIALOG_H

#include <QtGui/QDialog>
#include "pds/config/EpixConfigType.hh"
#include "pdsapp/config/ParameterSet.hh"

#include <vector>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace Pds_ConfigDb {

  class Epix10kaCopyTarget {
  public:
    virtual ~Epix10kaCopyTarget() {}
    virtual void copy(const Epix10kaCopyTarget&) = 0;
  };

  class Epix10kaCopyAsicDialog : public QDialog
  {
    Q_OBJECT

    public:
    Epix10kaCopyAsicDialog(int, std::vector<Epix10kaCopyTarget*>, QWidget* parent=0);
    virtual ~Epix10kaCopyAsicDialog() {}

    private slots:
    void copyClicked();
    void selectAllClicked();
    void selectNoneClicked();

    private:
    int       index;
    QCheckBox* asicCheckBox[3];
    QPushButton* copyButton;
    QPushButton* selectAllButton;
    QPushButton* selectNoneButton;
    QPushButton* quitButton;
    std::vector<Epix10kaCopyTarget*> _targets;
  };


  class Epix10kaAsicSet : public QObject, public ParameterSet {
    Q_OBJECT
    public:
    Epix10kaAsicSet(const char* l, Pds::LinkedList<Parameter>* a, ParameterCount& c,
                std::vector<Epix10kaCopyTarget*> d) :
      QObject(), ParameterSet(l, a, c), copyButton(0), dialog(0), data(d) {}
    ~Epix10kaAsicSet();

    public:
    QWidget* insertWidgetAtLaunch(int);

    public:
    QWidget*           copyButton;
    Epix10kaCopyAsicDialog *dialog;
    std::vector<Epix10kaCopyTarget*> data;

    private slots:
    void     copyClicked();
  };
};

#endif  // EPIX10kaCOPYASICDIALOG_H
