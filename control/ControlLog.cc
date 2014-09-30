#include "ControlLog.hh"

#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtGui/QTextCursor>

using namespace Pds;

ControlLog::ControlLog() :
  QTextEdit(0)
{
  setReadOnly(true);
  qRegisterMetaType<QTextCursor>();
  QObject::connect(this, SIGNAL(appended(const QString&)),
		   this, SLOT(append(const QString&)));

  QString s = QString("%1: Run control started\n").
    arg(QTime::currentTime().
	toString("hh:mm:ss"));
  appendText(s);
}

ControlLog::~ControlLog() {}

void ControlLog::appendText(const QString& t) { emit appended(t); }
