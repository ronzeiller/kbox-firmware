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
#include "common/config/KBoxConfig.h"
#include "common/signalk/SKUpdateStatic.h"
#include "common/signalk/SKUnits.h"
#include "IMUService.h"

void IMUService::setup() {
  _offsetRoll  = 0.0;
  _offsetPitch = 0.0;

  // until config is made, declare default mounting position KBox here
  uint8_t axisConfig = 0b00001001;
  uint8_t signConfig = 0b00000000;

  DEBUG("Initing BNO055");
  if (!bno055.begin(bno055.OPERATION_MODE_NDOF, axisConfig, signConfig)) {
    DEBUG("Error initializing BNO055");
  }
  else {
    if (recallCalibration())
      DEBUG("Success!");
  }
}

void IMUService::loop() {
  bno055.getCalibration(&_sysCalib, &_gyroCalib, &_accelCalib, &_magCalib);

  eulerAngles = bno055.getVector(Adafruit_BNO055::VECTOR_EULER);

  //DEBUG("Calib Sys: %i Accel: %i Gyro: %i Mag: %i", _sysCalib, _accelCalib, _gyroCalib, _magCalib);
  //DEBUG("Attitude roll: %.3f pitch: %.3f  Mag heading: %.3f", eulerAngles.z(), eulerAngles.y(), eulerAngles.x());

  SKUpdateStatic<2> update;
  // Note: We could calculate yaw as the difference between the Magnetic
  // Heading and the Course Over Ground Magnetic.

  /* if orientation == MOUNTED_ON_STARBOARD_HULL */
  _roll = SKDegToRad(eulerAngles.z() + _offsetRoll);
  _pitch = SKDegToRad(eulerAngles.y() + _offsetPitch);
  _heading = SKDegToRad(fmod(eulerAngles.x() + 270, 360));
  //DEBUG("Attitude heel: %.3f pitch: %.3f  Mag heading: %.3f", SKRadToDeg(_roll), SKRadToDeg(_pitch), SKRadToDeg(_heading));

  if (_magCalib >= cfHdgMinCal) {
    update.setNavigationHeadingMagnetic(_heading);
    _skHub.publish(update);
  }

	if (_accelCalib >= cfHeelPitchMinCal && _gyroCalib >= cfHeelPitchMinCal) {
    update.setNavigationAttitude(SKTypeAttitude(/* roll */ _roll, /* pitch */ _pitch, /* yaw */ 0));
    _skHub.publish(update);
  }
}

void IMUService::getLastValues(int &accelCalibration, double &pitch, double &roll, int &magCalibration, double &heading){
  accelCalibration = _accelCalib;
  pitch = _pitch;
  roll = _roll;
  magCalibration = _magCalib;
  heading = _heading;
}

//  Offset for Heel=0 and Pitch=0 with long button press in IMUMonitorPage
void IMUService::setOffset() {
  // FIXIT: Why it is called at startup of KBox? event longClick coming?
  if ( millis() > 10000 ) {
    _offsetRoll = _roll * (-1);
    _offsetPitch = _pitch * (-1);
    DEBUG("Offset changed: Heel -> %.3f | Pitch -> %.3f", SKRadToDeg(_offsetRoll), SKRadToDeg(_offsetPitch));
    // TODO: angle of heel and pitch must be smaller than 90°, check overflow!
  }
}

bool IMUService::saveCalibration() {
  int _eeAddress = 0;
  long bnoID;
  sensor_t sensor;
  adafruit_bno055_offsets_t newCalib;
  bno055.getSensorOffsets(newCalib);

  DEBUG("\n\nStoring calibration data to EEPROM...");
  bno055.getSensor(&sensor);
  bnoID = sensor.sensor_id;

  EEPROM.put(_eeAddress, bnoID);

  _eeAddress += sizeof(long);
  EEPROM.put(_eeAddress, newCalib);
  DEBUG("Data stored to EEPROM.");
  delay(500);
  return true;
}

bool IMUService::recallCalibration() {

  int _eeAddress = 0;
  long bnoID;

  EEPROM.get(_eeAddress, bnoID);
  adafruit_bno055_offsets_t calibrationData;
  sensor_t sensor;

  /*
  *  Look for the sensor's unique ID at the beginning oF EEPROM.
  *  This isn't foolproof, but it's better than nothing.
  */
  bno055.getSensor(&sensor);
  if (bnoID != sensor.sensor_id) {
      DEBUG("No Calibration Data for this sensor exists in EEPROM");
  }
  else {
      DEBUG("Found Calibration for this sensor in EEPROM.");
      _eeAddress += sizeof(long);
      EEPROM.get(_eeAddress, calibrationData);

      DEBUG("Restoring Calibration data to the BNO055...");
      bno055.setSensorOffsets(calibrationData);

      DEBUG("Calibration data loaded into BNO055");
      delay(1000);
      return true;
  }
  return false;
}
