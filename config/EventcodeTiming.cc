#include "pdsapp/config/EventcodeTiming.hh"

using namespace Pds_ConfigDb;

struct slot_s { 
  unsigned code;
  unsigned tick;
};

typedef struct slot_s slot_t;

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
                                 

unsigned EventcodeTiming::timeslot(unsigned code)
{
  unsigned n = sizeof(slots)/sizeof(slot_t);
  for(unsigned i=0; i<n; i++)
    if (slots[i].code==code)
      return slots[i].tick;
  if (code >= 67 && code <= 98)
    return timeslot(140)+code;
  if (code >= 140 && code <= 159)
    return 11850+(code-140);
  return 0;
}
