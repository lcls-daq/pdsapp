#ifndef Pds_Epix10kaQuadConfigP_hh
#define Pds_Epix10kaQuadConfigP_hh

#include "pdsapp/config/Serializer.hh"
#include <QtCore/QObject>

namespace Pds_ConfigDb {
  namespace Epix10kaQuad { 
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
      void set_gain(int);
      void set_all_gain();
    private:
      ConfigTable& _table;
    };
  };

  class Epix10kaQuadConfigP : public Serializer {
  public:
    Epix10kaQuadConfigP();
    ~Epix10kaQuadConfigP();
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
    bool validate       ();
  private:
    Epix10kaQuad::ConfigTable* _table;
  };
};

#endif
