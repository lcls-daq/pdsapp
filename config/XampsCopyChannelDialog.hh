#ifndef XAMPSCOPYCHANNELDIALOG_H
#define XAMPSCOPYCHANNELDIALOG_H

#include <QtGui/QDialog>
#include "pds/config/XampsConfigType.hh"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace Pds_ConfigDb {
  class XampsConfig;

  class XampsCopyChannelDialog : public QDialog
  {
    Q_OBJECT

    public:
    enum {numberOfChannels=XampsASIC::NumberOfChannels};
    XampsCopyChannelDialog(int, QWidget* parent = 0);
    virtual ~XampsCopyChannelDialog() {};
    void context(int i, XampsConfig* c) { myAsic=i; _config=c; }

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
    XampsConfig* _config;
  };
};

#endif
