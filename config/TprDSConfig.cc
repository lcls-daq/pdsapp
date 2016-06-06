#include "pdsapp/config/TprDSConfig.hh"
#include "pds/config/TprDSConfigType.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QTabWidget>
#include <QtGui/QMessageBox>
#include <QtGui/QLabel>

namespace Pds_ConfigDb {

  class TprDSConfig::Private_Data {
  public:
    Private_Data() :
      _dataSize("Data Size      (Words) ",20,0,1018),
      _fullThr ("Full Threshold (0-1023)",512,0,1023)
    {}
    ~Private_Data()
    {}
  public:
    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_dataSize);
      pList.insert(&_fullThr );
    }
    int pull(void *from) {
      const TprDSConfigType& tc = *reinterpret_cast<const TprDSConfigType*>(from);
      _dataSize.value      = tc.dataSize();
      _fullThr .value      = tc.fullThreshold();
      return tc._sizeof();
    }
    int push(void *to) {
      *new(to) TprDSConfigType(_dataSize.value,
                               _fullThr .value);
      return sizeof(TprDSConfigType);
    }

    int dataSize() const { return sizeof(TprDSConfigType); }
    bool validate() { return true; }
  private:
    NumericInt<unsigned> _dataSize;
    NumericInt<unsigned> _fullThr;
  };

};


using namespace Pds_ConfigDb;

TprDSConfig::TprDSConfig():
  Serializer("TprDS_Config"), 
  _private_data(new Private_Data)
{
  _private_data->insert(pList);
}

TprDSConfig::~TprDSConfig()
{
  delete _private_data;
}

int TprDSConfig::readParameters(void *from)
{
  return _private_data->pull(from);
}

int TprDSConfig::writeParameters(void *to)
{
  return _private_data->push(to);
}

int TprDSConfig::dataSize() const
{
  return _private_data->dataSize();
}

bool TprDSConfig::validate() 
{
  return _private_data->validate();
}
