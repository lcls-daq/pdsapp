#ifndef XAMPSCOPYASICDIALOG_H
#define XAMPSCOPYASICDIALOG_H

#include <QtGui/QDialog>
#include "pds/config/XampsConfigType.hh"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace Pds_ConfigDb {
  class XampsConfig;

  class XampsCopyAsicDialog : public QDialog
  {
    Q_OBJECT

    public:
    enum {numberOfASICs=XampsConfigType::NumberOfASICs};
    XampsCopyAsicDialog(int, QWidget *parent = 0);

    void context(XampsConfig* c) { _config = c; }

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
    XampsConfig* _config;
  };
};

#endif
