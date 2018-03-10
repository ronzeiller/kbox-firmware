/*
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
/*
    Jeder Eingang liefert letztendlich ein SKUpdate, das hier im SKHub landet.
    Über die Funktion publish werden diese SKUpdates an alle Subscriber verteilt.

    D.h. die Service Class bekommt als Parameter SKHub übergeben und vom lokalen
    Objekt _hub übernommen.
    Die Subscription erfolgt über den Aufruf: _hub.subscribe(this);
    Womit der Service in die Liste der Subscriber aufgenommen wird.

*/

#include <time.h>
#include <KBoxLogging.h>
#include "SKHub.h"
#include "SKSubscriber.h"
#include "SKUpdate.h"
#include "SKUnits.h"
#include "SKUpdateStatic.h"

/* for static variables only, but static probably not necessary
double SKHub::sog = SKDoubleNAN;
double SKHub::cog = SKDoubleNAN;
double SKHub::awa = SKDoubleNAN;
double SKHub::awaFiltered = SKDoubleNAN;
double SKHub::aws = SKDoubleNAN;
double SKHub::awsFiltered = SKDoubleNAN;
*/

SKHub::SKHub() {

  _awsFilter.setType(IIRFILTER_TYPE_LINEAR);
  _awsFilter.setFC(_dampingFactor10Hz);
  _awaFilter.setType(IIRFILTER_TYPE_DEG);
  _awaFilter.setFC(_dampingFactor10Hz);
  _heelFilter.setType(IIRFILTER_TYPE_LINEAR);
  _heelFilter.setFC(_dampingFactor10Hz);

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

  bool sendHighSpeedUpdate = true;
  bool sendPerformanceUpdate = false;
  SKUpdateStatic<2> updatePerf;

  // All KBox config settings are available here
  //if (_kboxConfig.serial1Config.repeatSentence) {}

  // Wind direction and wind speed are coming from NMEA2000 with around 8 to 10 Hz
  // we store the value and we filter (damp) the value in IIR-filter
  // TODO: Calculations could be done with high speed values or damped
  if (update.hasEnvironmentWindSpeedApparent()){
    _aws = update.getEnvironmentWindSpeedApparent();
    _awsFiltered = _awsFilter.filter(_aws);
  }
  if (update.hasEnvironmentWindAngleApparent()){
    _awa = update.getEnvironmentWindAngleApparent();
    _awaFiltered = _awaFilter.filter(_awa);
  }

  // For calculating corrected boat speed, apparent and true wind etc. we need
  // heel values from IMUService or external IMU-sensor, which should come with
  // around 10Hz.
  // For heel we make filtered values too.
  if (update.hasNavigationAttitude()){
    _heel = update.getNavigationAttitude().roll;
    //DEBUG("SKUpdate --> Heel: %f °", SKRadToDeg(_heel));
    _heelFiltered = _heelFilter.filter(_heel);
  }

  // On most systems boatspeed will come with 1Hz
  // So we take update of boat speed to trigger all performance calculations
  // SpeedThroughWater is so far the uncorrected measured raw value, which we
  // have to correct by calibration values and leeway!
  if (update.hasNavigationSpeedThroughWater() &&
      update.getNavigationSpeedThroughWater() > 0 &&
      _kboxConfig.performanceConfig.enabled){

    // SKUpdate is in m/s --> we need knots for formulas
    double bs_kts_m = SKMsToKnot(update.getNavigationSpeedThroughWater());
    double leeway = performance.getLeeway(bs_kts_m, _heel);
    double bs_kts_corr = performance.calcBoatSpeed(bs_kts_m, _heel, leeway);

    // change value in update to corrected one

    SKSource source = SKSource::performanceCalc();
    updatePerf.setSource(source);
    updatePerf.setTimestamp(now());
    updatePerf.setNavigationSpeedThroughWater(SKKnotToMs(bs_kts_corr));
    updatePerf.setPerformanceLeeway(leeway);

    sendPerformanceUpdate = true;
  }

  if (update.hasEnvironmentWindDirectionTrue()){}
  if (update.hasEnvironmentWindSpeedTrue()){}

  if (update.hasNavigationCourseOverGroundTrue()){
    _cog = update.getNavigationCourseOverGroundTrue();
    //DEBUG("COG = %f°", SKRadToDeg(cog));
  }

  if (update.hasNavigationMagneticVariation()){}
  if (update.hasNavigationPosition()){}
  if (update.hasNavigationSpeedOverGround()){
    _sog = update.getNavigationSpeedOverGround();
    //DEBUG("SOG = %f ktn", SKMsToKnot(sog));
  }

  if (update.hasNavigationTripLog()){}
  if (update.hasNavigationLog()){}
  if (update.hasPerformanceLeeway()){}

  for (LinkedListIterator<SKSubscriber*> it = _filteredSubscribers.begin(); it != _filteredSubscribers.end(); it++) {
    if ( !sendPerformanceUpdate ) {
      (*it)->updateReceived(update);
    } else {
      (*it)->updateReceived(updatePerf);
    }
  }

  // High speed subscribers
  if (sendHighSpeedUpdate){
    for (LinkedListIterator<SKSubscriber*> it = _subscribers.begin(); it != _subscribers.end(); it++) {
      if ( !sendPerformanceUpdate ) {
        (*it)->updateReceived(update);
      } else {
        (*it)->updateReceived(updatePerf);
      }
    }
  }
}
