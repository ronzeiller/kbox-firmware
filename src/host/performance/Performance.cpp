/*
     __  __     ______     ______     __  __
    /\ \/ /    /\  == \   /\  __ \   /\_\_\_\
    \ \  _"-.  \ \  __<   \ \ \/\ \  \/_/\_\/_
     \ \_\ \_\  \ \_____\  \ \_____\   /\_\/\_\
       \/_/\/_/   \/_____/   \/_____/   \/_/\/_/

  Project:  KBox
            Copyright (c) 2018 Thomas Sarlandie thomas@sarlandie.net
  Purpose:  Performance Calculations on KBox
  Author:   Ronnie Zeiller ronnie@zeiller.eu
  *****************************************************************************

  The MIT License
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
/*
  The Performance class is a service (Task) started in main.cpp
  When there is an update for heel coming, the value will be stored.
  When there is an update for boat speed through water coming we calculate
  first leeway and second correct boat speed with cosine of leeway angle.

  TODO:
  1. extend config to choose from where heel is taken (N2k Bus, internal sensor
      or NMEA0183 sensor on serial input)
  2. read correction file for boatspeed offsets for upright and heeled sailing
  3. extend class for corrections on Apparent Wind and calculations of True Wind

  Thanks to D. Pedrick and Richard McCurdy for their Yacht Performance analysis with computers 1981
*/

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "Performance.h"

#include <KBoxLogging.h>
#include "host/config/KBoxConfig.h"
#include "common/signalk/SKUnits.h"
#include "host/config/PerformanceConfig.h"
//#include <SdFat.h>
//#include "Polar.h"

Performance::Performance(PerformanceConfig &config) : _config(config) {

  _leeway = SKDoubleNAN;
  _heel = SKDoubleNAN;
  _bs_kts_corr = SKDoubleNAN;

  //Polar polar(config);
}

// ****************************************************************************
//  Calculate Boat Speed by applying calibration and count for heel
// ****************************************************************************
double Performance::calcBoatSpeed(double &bs_kts, double &heel, double &leeway) {

  _bs_kts_corr = getCorrForNonLinearTransducer(bs_kts, heel);

  // Correct measured speed for Leeway
  _bs_kts_corr = abs(_bs_kts_corr / cos(leeway));

  DEBUG("Boatspeed from Transducer: %.3f kts--> Corrected Boatspeed: %.3f kts", bs_kts, _bs_kts_corr);

  return _bs_kts_corr;
}

// ****************************************************************************
// Leeway is an angle of drift due to sidewards wind force
// it is depending of a hull-factor (given in config), actual heel and boat speed
// Leeway angle positiv or negative, depending from where the wind is coming.
// if heel is positive => wind from port => leeway positive (but AWA will be negative!!)
// ****************************************************************************
double Performance::getLeeway(double &bs_kts, double &heel) {

  // when there is no leewayHullFactor entered in config then it will be 0 by default
  double leewayHullFactor = _config.leewayHullFactor / 10.0;
  double lw = 0;
  // DEBUG("kBoxConfig leewayHullFactor: %f", leewayHullFactor);

  lw = leewayHullFactor * heel / (bs_kts * bs_kts);
  DEBUG("Leeway : %f°", SKRadToDeg(lw));

  // Validation check
  // Leeway with more than 20° (0.349rad) is probably wrong!
  if (lw < 0.349 && lw > (-0.349)){
    return lw;
  } else {
    return 0;
  }
}

void Performance::calcApparentWind(double &aws_m, double &awa_m, double &heel){

  double awa_corr, aws_corr;
  // TODO: check for pos/neg values
  awa_corr = atan2(tan(awa_m), cos(abs(heel)));

  // Correction measurement for heel
  aws_corr = aws_m * (cos(awa_m)/cos(awa_corr));

  // Correction measurement from sensor height to 10m above water as needed
  // for ORC performance calculations
  // Is there any sensorheight (in mm) more than 10m in config?
  if (_config.windCorr10m &&
      _config.windSensorHeight > 10000) {
    aws_corr = aws_corr * (1 / (0.9 + 0.1 * _config.windSensorHeight / 1000));
  }
  /*
  SKUpdateStatic<2> updateWrite;
  SKSource source = SKSource::performanceCalc();
  updateWrite.setSource(source);
  updateWrite.setTimestamp(now());
  updateWrite.setEnvironmentWindSpeedApparent(aws_corr);
  DEBUG("AWS sensor: %.3f --> AWS corrected: %.3f kts", aws_m, aws_corr);
  updateWrite.setEnvironmentWindAngleApparent(awa_corr);
  DEBUG("AWA sensor: %.3f --> AWA corrected: %.3f kts", awa_m, awa_corr);
  _hub.publish(updateWrite);
  */
}

// ****************************************************************************
// Load correction table from SD-Card and correct boat speed
// ****************************************************************************
double Performance::getCorrForNonLinearTransducer(double &bs_kts, double &heel) {

  // TODO: read file with correction values for boat speed transducer.
  // If heel is more than _maxForHeelCorrectionBoatSpeed then take
  // heeled correction values
  // double heelDeg = SKRadToDeg(heel);

  return bs_kts;
}
