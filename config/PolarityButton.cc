#include "PolarityButton.hh"

using namespace Pds_ConfigDb;

PolarityButton::PolarityButton(State s) :
  QPushButton("None")
{
  setMaximumWidth(47);
  setCheckable(true); 
  setState(s);
}

void PolarityButton::setState(State s)
{
  _state = s;
  switch(_state) {
  case None : setText("None");  break;
  case Pos  : setText("Pos" );  break;
  case Neg  : setText("Neg" );  break;
  default  :	break;
  }
  emit toggled(_state != None);
}

void PolarityButton::nextCheckState() 
{
  setState(State((_state+1)%NumberOfStates));
}
