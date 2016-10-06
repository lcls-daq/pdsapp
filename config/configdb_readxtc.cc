#include "pdsapp/config/XtcFileServer.hh"
#include "pdsapp/config/Xtc_Ui.hh"
#include "pdsapp/config/Parameters.hh"

#include <QtGui/QApplication>
#include <QtGui/QWidget>
#include <QtGui/QVBoxLayout>

#include <unistd.h>
#include <stdio.h>

using namespace Pds_ConfigDb;

static void show_usage(const char* p)
{
  printf("Usage: %s [options] <path>\n",p);
  printf("options: -e <try expert view first>\n");
}

extern int optind;

int main(int argc, char** argv)
{
  bool expert = false;

  int c;
  while ((c = getopt(argc, argv, "e")) != -1) {
    switch(c) {
    case 'e':
      expert = true;
      break;
    default:
      show_usage(argv[0]);
      return 0;
    }
  }

  if (optind == argc) {
    printf("Missing <path> argument\n");
    show_usage(argv[0]);
    return 0;
  }

  Parameter::readFromData(true);

  QApplication app(argc, argv);

  QWidget* parent = new QWidget(0);
  parent->setWindowTitle("Read XTC");
  parent->setAttribute(::Qt::WA_DeleteOnClose, true);

  XtcFileServer* fs = new XtcFileServer(argv[optind]);
  Xtc_Ui* ui = new Xtc_Ui((QWidget*)0, expert);
  QObject::connect(fs, SIGNAL(file_selected(QString)), ui, SLOT(set_file(QString)));
  QObject::connect(fs, SIGNAL(prev_cycle()), ui, SLOT(prev_cycle()));
  QObject::connect(fs, SIGNAL(next_cycle()), ui, SLOT(next_cycle()));
  QObject::connect(ui, SIGNAL(set_cycle(int)), fs, SLOT(set_cycle(int)));

  QVBoxLayout* l = new QVBoxLayout;
  l->addWidget(fs);
  l->addWidget(ui);

  parent->setLayout(l);
  parent->show();

  app.exec();
}
