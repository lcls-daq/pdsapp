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
      static const double coeffA;
      static const double coeffB;
      static const double coeffC;
      static const double coeffD;
      static const double t25;
      static const double k0;
      static const double vmax;
      static const double vref;
      static const double rdiv;

      // Temp range
      static const double minTemp;
      static const double maxTemp;
      static const double incTemp;

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
