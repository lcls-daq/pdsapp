#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include "pdsapp/config/FexampConfig.hh"
#include "pdsapp/config/FexampCopyChannelDialog.hh"

#include <stdio.h>

namespace Pds_ConfigDb {

  FexampCopyChannelDialog::FexampCopyChannelDialog(int ind, QWidget *parent)
      : QDialog(parent), index(ind)
  {
    char foo[80];
    unsigned u = 0;
    for (int i=0; i<numberOfChannels; i++) {
      if (i != index) {
        sprintf(foo, "Channel: %d", i);
        channelCheckBox[u] = new QCheckBox(QString(foo));
        channelCheckBox[u++]->setCheckState(Qt::Unchecked);
      }
    }

    copyButton = new QPushButton(tr("&Copy"));
    copyButton->setDefault(true);
    copyButton->setEnabled(true);

    selectAllButton = new QPushButton(tr("Select &All"));
    selectNoneButton = new QPushButton(tr("Select &None"));

    quitButton = new QPushButton(tr("&Quit"));

    //    connect(lineEdit, SIGNAL(textChanged(const QString &)),
    //            this, SLOT(enableFindButton(const QString &)));
    connect(copyButton, SIGNAL(clicked()),
        this, SLOT(copyClicked()));
    connect(selectAllButton, SIGNAL(clicked()),
        this, SLOT(selectAllClicked()));
    connect(selectNoneButton, SIGNAL(clicked()),
        this, SLOT(selectNoneClicked()));
    connect(quitButton, SIGNAL(clicked()),
        this, SLOT(reject()));

    QVBoxLayout*  bottomLeftLayout[numberOfChannels/16];
    for (int i=0; i<numberOfChannels/16; i++) {
      bottomLeftLayout[i] = new QVBoxLayout;
    }
    int lastbottomLeftLayout = 0;
    for (int i=0; i<numberOfChannels-1; i++) {
      bottomLeftLayout[i/16]->addWidget(channelCheckBox[i]);
      lastbottomLeftLayout = i/16;
    }
    bottomLeftLayout[lastbottomLeftLayout]->addStretch();

    QHBoxLayout* BLCLayout = new QHBoxLayout;
    for (int i=0; i<numberOfChannels/16; i++) {
      BLCLayout->addLayout(bottomLeftLayout[i]);
    }

    QLabel* destinations = new QLabel("Destination Channels");
    destinations->setAlignment(Qt::AlignHCenter);

    QVBoxLayout* leftLayout = new QVBoxLayout;
    leftLayout->addWidget(destinations);
    leftLayout->addLayout(BLCLayout);

    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->addStretch();
    rightLayout->addWidget(copyButton);
    rightLayout->addWidget(selectAllButton);
    rightLayout->addWidget(selectNoneButton);
    rightLayout->addWidget(quitButton);
    rightLayout->addStretch();

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(rightLayout);
    setLayout(mainLayout);

    sprintf(foo, "Fexamp Copy Channel %d", index);
    setWindowTitle(tr(foo));
    setFixedHeight(sizeHint().height());
  }

  class FexampChannelData;

  void FexampCopyChannelDialog::copyClicked()
  {
    FexampASICdata& a = *_config->_asic[myAsic];
    FexampChannelData* src = a._channel[index];
    unsigned cb = 0;
    for (int i=0; i<numberOfChannels; i++) {
      if (i != index) {
        if (channelCheckBox[cb]->checkState() == Qt::Checked) {
          FexampChannelData* dest = a._channel[i];
          for (int j=0; j<FexampChannel::NumberOfChannelBitFields; j++) {
            dest->_reg[j]->value = src->_reg[j]->value;
          }
        }
        cb++;
      }
    }
    emit accepted();
    emit done(0);
  }

  void FexampCopyChannelDialog::selectAllClicked()
  {
    unsigned cb = 0;
    for (int i=0; i<numberOfChannels; i++)
    {
      if (i!=index)
      {
        channelCheckBox[cb++]->setCheckState(Qt::Checked);
      }
    }
  }

  void FexampCopyChannelDialog::selectNoneClicked()
  {
    unsigned cb = 0;
    for (int i=0; i<numberOfChannels; i++)
    {
      if (i!=index)
      {
        channelCheckBox[cb++]->setCheckState(Qt::Unchecked);
      }
    }
  }
}
