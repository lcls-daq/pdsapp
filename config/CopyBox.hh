#ifndef Pds_ConfigDb_QtCopyBox_hh
#define Pds_ConfigDb_QtCopyBox_hh

#include <QtGui/QGroupBox>

namespace Pds_ConfigDb {
  class CopyBox : public QGroupBox {
    Q_OBJECT
  public:
    CopyBox(const char* title, 
            const char* rowLabel,
            const char* columnLabel,
            unsigned    rows, 
            unsigned    columns);
    ~CopyBox();
  public:
    virtual void initialize() = 0;
    virtual void finalize  () = 0;
    virtual void copyItem(unsigned rowFrom,
                          unsigned columnFrom,
                          unsigned rowTo,
                          unsigned columnTo) = 0;
  public slots:
    void all ();
    void none();
    void copy();
  private:  
    class PrivateData;
    PrivateData* _private;
  };
};

#endif
