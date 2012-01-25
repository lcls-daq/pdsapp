//-----------------------------------------------------------------------------
// File          : Cspad2x2Temp.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/26/2009
// Project       : LCLS - CSPAD2X2 Detector
//-----------------------------------------------------------------------------
// Description :
// Temperature Container
//-----------------------------------------------------------------------------
// Copyright (c) 2009 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :  Modified by jackp for use in DAQ system
// 01/26/2009: created
// 2012.1.24   incorporated into DAQ config_gui
//-----------------------------------------------------------------------------
#ifndef __CSPAD2X2_TEMP_HH__
#define __CSPAD2X2_TEMP_HH__

#include <string>
#include <unistd.h>
namespace Pds_ConfigDb {

  // Calibration Data Class
  class Cspad2x2Temp {

    public:

      // Constants
      static const double coeffA = -1.4141963e1;
      static const double coeffB =  4.4307830e3;
      static const double coeffC = -3.4078983e4;
      static const double coeffD = -8.8941929e6;
      static const double t25    = 10000.0;
      static const double k0     = 273.15;
      static const double vmax   = 3.3;
      static const double vref   = 2.5;
      static const double rdiv   = 20000;

      // Temp range
      static const double minTemp = -50;
      static const double maxTemp = 150;
      static const double incTemp = 0.01;

      // Conversion table
      static const unsigned int adcCnt = 4096;
      double tempTable[adcCnt];

      // Data Container
      unsigned int adcValue;

    public:

      // Constructor
      Cspad2x2Temp ();

      // Get Temperature from adc value, deg C
      double getTemp ();

      // Return adc value for given temperature
      unsigned tempToAdc ( int temp );

  };
}

#endif
