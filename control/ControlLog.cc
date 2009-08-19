#include "ControlLog.hh"

#include <QtCore/QString>
#include <QtCore/QTime>

using namespace Pds;

ControlLog::ControlLog() :
  QTextEdit(QString("%1: Run control started\n").
		    arg(QTime::currentTime().
			toString("hh:mm:ss")))
{
  setReadOnly(true);
}

ControlLog::~ControlLog() {}
