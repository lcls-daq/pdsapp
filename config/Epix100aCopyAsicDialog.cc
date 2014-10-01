#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include "pdsapp/config/Epix100aConfig.hh"
#include "pdsapp/config/Epix100aCopyAsicDialog.hh"
#include "pds/config/Epix100aConfigV1.hh"

#include <stdio.h>

namespace Pds_ConfigDb {

  Epix100aCopyAsicDialog::Epix100aCopyAsicDialog(int ind, 
                                         std::vector<Epix100aCopyTarget*> t,
                                         QWidget *parent)
    : QDialog(parent), index(ind), _targets(t)
  {
    char foo[80];
    unsigned u = 0;
    for (int i=0; i<Epix100aConfigShadow::NumberOfAsics; i++) {
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
    for (int i=0; i<(Epix100aConfigShadow::NumberOfAsics-1); i++) {
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

  void Epix100aCopyAsicDialog::copyClicked()
  {
    const Epix100aCopyTarget& src = *_targets[index];
    unsigned cb = 0;
    for (int i=0; i<Epix100aConfigShadow::NumberOfAsics; i++) {
      if (i != index) {
        if (asicCheckBox[cb]->checkState() == Qt::Checked) {
          _targets[i]->copy(src);
        }
        cb++;
      }
    }
    emit accepted();
    emit done(0);
  }

  void Epix100aCopyAsicDialog::selectAllClicked()
  {
    unsigned cb = 0;
    for (int i=0; i<Epix100aConfigShadow::NumberOfAsics; i++)
    {
      if (i!=index)
      {
        asicCheckBox[cb++]->setCheckState(Qt::Checked);
      }
    }
  }

  void Epix100aCopyAsicDialog::selectNoneClicked()
  {
    unsigned cb = 0;
    for (int i=0; i<Epix100aConfigShadow::NumberOfAsics; i++)
    {
      if (i!=index)
      {
        asicCheckBox[cb++]->setCheckState(Qt::Unchecked);
      }
    }
  }

  Epix100aAsicSet::~Epix100aAsicSet() {}

  void Epix100aAsicSet::copyClicked() {
    dialog->exec();
  }


  QWidget* Epix100aAsicSet::insertWidgetAtLaunch(int ind) {
    dialog = new Epix100aCopyAsicDialog(ind,data);
    copyButton = new QPushButton(tr("&Copy this ASIC"));
    QObject::connect(copyButton, SIGNAL(clicked()), this, SLOT(copyClicked()));
    return copyButton;
  }


}
