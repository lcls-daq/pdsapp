#include "pdsapp/config/FilterDialog.hh"

#include <QtGui/QFileDialog>

using namespace Pds_ConfigDb;

FilterDialog::~FilterDialog() { delete _d; }

void FilterDialog::mport() {
  _d->setAcceptMode(QFileDialog::AcceptOpen);
  if (_d->exec()==QDialog::Accepted)
    _p.mport(_d->selectedFiles()[0]);
}

void FilterDialog::xport() {
  _d->setAcceptMode(QFileDialog::AcceptSave);
  if (_d->exec()==QDialog::Accepted)
    _p.xport(_d->selectedFiles()[0]);
}
