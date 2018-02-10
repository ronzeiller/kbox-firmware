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

#pragma once

#include <Adafruit_BMP280.h>
#include "common/os/Task.h"
#include "common/signalk/KMessage.h"
#include "common/signalk/SKHub.h"
#include "config/BarometerConfig.h"

class BarometerService : public Task, public KGenerator {
  private:
    const BarometerConfig &_config;
    SKHub& _skHub;
    int _status = -1;

    void fetchValues();

    Adafruit_BMP280 _bmp280;
    float _temperature;
    float _pressure;

  public:
    BarometerService(BarometerConfig &config, SKHub& skHub) : Task("Barometer"), _config(config), _skHub(skHub) {};

    void setup();
    void loop();
};
