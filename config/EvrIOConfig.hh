#ifndef Pds_EvrIOConfig_hh
#define Pds_EvrIOConfig_hh

#include "pdsapp/config/Serializer.hh"

#include <QtCore/QObject>

namespace Pds_ConfigDb {

  class EvrIOConfig : public Serializer {
  public:
    EvrIOConfig();
    ~EvrIOConfig() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
  public:
    class Panel;
    class Private_Data;
  private:
    Private_Data* _private_data;
  };

  class EvrIOConfigQ : public QObject {
    Q_OBJECT
  public:
    EvrIOConfigQ(EvrIOConfig::Private_Data&,
                 QWidget*);
  public slots:
    void addEvr();
    void remEvr();

  private:
    EvrIOConfig::Private_Data& _c;
  };

};

#endif
