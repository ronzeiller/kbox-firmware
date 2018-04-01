/*
     __  __     ______     ______     __  __
    /\ \/ /    /\  == \   /\  __ \   /\_\_\_\
    \ \  _"-.  \ \  __<   \ \ \/\ \  \/_/\_\/_
     \ \_\ \_\  \ \_____\  \ \_____\   /\_\/\_\
       \/_/\/_/   \/_____/   \/_____/   \/_/\/_/

  Project  :  KBox
              Copyright (c) 2018 Thomas Sarlandie thomas@sarlandie.net
  Purpose  :  Central Data Hub in KBox
  Author(s):  Thomas Sarlandie thomas@sarlandie.net, Ronnie Zeiller ronnie@zeiller.eu
  *********************************************************************************

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
    Jeder Eingang liefert letztendlich ein SKUpdate, das hier im SKHub landet.
    Über die Funktion publish werden diese SKUpdates an alle Subscriber verteilt.

    D.h. die Service Class bekommt als Parameter SKHub übergeben und vom lokalen
    Objekt _hub übernommen.
    Die Subscription erfolgt über den Aufruf: _hub.subscribe(this);
    Womit der Service in die Liste der Subscriber aufgenommen wird.

    New list which subscribers can subscribe to: _filteredSubscribers
    This updates could be used for displays, pages, navigation,....
    Here IIR filtered datas with _displayInterval (e.g. 1Hz) will be sent like:
      - Heel
      - Additionally datas which originally come in 1 Hz like:
          Boat Speed (more coming soon)

*/

#include <time.h>
#include <KBoxLogging.h>
#include "SKHub.h"
#include "SKSubscriber.h"
#include "SKUpdate.h"
#include "SKUnits.h"
#include "SKUpdateStatic.h"

SKHub::SKHub() {

  _awsFilter.setType(IIRFILTER_TYPE_LINEAR);
  _awsFilter.setFC(_dampingFactor10Hz);
  _awaFilter.setType(IIRFILTER_TYPE_RAD);
  _awaFilter.setFC(_dampingFactor10Hz);
  _heelFilter.setType(IIRFILTER_TYPE_RAD);
  _heelFilter.setFC(_dampingFactor10Hz);
  _heel = SKDoubleNAN;
  _cog  = SKDoubleNAN;
  _sog  = SKDoubleNAN;
  _hdgMag = 2 * M_PI + 0.1;   // over 360° not valid
  _hdgTrue = 2 * M_PI + 0.1;  // ditto
  _variationMagn = 0;
  _lat = 0;
  _lon = 0;

}

SKHub::~SKHub() {
}

void SKHub::subscribe(SKSubscriber* subscriber) {
  _subscribers.add(subscriber);
}

void SKHub::subscribeFiltered(SKSubscriber* subscriber) {
  _filteredSubscribers.add(subscriber);
}

/*
 *    Services which get values from input sources, or
 *    Sensors which produce values send their datas in SKUpdate format
 *    to SKHub by publish(SKUpdate)
 *
 *    Some of the values are High Speed Values with 10Hz just needed for calculations.
 *    They will run through a filter and will be sent in 1Hz to filteredSubscribers only.
 *
 *    Other Subscribers subscribe to NMEA0183 sentences and will get informed if
 *    new values are on the Hub.
 */
void SKHub::publish(const SKUpdate& update) {

  Performance performance(_kboxConfig.performanceConfig);

  //bool sendHighSpeedUpdate = true;
  bool sendLowSpeedUpdate = true;   // by default all values are sent if not identified as high speed values
  bool sendFilteredUpdate = false;
  bool sendPerformanceUpdate = false;

  // We need minimum 4 updates for combined values in NMEA0183 sentences
  SKUpdateStatic<4> updatePerf;
  SKSource source = SKSource::performanceCalc();
  updatePerf.setSource(source);
  updatePerf.setTimestamp(now());

  // SKUpdate for display filtered Values
  SKUpdateStatic<6> updateFiltered;
  // TODO: may be wise to make a new source for filtered values?
  updateFiltered.setSource(source);

  // All KBox config settings are available here, e.g.:
  //if (_kboxConfig.serial1Config.repeatSentence) {}

  /* *************************************************************
  // If calcTrueWind is set by performance config we discard all True Wind values,
  // as we do not know how they are calculated!
   * *********************************************************** */
  if (_kboxConfig.performanceConfig.calcTrueWind) {
    if (update.hasEnvironmentWindDirectionTrue() ||
        update.hasEnvironmentWindSpeedTrue()){
      return;
    }
  }

  /* *************************************************************
  // Wind direction and wind speed are assumed to come from NMEA2000 with around 8 to 10 Hz
  // we store the value and we filter (damp) the value in IIR-filter
  // Calculations are done with high speed values
   * *********************************************************** */
  if (update.hasEnvironmentWindSpeedApparent() &&
      update.hasEnvironmentWindAngleApparent()){        // Wind speed and angle are coming in one update
    _aws = update.getEnvironmentWindSpeedApparent();
    _awa = update.getEnvironmentWindAngleApparent();    // AWA pos coming from starboard, neg from port, relative to centerline vessel

    sendLowSpeedUpdate = false;  // LowSpeed Subscribers do not get all incoming values

    if (_kboxConfig.performanceConfig.enabled) {
      // now calculate corrected wind Values. We should have an actual value for heel
      performance.calcApparentWind(_aws, _awa, _heel);
      // AWS Apparent Wind Speed
      updatePerf.setEnvironmentWindSpeedApparent(_aws);
      // AWA pos coming from starboard, neg from port, relative to centerline vessel
      updatePerf.setEnvironmentWindAngleApparent(SKNormalizeAngle(_awa));
      // send calculated performance update instead of update with incoming datas
      //TODO: calculate True Wind, but for this we need COG, SOG, boat speed vector, course
      sendPerformanceUpdate = true;
    }

    _awsFiltered = _awsFilter.filter( round(_aws * 100)/100.0 );
    _awaFiltered = _awaFilter.filter( round(_awa * 100)/100.0 );

    // it is assumed that wind values are coming with 8 to 10Hz
    if ( _timeSinceLastWind > _displayInterval){
      DEBUG("Wind Aktuell --> Filtered: AWA %.2f° --> %.2f° // AWS %.2fkts --> %.2fkts",
            SKRadToDeg(_awa), SKRadToDeg(_awaFiltered), SKMsToKnot(_aws), SKMsToKnot(_awsFiltered));
      updateFiltered.setTimestamp(now());
      updateFiltered.setEnvironmentWindSpeedApparent(_awsFiltered);
      updateFiltered.setEnvironmentWindAngleApparent(_awaFiltered);
      _timeSinceLastWind = 0;
      sendFilteredUpdate = true;
    }
  }
  /* *************************************************************
  // For calculating corrected boat speed, apparent and true wind etc. we need
  // heel values from IMUService or external IMU-sensor, which should come with
  // around 10Hz.
  // For heel we make filtered values too.
   * *********************************************************** */
  if (update.hasNavigationAttitude() &&
      update.getNavigationAttitude().roll != SKDoubleNAN){

    sendLowSpeedUpdate = false;  // LowSpeed Subscribers do not get all incoming values
    _heel = update.getNavigationAttitude().roll;
    //DEBUG("SKUpdate --> Heel: %f °", SKRadToDeg(_heel));

    _heelFiltered = _heelFilter.filter(_heel);
    //_heelFiltered = _heel;

    if (_timeSinceLastHeel > _displayInterval){
      // DEBUG("Time since last Heel: %d, Heel: %.1f", millis(), SKRadToDeg(_heelFiltered));
      updateFiltered.setTimestamp(now());
      // for pitch we take the unfiltered actual value, as pitch is not needed at this time,
      // but we do not want empty NMEA sentence later on.
      updateFiltered.setNavigationAttitude(SKTypeAttitude( _heelFiltered,
                                                          update.getNavigationAttitude().pitch,
                                                          SKDoubleNAN));
      _timeSinceLastHeel = 0;
      sendFilteredUpdate = true;
    }
  }

  /* *************************************************************
  // On most systems boatspeed will come with 1Hz
  // So we take update of boat speed to trigger all performance calculations
  // SpeedThroughWater is so far the uncorrected measured raw value,
  // which we have to correct by calibration values and leeway!
   * *********************************************************** */
  if (update.hasNavigationSpeedThroughWater() &&
      update.getNavigationSpeedThroughWater() > 0) {

    if (_kboxConfig.performanceConfig.enabled && _heel != SKDoubleNAN) {
      // SKUpdate is in m/s --> we need knots for formulas
      double bs_kts_m = SKMsToKnot(update.getNavigationSpeedThroughWater());
      //TODO: if leeway is max due to too low speed and/or too high heel, may be SKDoubleNAN should be sent
      // and following no value in NMEA0183?
      _leeway = performance.getLeeway(bs_kts_m, _heel);
      //DEBUG("Leeway : %f°", SKRadToDeg(_leeway));
      _stwCorr = SKKnotToMs(performance.calcBoatSpeed(bs_kts_m, _heel, _leeway));
      DEBUG("boat Speed Through Water corrected: %.2fkts --> %.2fkts", bs_kts_m, SKMsToKnot(_stwCorr));
      // change value in update to corrected one
      updatePerf.setNavigationSpeedThroughWater(_stwCorr);
      updatePerf.setPerformanceLeeway(_leeway);

      // add Heading True/Mag  [rad] from rapid PGN to this update so it is possible to convert
      // to NMEA0183 VHW with STW + HDG
      if (_hdgMag <= 2 * M_PI){
        updatePerf.setNavigationHeadingMagnetic(_hdgMag);
      }
      if (_hdgTrue <= 2 * M_PI){
        updatePerf.setNavigationHeadingTrue(_hdgTrue);
      }
      sendPerformanceUpdate = true;

      // to block normal update for low speed
      sendLowSpeedUpdate = false;
      updateFiltered.setTimestamp(now());
      updateFiltered.setNavigationSpeedThroughWater(_stwCorr);
      updateFiltered.setPerformanceLeeway(_leeway);
      sendFilteredUpdate = true;
    }
  }

  /* *************************************************************
  //  For RMC NMEA0183 sentence we need:
  //  129025 Pos. Rapid Update + 129026 COG & SOG Rapid + 127258 Variation
  //  126992 System Time / Date (Trigger)
  //  But at the moment we have no Signal K for Time......
  //  in the meantime I take (because I will never need....):
  //  EnvironmentOutsideApparentWindChillTemperature --> Days since 1970-01-01
  //  EnvironmentWaterTemperature --> seconds since midnight with 2 digits
  //  TODO: change as soon as we have SKUpdate for Time
   * *********************************************************** */
   if (update.hasEnvironmentOutsideApparentWindChillTemperature()){
     _daysSince1970 = update.getEnvironmentOutsideApparentWindChillTemperature();
   }
   if (update.hasEnvironmentWaterTemperature()){
     _secondsSinceMidnight = update.getEnvironmentWaterTemperature();

     updateFiltered.setTimestamp(now());
     updateFiltered.setEnvironmentOutsideApparentWindChillTemperature(_daysSince1970);
     updateFiltered.setEnvironmentWaterTemperature(_secondsSinceMidnight);

     // add actual COG and SOG
     updateFiltered.setNavigationCourseOverGroundTrue(_cog);
     updateFiltered.setNavigationSpeedOverGround(_sog);

     // add actual Lat and Lon
     updateFiltered.setNavigationPosition(SKTypePosition(_lat, _lon, 0));

     // add variation
     updateFiltered.setNavigationMagneticVariation(_variationMagn);

     sendFilteredUpdate = true;
   }


  /* *************************************************************
  //  We take from PGN 129026 COG & SOG, Rapid Update
  //  Should come with around 10Hz
   * *********************************************************** */
  if (update.hasNavigationCourseOverGroundTrue()){
    _cog = update.getNavigationCourseOverGroundTrue();
    //DEBUG("COG = %f°", SKRadToDeg(cog));
  }

  if (update.hasNavigationSpeedOverGround()){
    _sog = update.getNavigationSpeedOverGround();
    //DEBUG("SOG = %f ktn", SKMsToKnot(sog));
  }

  if (update.hasNavigationHeadingMagnetic()){
    _hdgMag = update.getNavigationHeadingMagnetic();
  }

  if (update.hasNavigationHeadingTrue()){
    _hdgTrue = update.getNavigationHeadingTrue();
  }

  if (update.hasNavigationPosition()){
    _lat = 0;
    _lon = 0;
  }

  if (update.hasNavigationMagneticVariation()){
    _variationMagn = update.getNavigationMagneticVariation();
  }

  // TODO

  if (update.hasNavigationTripLog()){}
  if (update.hasNavigationLog()){}


  /* *************************************************************
  // Filtered Updates for Displays,  Low Speed Updates
   * *********************************************************** */
    for (LinkedListIterator<SKSubscriber*> it = _filteredSubscribers.begin(); it != _filteredSubscribers.end(); it++) {
    if ( sendLowSpeedUpdate ) {
      // send original datas from input (if not identified as high frequently coming in)
      (*it)->updateReceived(update);
    }
    if ( sendFilteredUpdate ) {
      // send updates filtered for display interval
      (*it)->updateReceived(updateFiltered);
    }
  }

  // High speed subscribers get all updates as usual
  //if (sendHighSpeedUpdate){
    for (LinkedListIterator<SKSubscriber*> it = _subscribers.begin(); it != _subscribers.end(); it++) {
      if ( !sendPerformanceUpdate ) {
        (*it)->updateReceived(update);
      } else {
        (*it)->updateReceived(updatePerf);
      }
    }
  //}
}

/* --> IIR FILTER FÜR HEEL aus alter IMUPage Version, die funktionierte!!!
if ( up.hasNavigationHeadingMagnetic() ) {
    const SKValue& vm = up.getNavigationHeadingMagnetic();
    _IMU_HDGFiltered = IMU_HdgFilter.filter( round ( SKRadToDeg( vm.getNumberValue()) * 10) / 10.0 );
    //DEBUG("HDG: %f\n", round( SKRadToDeg( vm.getNumberValue()) * 10) / 10.0 );
  }

  if ( up.hasNavigationAttitude() ) {
    const SKValue& vm = up.getNavigationAttitude();
    _IMU_HeelFiltered = IMU_HeelFilter.filter( round( SKRadToDeg( vm.getAttitudeValue().roll ) * 10) / 10.0 );
    _IMU_PitchFiltered = IMU_PitchFilter.filter( round( SKRadToDeg( vm.getAttitudeValue().pitch )* 10) / 10.0 );
    //DEBUG("Heel: %f\n", round( SKRadToDeg( vm.getAttitudeValue().roll ) * 10) / 10.0);
    //DEBUG("Pitch: %f\n", round( SKRadToDeg( vm.getAttitudeValue().pitch )* 10) / 10.0);
  }


  // Display interval 400ms
  if (millis() > _nextPrint) {
    _nextPrint = millis() + 400;


    //DEBUG("%f\n", _IMU_HDGFiltered);
    //DEBUG("%f\n",_IMU_PitchFiltered);
    //DEBUG("%f\n",_IMU_HeelFiltered);
    if ( _IMU_HDGFiltered < 361 && _IMU_HDGFiltered >= 0 )
      hdgTL->setText(String( _IMU_HDGFiltered, 1) + "°        ");
    if ( _IMU_PitchFiltered < 181 && _IMU_PitchFiltered > -181 )
      pitchTL->setText(String( _IMU_PitchFiltered, 1 ) + "°     ");
    if ( _IMU_HeelFiltered < 181 && _IMU_HeelFiltered > -181 )
      heelTL->setText(String( _IMU_HeelFiltered, 1 ) + "°     ");

  }
  */
