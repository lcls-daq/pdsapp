#ifndef EPIXCOPYASICDIALOG_H
#define EPIXCOPYASICDIALOG_H

#include <QtGui/QDialog>
#include "pds/config/EpixConfigType.hh"
#include "pdsapp/config/ParameterSet.hh"

#include <vector>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace Pds_ConfigDb {

  class EpixCopyTarget {
  public:
    virtual ~EpixCopyTarget() {}
    virtual void copy(const EpixCopyTarget&) = 0;
  };

  class EpixCopyAsicDialog : public QDialog
  {
    Q_OBJECT

    public:
    enum {numberOfASICs=EpixConfigShadow::NumberOfAsics};
    EpixCopyAsicDialog(int, std::vector<EpixCopyTarget*>, QWidget* parent=0);
    virtual ~EpixCopyAsicDialog() {}

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
    std::vector<EpixCopyTarget*> _targets;
  };


  class EpixAsicSet : public QObject, public ParameterSet {
    Q_OBJECT
    public:
    EpixAsicSet(const char* l, Pds::LinkedList<Parameter>* a, ParameterCount& c,
                std::vector<EpixCopyTarget*> d) :
      QObject(), ParameterSet(l, a, c), copyButton(0), dialog(0), data(d) {}
    ~EpixAsicSet();

    public:
    QWidget* insertWidgetAtLaunch(int);

    public:
    QWidget*           copyButton;
    EpixCopyAsicDialog *dialog;
    std::vector<EpixCopyTarget*> data;

    private slots:
    void     copyClicked();
  };
};

#endif  // EPIXCOPYASICDIALOG_H
