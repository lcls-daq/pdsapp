//-----------------------------------------------------------------------------
// File          : Cspad2x2Temp.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/26/2010
// Project       : LCLS - CSPAD2X2 Detector
//-----------------------------------------------------------------------------
// Description :
// Temperature Container
//-----------------------------------------------------------------------------
// Copyright (c) 2009 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :  Modified by jackp for use in DAQ system
// 01/26/2010: created
// 2012.1.24   incorporated into DAQ config_gui
//-----------------------------------------------------------------------------
#include <unistd.h>
#include <math.h>
#include "pdsapp/config/Cspad2x2Temp.hh"

using namespace Pds_ConfigDb;

// Constants
const double Cspad2x2Temp::coeffA = -1.4141963e1;
const double Cspad2x2Temp::coeffB =  4.4307830e3;
const double Cspad2x2Temp::coeffC = -3.4078983e4;
const double Cspad2x2Temp::coeffD = -8.8941929e6;
const double Cspad2x2Temp::t25    = 10000.0;
const double Cspad2x2Temp::k0     = 273.15;
const double Cspad2x2Temp::vmax   = 3.3;
const double Cspad2x2Temp::vref   = 2.5;
const double Cspad2x2Temp::rdiv   = 20000;

// Temp range
const double Cspad2x2Temp::minTemp = -50;
const double Cspad2x2Temp::maxTemp = 150;
const double Cspad2x2Temp::incTemp = 0.01;

// Constructor
Cspad2x2Temp::Cspad2x2Temp() {
   double       temp;
   double       tk;
   double       res;
   double       volt;
   unsigned     idx;

   temp = minTemp;
   while ( temp < maxTemp ) {
      tk = k0 + temp;
      res = t25 * exp(coeffA+(coeffB/tk)+(coeffC/(tk*tk))+(coeffD/(tk*tk*tk)));      
      volt = (res*vmax)/(rdiv+res);
      idx = (int)((volt / vref) * (double)(adcCnt-1));
      if ( idx < adcCnt ) tempTable[idx] = temp; 
      temp += incTemp;
   }
}

// Get Temperature from adc value, deg C
double Cspad2x2Temp::getTemp() {
   if ( adcValue < adcCnt) return(tempTable[adcValue]);
   else return(0);
}

// Return adc value for given temperature
unsigned Cspad2x2Temp::tempToAdc( int t ) {
   unsigned x;

   //first set values for outside the range
   if (t > 150) x = 50;
   else if (t<-12) x = 4095;
   //then hardwire the zero return where the algorithm diverges
   else if (t==0) x = 3355;
   //if none of the above, then look up the answer
   else for ( x=50; x < adcCnt; x++) {
      if ( t == (int)tempTable[x] ) break;
   }
   //And adjust it to minimize the error in the -12 C to 40 C range
   if ((t>0)&&(t<74)) x += 66;
   if ((t>8)&&(t<74)) x -= t-9;

   return(x);
}

