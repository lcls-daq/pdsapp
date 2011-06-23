#ifndef Pds_XampsConfig_hh
#define Pds_XampsConfig_hh

#include "pdsapp/config/Serializer.hh"
#include "pds/config/XampsConfigType.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pdsapp/config/XampsCopyChannelDialog.hh"
#include "pdsapp/config/XampsCopyAsicDialog.hh"

#include <QtGui/QPushButton>
#include <QtCore/QObject>

namespace Pds_ConfigDb {

  class XampsChannelSet : public QObject, public ParameterSet {
    Q_OBJECT
    public:
    XampsChannelSet(const char* l, Pds::LinkedList<Parameter>* a, ParameterCount& c) :
      QObject(), ParameterSet(l, a, c), copyButton(0), dialog(0) {}
    virtual ~XampsChannelSet() {
      if (copyButton) delete copyButton;
      if (dialog) delete dialog;
    }

    public:
    QWidget* insertWidgetAtLaunch(int);

    public:
    QWidget* copyButton;
    XampsCopyChannelDialog *dialog;
    XampsConfig*   config;
    int            myAsic;

    private slots:
    void     copyClicked();
  };

  class XampsAsicSet : public QObject, public ParameterSet {
    Q_OBJECT
    public:
    XampsAsicSet(const char* l, Pds::LinkedList<Parameter>* a, ParameterCount& c) :
      QObject(), ParameterSet(l, a, c), copyButton(0), dialog(0) {}
    virtual ~XampsAsicSet() {
      if (copyButton) delete copyButton;
      if (dialog) delete dialog;
    }

    public:
    QWidget* insertWidgetAtLaunch(int);

    public:
    QWidget* copyButton;
    XampsCopyAsicDialog *dialog;
    XampsConfig*   config;

    private slots:
    void     copyClicked();
  };



  class SimpleCount : public ParameterCount {
    public:
    SimpleCount(unsigned c) : mycount(c) {};
    ~SimpleCount() {};
    bool connect(ParameterSet&) { return false; }
    unsigned count() { return mycount; }
    unsigned mycount;
  };

  class XampsChannelData {
    public:
      XampsChannelData();
      ~XampsChannelData() {};
      void insert(Pds::LinkedList<Parameter>&);
      int pull(void*);
      int push(void*);
    public:
      NumericInt<uint32_t>* _reg[XampsChannel::NumberOfChannelBitFields];
  };

  class XampsASICdata {
    public:
      XampsASICdata(XampsConfig*, int);
      ~XampsASICdata() {};
      void insert(Pds::LinkedList<Parameter>&);
      int pull(void*);
      int push(void*);
    public:
      NumericInt<uint32_t>*      _reg[XampsASIC::NumberOfASIC_Entries];
      SimpleCount                _count;
      XampsChannelData*          _channel[XampsASIC::NumberOfChannels];
      Pds::LinkedList<Parameter> _channelArgs[XampsASIC::NumberOfChannels];
      XampsChannelSet            _channelSet;
      XampsConfig*               _config;
  };

  class XampsConfig : public Serializer {
  public:
    XampsConfig();
    ~XampsConfig() {};
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  public:
    NumericInt<uint32_t>*      _reg[XampsConfigType::NumberOfRegisters];
    SimpleCount                _count;
    XampsASICdata*             _asic[XampsConfigType::NumberOfASICs];
    Pds::LinkedList<Parameter> _asicArgs[XampsConfigType::NumberOfASICs];
    XampsAsicSet               _asicSet;
  };

};

#endif
