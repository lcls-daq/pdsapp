#include "pdsapp/config/EventcodeTiming.hh"

#include <stdint.h>

struct slot_s { 
  unsigned code;
  unsigned tick;
};

typedef struct slot_s slot_t;

static const unsigned EvrClkRate = 119000000;

/**
 **  Found timing moved sometime before Apr 25, 2013
 **
static const unsigned tevcode140 = 11850;
static const unsigned tucodebase = 11900;

static const slot_t slots[] = { 
  {   9, 12900 },
  {  10, 12951 },
  {  11, 12961 },
  {  12, 12971 },
  {  13, 12981 },
  {  14, 12991 },
  {  15, 13001 },
  {  16, 13011 },
  {  40, 12954 },
  {  41, 12964 },
  {  42, 12974 },
  {  43, 12984 },
  {  44, 12994 },
  {  45, 13004 },
  {  46, 13014 },
  { 162, 11840 }
};
**
**  Timing at Apr 25, 2013
*/
static const unsigned tevcode140 = 11900;
static const unsigned tucodebase = 11900;

static const slot_t slots[] = { 
  {   9, 12950 },
  {  10, 13001 },
  {  11, 13011 },
  {  12, 13021 },
  {  13, 13031 },
  {  14, 13041 },
  {  15, 13051 },
  {  16, 13061 },
  {  40, 13004 },
  {  41, 13014 },
  {  42, 13024 },
  {  43, 13034 },
  {  44, 13044 },
  {  45, 13054 },
  {  46, 13064 },
  { 162, 11890 },
  { 163, 11922 }
};
                                 

unsigned Pds_ConfigDb::EventcodeTiming::timeslot(unsigned code)
{
  unsigned n = sizeof(slots)/sizeof(slot_t);
  for(unsigned i=0; i<n; i++)
    if (slots[i].code==code)
      return slots[i].tick;
  if (code >= 140 && code <= 161)
    return tevcode140+(code-140);
  if (code >= 67 && code <= 98)
    return tucodebase+code;
  if (code >= 167 && code <= 198)
    return tucodebase+code;
  return 0;
}

unsigned Pds_ConfigDb::EventcodeTiming::period(unsigned code)
{
  switch(code) {
  case 40:
  case 140: return EvrClkRate/120;
  case 41:
  case 141: return EvrClkRate/ 60;
  case 42:
  case 142: return EvrClkRate/ 30;
  case 43:
  case 143: return EvrClkRate/ 10;
  case 44:
  case 144: return EvrClkRate/  5;
  case 45:
  case 145: return EvrClkRate;
  case 46:
  case 146: return EvrClkRate*  2;
  default:  return unsigned(-1);
  }
}
