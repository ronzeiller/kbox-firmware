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

#include <KBoxLogging.h>
#include "SKHub.h"
#include "SKSubscriber.h"
#include "SKUpdate.h"
#include "SKUnits.h"

double SKHub::sog = SKDoubleNAN;
double SKHub::cog = SKDoubleNAN;

SKHub::SKHub() {
}

SKHub::~SKHub() {
}

void SKHub::subscribe(SKSubscriber* subscriber) {
  _subscribers.add(subscriber);
}

void SKHub::publish(const SKUpdate& update) {

  if (update.hasEnvironmentWindDirectionTrue()){}
  if (update.hasEnvironmentWindSpeedTrue()){}
  if (update.hasNavigationCourseOverGroundTrue()){
    cog = update.getNavigationCourseOverGroundTrue();
    //DEBUG("COG = %f°", SKRadToDeg(cog));
  }

  if (update.hasNavigationMagneticVariation()){}
  if (update.hasNavigationPosition()){}
  if (update.hasNavigationSpeedOverGround()){
    sog = update.getNavigationSpeedOverGround();
    //DEBUG("SOG = %f ktn", SKMsToKnot(sog));
  }
  if (update.hasNavigationSpeedThroughWater()){}
  if (update.hasNavigationTripLog()){}
  if (update.hasNavigationLog()){}
  if (update.hasPerformanceLeeway()){}

  for (LinkedListIterator<SKSubscriber*> it = _subscribers.begin(); it != _subscribers.end(); it++) {
    (*it)->updateReceived(update);
  }
}
