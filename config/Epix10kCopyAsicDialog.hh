#ifndef EPIX10kCOPYASICDIALOG_H
#define EPIX10kCOPYASICDIALOG_H

#include <QtGui/QDialog>
#include "pds/config/EpixConfigType.hh"
#include "pdsapp/config/ParameterSet.hh"

#include <vector>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace Pds_ConfigDb {

  class Epix10kCopyTarget {
  public:
    virtual ~Epix10kCopyTarget() {}
    virtual void copy(const Epix10kCopyTarget&) = 0;
  };

  class Epix10kCopyAsicDialog : public QDialog
  {
    Q_OBJECT

    public:
    enum {numberOfASICs=Epix10kConfigShadow::NumberOfAsics};
    Epix10kCopyAsicDialog(int, std::vector<Epix10kCopyTarget*>, QWidget* parent=0);
    virtual ~Epix10kCopyAsicDialog() {}

    private slots:
    void copyClicked();
    void selectAllClicked();
    void selectNoneClicked();

    private:
    int       index;
    QCheckBox *asicCheckBox[numberOfASICs-1];
    QPushButton* copyButton;
    QPushButton* selectAllButton;
    QPushButton* selectNoneButton;
    QPushButton* quitButton;
    std::vector<Epix10kCopyTarget*> _targets;
  };


  class Epix10kAsicSet : public QObject, public ParameterSet {
    Q_OBJECT
    public:
    Epix10kAsicSet(const char* l, Pds::LinkedList<Parameter>* a, ParameterCount& c,
                std::vector<Epix10kCopyTarget*> d) :
      QObject(), ParameterSet(l, a, c), copyButton(0), dialog(0), data(d) {}
    ~Epix10kAsicSet();

    public:
    QWidget* insertWidgetAtLaunch(int);

    public:
    QWidget*           copyButton;
    Epix10kCopyAsicDialog *dialog;
    std::vector<Epix10kCopyTarget*> data;

    private slots:
    void     copyClicked();
  };
};

#endif  // EPIXCOPYASICDIALOG_H
