#ifndef FEXAMPCOPYASICDIALOG_H
#define FEXAMPCOPYASICDIALOG_H

#include <QtGui/QDialog>
#include "pds/config/FexampConfigType.hh"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace Pds_ConfigDb {
  class FexampConfig;

  class FexampCopyAsicDialog : public QDialog
  {
    Q_OBJECT

    public:
    enum {numberOfASICs=FexampConfigType::NumberOfASICs};
    FexampCopyAsicDialog(int, QWidget *parent = 0);

    void context(FexampConfig* c) { _config = c; }

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
    FexampConfig* _config;
  };
};

#endif
