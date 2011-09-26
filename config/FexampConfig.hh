#ifndef Pds_FexampConfig_hh
#define Pds_FexampConfig_hh

#include "pdsapp/config/Serializer.hh"
#include "pds/config/FexampConfigType.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pdsapp/config/FexampCopyChannelDialog.hh"
#include "pdsapp/config/FexampCopyAsicDialog.hh"

#include <QtGui/QPushButton>
#include <QtCore/QObject>

namespace Pds_ConfigDb {

  class FexampChannelSet : public QObject, public ParameterSet {
    Q_OBJECT
    public:
    FexampChannelSet(const char* l, Pds::LinkedList<Parameter>* a, ParameterCount& c) :
      QObject(), ParameterSet(l, a, c), copyButton(0), dialog(0) {}
    virtual ~FexampChannelSet() {
      if (copyButton) delete copyButton;
      if (dialog) delete dialog;
    }

    public:
    QWidget* insertWidgetAtLaunch(int);

    public:
    QWidget* copyButton;
    FexampCopyChannelDialog *dialog;
    FexampConfig*   config;
    int            myAsic;

    private slots:
    void     copyClicked();
  };

  class FexampAsicSet : public QObject, public ParameterSet {
    Q_OBJECT
    public:
    FexampAsicSet(const char* l, Pds::LinkedList<Parameter>* a, ParameterCount& c) :
      QObject(), ParameterSet(l, a, c), copyButton(0), dialog(0) {}
    virtual ~FexampAsicSet() {
      if (copyButton) delete copyButton;
      if (dialog) delete dialog;
    }

    public:
    QWidget* insertWidgetAtLaunch(int);

    public:
    QWidget* copyButton;
    FexampCopyAsicDialog *dialog;
    FexampConfig*   config;

    private slots:
    void     copyClicked();
  };



  class FexampSimpleCount : public ParameterCount {
    public:
    FexampSimpleCount(unsigned c) : mycount(c) {};
    ~FexampSimpleCount() {};
    bool connect(ParameterSet&) { return false; }
    unsigned count() { return mycount; }
    unsigned mycount;
  };

  class FexampChannelData {
    public:
      FexampChannelData();
      ~FexampChannelData() {};
      void insert(Pds::LinkedList<Parameter>&);
      int pull(void*);
      int push(void*);
    public:
      NumericInt<uint32_t>* _reg[FexampChannel::NumberOfChannelBitFields];
  };

  class FexampASICdata {
    public:
      FexampASICdata(FexampConfig*, int);
      ~FexampASICdata() {};
      void insert(Pds::LinkedList<Parameter>&);
      int pull(void*);
      int push(void*);
    public:
      NumericInt<uint32_t>*       _reg[FexampASIC::NumberOfASIC_Entries];
      FexampSimpleCount           _count;
      FexampChannelData*          _channel[FexampASIC::NumberOfChannels];
      Pds::LinkedList<Parameter>  _channelArgs[FexampASIC::NumberOfChannels];
      FexampChannelSet            _channelSet;
      FexampConfig*               _config;
  };

  class FexampConfig : public Serializer {
  public:
    FexampConfig();
    ~FexampConfig() {};
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  public:
    NumericInt<uint32_t>*       _reg[FexampConfigType::NumberOfRegisters];
    FexampSimpleCount           _count;
    FexampASICdata*             _asic[FexampConfigType::NumberOfASICs];
    Pds::LinkedList<Parameter>  _asicArgs[FexampConfigType::NumberOfASICs];
    FexampAsicSet               _asicSet;
  };

};

#endif
