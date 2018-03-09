#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include "pdsapp/config/Epix10kaConfig.hh"
#include "pdsapp/config/Epix10kaCopyAsicDialog.hh"
#include "pds/config/Epix10kaConfigV1.hh"

#include <stdio.h>

namespace Pds_ConfigDb {

  Epix10kaCopyAsicDialog::Epix10kaCopyAsicDialog(int ind, 
                                         std::vector<Epix10kaCopyTarget*> t,
                                         QWidget *parent)
    : QDialog(parent), index(ind), _targets(t)
  {
    char foo[80];
    unsigned u = 0;
    for (int i=0; i<Epix10kaConfigShadow::NumberOfAsics; i++) {
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
    for (int i=0; i<(Epix10kaConfigShadow::NumberOfAsics-1); i++) {
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

  void Epix10kaCopyAsicDialog::copyClicked()
  {
    const Epix10kaCopyTarget& src = *_targets[index];
    unsigned cb = 0;
    for (int i=0; i<Epix10kaConfigShadow::NumberOfAsics; i++) {
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

  void Epix10kaCopyAsicDialog::selectAllClicked()
  {
    unsigned cb = 0;
    for (int i=0; i<Epix10kaConfigShadow::NumberOfAsics; i++)
    {
      if (i!=index)
      {
        asicCheckBox[cb++]->setCheckState(Qt::Checked);
      }
    }
  }

  void Epix10kaCopyAsicDialog::selectNoneClicked()
  {
    unsigned cb = 0;
    for (int i=0; i<Epix10kaConfigShadow::NumberOfAsics; i++)
    {
      if (i!=index)
      {
        asicCheckBox[cb++]->setCheckState(Qt::Unchecked);
      }
    }
  }

  Epix10kaAsicSet::~Epix10kaAsicSet() {}

  void Epix10kaAsicSet::copyClicked() {
    dialog->exec();
  }


  QWidget* Epix10kaAsicSet::insertWidgetAtLaunch(int ind) {
    dialog = new Epix10kaCopyAsicDialog(ind,data);
    copyButton = new QPushButton(tr("&Copy this ASIC"));
    QObject::connect(copyButton, SIGNAL(clicked()), this, SLOT(copyClicked()));
    return copyButton;
  }


}
