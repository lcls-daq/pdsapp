#ifndef PdsConfigDb_UxiTimingConfig_hh
#define PdsConfigDb_UxiTimingConfig_hh

#include "pds/config/UxiConfigType.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/QtChoiceConcealer.hh"

#include <QtCore/QObject>

class QSignalMapper;

namespace Pds_ConfigDb {
  class UxiTimingConfig : public QObject, public Parameter {
    Q_OBJECT
  public:
    UxiTimingConfig(bool expert_mode);
    ~UxiTimingConfig();
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    QLayout* initialize(QWidget* parent);
    void pull(const UxiConfigType& cfg);
    UxiConfigType::TimingMode timingMode() const;
    const uint32_t* timing();
    void update();
    void flush ();
    void enable(bool v);
    bool validate();
  private slots:
    void copyTiming(int modeside);
  private:
    QLayout* initializeLabel(QWidget* p, QSignalMapper* signalMapper, unsigned iside, UxiConfigType::TimingMode mode);
    bool _expert_mode;
    Pds::LinkedList<Parameter> _pList;
    QtChoiceConcealer _concealerTiming;
    Enumerated<UxiConfigType::TimingMode> _timingModeParam;
    NumericInt<uint32_t>* _timeOnSideParam[UxiConfigNumberOfSides];
    NumericInt<uint32_t>* _timeOffSideParam[UxiConfigNumberOfSides];
    NumericInt<uint32_t>* _delaySideParam[UxiConfigNumberOfSides];
    NumericIntArray<uint32_t, UxiConfigMaxManualShutterSequence>* _manualShutterSideParam[UxiConfigNumberOfSides];
    NumericIntArray<uint32_t, UxiConfigMaxTimingSequence>* _arbitraryTimingSideParam[UxiConfigNumberOfSides];
    uint32_t _timing[UxiConfigNumberOfSides * UxiConfigMaxTimingSequence];
    static const int SHUTTER_ITEM_WIDTH = 90;
    static const int TIMING_ITEM_WIDTH = 40;
    static const uint32_t MIN_SHUTTER_TIME = 75;
    static const uint32_t MAX_SHUTTER_TIME = 2000000000;
    static const uint32_t SHUTTER_MULTIPLIER = 25;
    static const char*  sideNames[];
    static const char*  timingModeNames[];
  };
}

#endif
