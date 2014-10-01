#ifndef EPIX100aCOPYASICDIALOG_H
#define EPIX100aCOPYASICDIALOG_H

#include <QtGui/QDialog>
#include "pds/config/EpixConfigType.hh"
#include "pdsapp/config/ParameterSet.hh"

#include <vector>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace Pds_ConfigDb {

  class Epix100aCopyTarget {
  public:
    virtual ~Epix100aCopyTarget() {}
    virtual void copy(const Epix100aCopyTarget&) = 0;
  };

  class Epix100aCopyAsicDialog : public QDialog
  {
    Q_OBJECT

    public:
    Epix100aCopyAsicDialog(int, std::vector<Epix100aCopyTarget*>, QWidget* parent=0);
    virtual ~Epix100aCopyAsicDialog() {}

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
    std::vector<Epix100aCopyTarget*> _targets;
  };


  class Epix100aAsicSet : public QObject, public ParameterSet {
    Q_OBJECT
    public:
    Epix100aAsicSet(const char* l, Pds::LinkedList<Parameter>* a, ParameterCount& c,
                std::vector<Epix100aCopyTarget*> d) :
      QObject(), ParameterSet(l, a, c), copyButton(0), dialog(0), data(d) {}
    ~Epix100aAsicSet();

    public:
    QWidget* insertWidgetAtLaunch(int);

    public:
    QWidget*           copyButton;
    Epix100aCopyAsicDialog *dialog;
    std::vector<Epix100aCopyTarget*> data;

    private slots:
    void     copyClicked();
  };
};

#endif  // EPIXCOPYASICDIALOG_H
