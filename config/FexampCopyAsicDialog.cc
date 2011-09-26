#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include "pdsapp/config/FexampCopyAsicDialog.hh"
#include "pdsapp/config/FexampConfig.hh"

namespace Pds_ConfigDb {

  FexampCopyAsicDialog::FexampCopyAsicDialog(int ind, QWidget *parent)
  : QDialog(parent), index(ind)
  {
    char foo[80];
    unsigned u = 0;
    for (int i=0; i<numberOfASICs; i++) {        
      if (i != index) {
        sprintf(foo, "ASIC: %d", i);
        asicCheckBox[u] = new QCheckBox(QString(foo));
        asicCheckBox[u++]->setCheckState(Qt::Unchecked);
      }
    }

    copyButton = new QPushButton(tr("&Copy"));
    copyButton->setDefault(true);
    copyButton->setEnabled(true);

    selectAllButton = new QPushButton(tr("Select &All"));
    selectNoneButton = new QPushButton(tr("Select &None"));

    quitButton = new QPushButton(tr("&Quit"));

    connect(copyButton, SIGNAL(clicked()),
        this, SLOT(copyClicked()));
    connect(selectAllButton, SIGNAL(clicked()),
        this, SLOT(selectAllClicked()));
    connect(selectNoneButton, SIGNAL(clicked()),
        this, SLOT(selectNoneClicked()));
    connect(quitButton, SIGNAL(clicked()),
        this, SLOT(reject()));

    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->addWidget(new QLabel("Destination ASICs"));
    for (int i=0; i<numberOfASICs-1; i++) {        
      leftLayout->addWidget(asicCheckBox[i]);
    }

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

    sprintf(foo, "Copy ASIC %d", index);
    setWindowTitle(tr(foo));
    setFixedHeight(sizeHint().height());
  }

  void FexampCopyAsicDialog::copyClicked()
  {
    FexampASICdata* src = _config->_asic[index];
    unsigned cb = 0;
    for (int i=0; i<numberOfASICs; i++) {        
      if (i != index) {
        if (asicCheckBox[cb]->checkState() == Qt::Checked) {
          FexampASICdata* dest = _config->_asic[i];
          for (int j=0; j<FexampASIC::NumberOfASIC_Entries; j++) {
            dest->_reg[j]->value = src->_reg[j]->value;
          }
          for (int j=0; j<FexampASIC::NumberOfChannels; j++) {
            FexampChannelData* chsrc = src->_channel[j];
            for (int l=0; l<FexampChannel::NumberOfChannelBitFields; l++) {
              dest->_channel[j]->_reg[l]->value = chsrc->_reg[l]->value;
            }
          }
        }
        cb++;
      }
    }
    emit accepted();
    emit done(0);
  }

  void FexampCopyAsicDialog::selectAllClicked()
  {
    unsigned cb = 0;
    for (int i=0; i<numberOfASICs; i++)
    {
      if (i!=index)
      {
        asicCheckBox[cb++]->setCheckState(Qt::Checked);
      }
    }
  }

  void FexampCopyAsicDialog::selectNoneClicked()
  {
    unsigned cb = 0;
    for (int i=0; i<numberOfASICs; i++)
    {
      if (i!=index)
      {
        asicCheckBox[cb++]->setCheckState(Qt::Unchecked);
      }
    }
  }
}
