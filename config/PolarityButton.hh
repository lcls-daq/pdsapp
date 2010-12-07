#ifndef Pds_PolarityButton_hh
#define Pds_PolarityButton_hh

#include <QtGui/QPushButton>

namespace Pds_ConfigDb
{
  class PolarityButton : public QPushButton {
  public:
    enum State { None, Pos, Neg, NumberOfStates };
    PolarityButton(State s = None);
  public:
    void setState(State s);
    void nextCheckState();
    State state() const { return _state; }
  private:
    State _state;
  };
};

#endif
