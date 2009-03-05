#ifndef Pds_ConfigTC_Gui_hh
#define Pds_ConfigTC_Gui_hh

#include <QtGui/QWidget>

class QListWidget;
class QListWidgetItem;

namespace ConfigGui {

  class Serializer;

  class ConfigTC_Gui : public QWidget {
    Q_OBJECT
  public:
    ConfigTC_Gui();
    ~ConfigTC_Gui();
  public slots:
    void launchSerializer(QListWidgetItem*);
  public:
    void addSerializer(Serializer* s);
  private:
    QListWidget* _types;
    enum { MaxSerializers=10 };
    Serializer*  _serializers[MaxSerializers];
  };
};

#endif  
