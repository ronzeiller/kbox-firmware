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

#include "common/signalk/SKUpdateStatic.h"

#include "ADCService.h"

void ADCService::loop() {

  SKUpdateStatic<4> sk;

  if (_config.enableAdc1) {
    int adc1_adc = _adc.analogRead(adc1_analog, ADC_0);
    _adc1 = adc1_adc * analog_max_voltage / _adc.getMaxValue();
    sk.setElectricalBatteriesVoltage(_config.labelAdc1, _adc1);
  }

  if (_config.enableAdc2) {
    int adc2_adc = _adc.analogRead(adc2_analog, ADC_0);
    _adc2 = adc2_adc * analog_max_voltage / _adc.getMaxValue();
    sk.setElectricalBatteriesVoltage(_config.labelAdc2, _adc2);
  }

  if (_config.enableAdc3) {
    int adc3_adc = _adc.analogRead(adc3_analog, ADC_0);
    _adc3 = adc3_adc * analog_max_voltage / _adc.getMaxValue();
    sk.setElectricalBatteriesVoltage(_config.labelAdc3, _adc3);
  }

  if (_config.enableAdc4) {
    int supply_adc = _adc.analogRead(supply_analog, ADC_0);
    _supply = supply_adc * analog_max_voltage / _adc.getMaxValue();
    sk.setElectricalBatteriesVoltage(_config.labelAdc4, _supply);
  }

  _skHub.publish(sk);
}
