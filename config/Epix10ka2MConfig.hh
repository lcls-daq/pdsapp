#ifndef Pds_Epix10ka2MConfig_hh
#define Pds_Epix10ka2MConfig_hh

#include "pdsapp/config/Serializer.hh"
#include <QtCore/QObject>

namespace Pds_ConfigDb {
  namespace Epix10ka2M { 
    class ConfigTable; 
    class ConfigTableQ : public QObject {
      Q_OBJECT
    public:
      ConfigTableQ(ConfigTable&,QObject*);
    public slots:
      void update_maskv();
      void update_maskg();
      void pixel_map_dialog();
      void calib_map_dialog();
      void set_high_gain();
      void set_medium_gain();
      void set_low_gain();
      void set_auto_high_low_gain();
      void set_auto_medium_low_gain();
    private:
      ConfigTable& _table;
    };
  };

  class Epix10ka2MConfig : public Serializer {
  public:
    Epix10ka2MConfig();
    ~Epix10ka2MConfig();
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
    bool validate       ();
  private:
    Epix10ka2M::ConfigTable* _table;
  };
};

#endif
