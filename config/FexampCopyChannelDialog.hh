#ifndef FEXAMPCOPYCHANNELDIALOG_H
#define FEXAMPCOPYCHANNELDIALOG_H

#include <QtGui/QDialog>
#include "pds/config/FexampConfigType.hh"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace Pds_ConfigDb {
  class FexampConfig;

  class FexampCopyChannelDialog : public QDialog
  {
    Q_OBJECT

    public:
    enum {numberOfChannels=FexampASIC::NumberOfChannels};
    FexampCopyChannelDialog(int, QWidget* parent = 0);
    virtual ~FexampCopyChannelDialog() {};
    void context(int i, FexampConfig* c) { myAsic=i; _config=c; }

    private slots:
    void copyClicked();
    void selectAllClicked();
    void selectNoneClicked();

    private:
    QCheckBox *channelCheckBox[numberOfChannels-1];
    QPushButton* copyButton;
    QPushButton* quitButton;
    QPushButton* selectAllButton;
    QPushButton* selectNoneButton;
    int          index;
    int          myAsic;
    FexampConfig* _config;
  };
};

#endif
