#include "pdsapp/config/PolyDialog.hh"

#include <QtGui/QFileDialog>

using namespace Pds_ConfigDb;

PolyDialog::~PolyDialog() { delete _d; }

void PolyDialog::mport() {
  if (_d->exec()==QDialog::Accepted)
    _p.mport(_d->selectedFiles()[0]);
}

void PolyDialog::xport() {
  if (_d->exec()==QDialog::Accepted)
    _p.xport(_d->selectedFiles()[0]);
}
