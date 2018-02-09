/*
     __  __     ______     ______     __  __
    /\ \/ /    /\  == \   /\  __ \   /\_\_\_\
    \ \  _"-.  \ \  __<   \ \ \/\ \  \/_/\_\/_
     \ \_\ \_\  \ \_____\  \ \_____\   /\_\/\_\
       \/_/\/_/   \/_____/   \/_____/   \/_/\/_/

  The MIT License
  Copyright (c) 2018 Ronnie Zeiller ronnie@zeiller.eu
  Copyright (c) 2018 Thomas Sarlandie thomas@sarlandie.net

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

// TODO: Performance class has to have access to PerformanceConfig

// Performance class calculates values and gives them back to keep the class
// independent from the calling function

#include <KBoxLogging.h>
#include "host/config/KBoxConfig.h"
#include "Performance.h"
#include "common/signalk/SKUnits.h"

// ****************************************************************************
// Load correction table from SD-Card and correct boat speed
// ****************************************************************************
bool Performance::corrForNonLinearTransducer(double &bs_kts, double &heel) {


  // If heel is more than _maxForHeelCorrectionBoatSpeed then take
  // heeled correction values

  return true;
}

// ****************************************************************************
// param boatspeed in m/s which we have to convert, because Leeway formula
// needs knots.
// ****************************************************************************
bool Performance::calcBoatSpeed(double &boatspeed, double &heel, double &leeway) {

  double bs_kts = SKMsToKnot(boatspeed);
  corrForNonLinearTransducer(bs_kts, heel);

  // Correct for Leeway
  if (calcLeeway(bs_kts, heel, leeway) && (leeway < M_PI_2) && (leeway > (-1)* M_PI_2)) {

    boatspeed = abs(SKKnotToMs(bs_kts / cos(leeway)));
    return true;
  } else {
    return false;
  }
}

// ****************************************************************************
// Leeway is an angle of drift due to sidewards wind force
// it is depending of a hull-factor (given in config), actual heel and boat speed
// Leeway angle here in this function is always positiv, as we do not know from
// which direction the wind is coming
// ****************************************************************************
bool Performance::calcLeeway(double &bs_kts, double &heel, double &leeway) {

  double leewayHullFactor = 10 * heel / 10.0; // kboxConfig.performanceConfig.leewayHullFactor * heel / 10.0;
  // DEBUG("kBoxConfig leewayHullFactor: %f", leewayHullFactor);

  if (bs_kts > 0) {
    leeway = leewayHullFactor / (bs_kts * bs_kts);
    return true;
  } else {
    return true;
  }
}
