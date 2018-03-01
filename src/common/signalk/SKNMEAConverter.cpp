/*
     __  __     ______     ______     __  __
    /\ \/ /    /\  == \   /\  __ \   /\_\_\_\
    \ \  _"-.  \ \  __<   \ \ \/\ \  \/_/\_\/_
     \ \_\ \_\  \ \_____\  \ \_____\   /\_\/\_\
       \/_/\/_/   \/_____/   \/_____/   \/_/\/_/

  The MIT License

  Copyright (c) 2017 Thomas Sarlandie thomas@sarlandie.net

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

#include <KBoxLogging.h>
#include "SKUnits.h"
#include "common/nmea/NMEASentenceBuilder.h"

#include "SKNMEAConverter.h"

static double sog;
static double cog;

void SKNMEAConverter::convert(const SKUpdate& update, SKNMEAOutput& output) {
  String talkerID;

  // Trigger a call of visitSKElectricalBatteriesVoltage for every key with that path
  // (there can be more than one and we do not know how they are called)
  _currentOutput = &output;
  if (_config.xdrBattery) {
    update.accept(*this, SKPathElectricalBatteriesVoltage);
  }

  if (update.hasEnvironmentWindDirectionTrue()){}
  if (update.hasEnvironmentWindSpeedTrue()){}
  if (update.hasNavigationCourseOverGroundTrue()){ cog = update.hasNavigationCourseOverGroundTrue(); }
  if (update.hasNavigationMagneticVariation()){}
  if (update.hasNavigationPosition()){}
  if (update.hasNavigationSpeedOverGround()){ sog = update.getNavigationSpeedOverGround(); }
  if (update.hasNavigationSpeedThroughWater()){}
  if (update.hasNavigationTripLog()){}
  if (update.hasNavigationLog()){}
  if (update.hasPerformanceLeeway()){}

  if (_config.xdrPressure && update.hasEnvironmentOutsidePressure()) {
    if (_config.propTalkerIDEnabled &&
        update.getSource().getInput() == SKSourceInputKBoxBarometer) {
      talkerID =  _config.talkerID;
      DEBUG("Talker-ID: %s",talkerID);
    } else {
      talkerID = "II";
    }
    NMEASentenceBuilder sb(talkerID, "XDR", 4);
    sb.setField(1, "P");
    sb.setField(2, SKPascalToBar(update.getEnvironmentOutsidePressure()), 5);
    sb.setField(3, "B");
    sb.setField(4, "Barometer");
    output.write(sb.toNMEA());
  }

  if (_config.xdrAttitude && update.hasNavigationAttitude()) {
    // check if update is coming from internal sensor
    // DEBUG("%i",update.getSource().getInput());
    if (_config.propTalkerIDEnabled &&
        update.getSource().getInput() == SKSourceInputKBoxIMU) {
      talkerID =  _config.talkerID;
    } else {
      talkerID = "II";
    }
    NMEASentenceBuilder sb(talkerID, "XDR", 8);
    sb.setField(1, "A");
    if (update.getNavigationAttitude().pitch == SKDoubleNAN) {
      sb.setField(2, "");
    } else {
      sb.setField(2, SKRadToDeg(update.getNavigationAttitude().pitch), 1);
    }
    sb.setField(3, "D");
    sb.setField(4, "PTCH");
    sb.setField(5, "A");
    if (update.getNavigationAttitude().roll == SKDoubleNAN) {
      sb.setField(6, "");
    } else {
      sb.setField(6, SKRadToDeg(update.getNavigationAttitude().roll), 1);
    }
    sb.setField(7, "D");
    sb.setField(8, "ROLL");
    output.write(sb.toNMEA());
  }

  if (_config.hdm && update.hasNavigationHeadingMagnetic()) {
    if (_config.propTalkerIDEnabled &&
        update.getSource().getInput() == SKSourceInputKBoxIMU) {
      talkerID =  _config.talkerID;
    } else {
      talkerID = "II";
    }
    NMEASentenceBuilder sb(talkerID, "HDM", 2);
    sb.setField(1, SKRadToDeg(update.getNavigationHeadingMagnetic()), 1);
    sb.setField(2, "M");
    output.write(sb.toNMEA());
  }

  //  ***********************************************
  //    RSA Rudder Sensor Angle
  //    Talker-ID: AG - Autopilot general
  //    also seen: ERRSA
  //    Expedition: IIXDR
  //
  //            1  2  3  4
  //            |  |  |  |
  //    $--RSA,x.x,A,x.x,A*hh
  //      1) Starboard (or single) rudder sensor, "-" means Turn To Port
  //      2) Status, A means data is valid
  //      3) Port rudder sensor
  //      4) Status, A means data is valid
  // *********************************************** */
  if (_config.rsa && update.hasSteeringRudderAngle()) {
    NMEASentenceBuilder sb("II", "RSA", 4);
    sb.setField(1, SKRadToDeg(SKNormalizeAngle(update.getSteeringRudderAngle())), 1 );
    sb.setField(2, "A");
    sb.setField(3, "");
    sb.setField(4, "");
    output.write(sb.toNMEA());
  }

  /* ****************************************************************************
        DBT Depth Below Transducer
        ==========================
                1  2  3  4  5  6
                |  |  |  |  |  |
        $--DBT,x.x,f,x.x,M,x.x,F*hh
        1) Depth, feet 2) f = feet
        3) Depth, meters 4) M = meters
        5) Depth, Fathoms 6) F = Fathoms
  *************************************************************************** */
  if (_config.dbt && update.hasEnvironmentDepthBelowTransducer()) {
    NMEASentenceBuilder sb("II", "DBT", 6);
    sb.setField(1, "");
    sb.setField(2, "f");
    sb.setField(3, update.getEnvironmentDepthBelowTransducer(), 1);
    sb.setField(4, "M");
    sb.setField(5, "");
    sb.setField(6, "F");
    output.write(sb.toNMEA());
  }


  if (_config.mwv && update.hasEnvironmentWindAngleApparent()
      && update.hasEnvironmentWindSpeedApparent()) {
    generateMWV(output, update.getEnvironmentWindAngleApparent(), update.getEnvironmentWindSpeedApparent(), true);
  }

  if (_config.mwv && update.hasEnvironmentWindAngleTrueWater()
      && update.hasEnvironmentWindSpeedTrue()) {
    generateMWV(output, update.getEnvironmentWindAngleTrueWater(), update.getEnvironmentWindSpeedTrue(), false);
  }

  //  ***********************************************
  //  New NMEA 0183 sentence Leeway
  //  https://www.nmea.org/Assets/20170303%20nautical%20leeway%20angle%20measurement%20sentence%20amendment.pdf
  //         1  2
  //         |  |
  //  $--LWY,A,x.x*hh<CR><LF>
  //      1) Valid or not A/V
  //      2) leeway in degrees and decimal degrees, positiv slipping to starboard
  //  ***********************************************
  //  TODO!
}

void SKNMEAConverter::visitSKElectricalBatteriesVoltage(const SKUpdate& u, const SKPath &p, const SKValue &v) {
  NMEASentenceBuilder sb("II", "XDR", 4);
  sb.setField(1, "V");
  sb.setField(2, v.getNumberValue(), 2);
  sb.setField(3, "V");
  sb.setField(4, p.getIndex());

  _currentOutput->write(sb.toNMEA());
}

// ***********************  Wind Speed and Angle  ************************
//  Talker ID: WI Weather Instruments
//             VW Velocity Sensor, Mechanical?
//  also seen: IIVWR
//
//
//  NMEA0183  MWV Wind Speed and Angle  --> recommended
//              1  2  3  4 5
//              |  |  |  | |
//      $--MWV,x.x,a,x.x,a,A*hh
//  1) Wind Angle, 0 to 360 degrees
//  2) Reference, R = Relative, T = True
//  3) Wind Speed
//  4) Wind Speed Units: K = km/s, M = m/s, N = Knots
//  5) Status, A = Data Valid
// ************************************************************************
void SKNMEAConverter::generateMWV(SKNMEAOutput &output, double windAngle, double windSpeed, bool apparent) {
  NMEASentenceBuilder sb( "II", "MWV", 5);
  sb.setField(1, SKRadToDeg(SKNormalizeDirection(windAngle)), 1);
  sb.setField(2, apparent ? "R" : "T");
  sb.setField(3, windSpeed, 2 );
  sb.setField(4, "M");
  sb.setField(5, "A");
  output.write(sb.toNMEA());
}
