/*
  The MIT License

  Copyright (c) 2016 Thomas Sarlandie thomas@sarlandie.net

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
#include <KBoxHardware.h>
#include <Adafruit_BMP280.h>
#include "common/signalk/SKUpdateStatic.h"
#include "BarometerService.h"

void BarometerService::setup() {
  if (!_bmp280.begin(bmp280_address)) {
    DEBUG("Error initializing BMP280");
    _status = 1;
  }
  else {
    _status = 0;
  }
}

void BarometerService::fetchValues() {
  _temperature = _bmp280.readTemperature();
  _pressure = _bmp280.readPressure() + _config.calOffset;

  DEBUG("Temperature=%.2f C | Pressure=%.1f hPa", _temperature, _pressure/100);

  SKUpdateStatic<1> update;
  update.setEnvironmentOutsidePressure(_pressure);  // in Pa
  SKSource source = SKSource::internalSensor();
  update.setSource(source);
  _skHub.publish(update);
}

void BarometerService::loop() {
  fetchValues();
}
