#include "pdsapp/config/AliasConfig.hh"

#include "pdsapp/config/Parameters.hh"

#include "pds/config/AliasConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

#include <new>

using Pds::DetInfo;

namespace Pds_ConfigDb {
  
  class AliasConfig::Private_Data : public Parameter {
  public:
    Private_Data(){}

    int pull(void* from) {
      int index=0;
      while( index < _layout->count()) {
        int row, col, srow, scol;
        _layout->getItemPosition(index, &row, &col, &srow, &scol);
        if (row>0)
          delete _layout->takeAt(index);
        else
          index++;
      }
      const AliasConfigType& tc = *reinterpret_cast<const AliasConfigType*>(from);
      for(unsigned i=0; i<tc.numSrcAlias(); i++) {
        QString src = QString("%1.%2").
          arg(QString::number(tc.srcAlias()[i].src().log(),16)).
          arg(QString::number(tc.srcAlias()[i].src().phy(),16));
        _layout->addWidget(new QLabel(src), i+1, 0);
        QString name = QString(DetInfo::name(static_cast<const DetInfo&>(tc.srcAlias()[i].src())));
        _layout->addWidget(new QLabel(name), i+1, 1);
        QString alias = QString(tc.srcAlias()[i].aliasName());
        _layout->addWidget(new QLabel(alias), i+1, 2);
      }
      return tc._sizeof();
    }

    int push(void* to) {
      AliasConfigType tc(0,0);
      return tc._sizeof();
    }

    int dataSize() const {
      AliasConfigType tc(0,0);
      return tc._sizeof();
    }

  public:
    QLayout* initialize(QWidget*) {
      _layout = new QGridLayout;
      _layout->addWidget(new QLabel("Source"), 0, 0);
      _layout->addWidget(new QLabel("Name")  , 0, 1);
      _layout->addWidget(new QLabel("Alias") , 0, 2);
      return _layout;
    }
    void update() {}
    void flush() {}
    void enable(bool) {}
  private:
    QGridLayout* _layout;
  };
};


using namespace Pds_ConfigDb;

AliasConfig::AliasConfig() : 
  Serializer("Alias_Config"),
  _private_data( new Private_Data )
{
  pList.insert(_private_data);
}

int  AliasConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  AliasConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  AliasConfig::dataSize() const {
  return _private_data->dataSize();
}

