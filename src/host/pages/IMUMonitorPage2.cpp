/*
     __  __     ______     ______     __  __
    /\ \/ /    /\  == \   /\  __ \   /\_\_\_\
    \ \  _"-.  \ \  __<   \ \ \/\ \  \/_/\_\/_
     \ \_\ \_\  \ \_____\  \ \_____\   /\_\/\_\
       \/_/\/_/   \/_____/   \/_____/   \/_/\/_/

  Project:  KBox
            Copyright (c) 2018 Thomas Sarlandie thomas@sarlandie.net
  Purpose:  Monitor Page for extern IMU, Hdg, heel, pitch
  Authors:  Ronnie Zeiller ronnie@zeiller.eu
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


#include "IMUMonitorPage2.h"

#include "common/signalk/SKUpdate.h"
#include "common/signalk/SKUnits.h"

IMUMonitorPage2::IMUMonitorPage2(IMUConfig &config, SKHub& hub) : _config(config) {
  _IMU_HDGFiltered    = 361;
  _IMU_PitchFiltered  = 0;
  _IMU_RollFiltered   = 0;

  // Damping values 1.....100 --> 0.5 no filter ......0.005 max filter
  // for higher frequencies even more
  // TODO: make KBox setting for damping values 1.......100

  // auto damping factored by the IMUServiceInterval which is
  // probably between 50 and 100 (20 - 10 Hz)
  // TODO: set and get IMUServiceInterval for automatic modus -> #define IMUServiceInterval ??

  _dampingHdg = 0.05; // _IMUServiceInterval / 1400.000; // 0.05
  if (_dampingHdg>0.5) _dampingHdg=0.5;


  // Heel and Pitch need smaller damping
  _dampingRollPitch = _IMUServiceInterval / 700.000; // 0.1 .... 0.1;
  if (_dampingRollPitch>0.5) _dampingRollPitch=0.5;

  INFO( "HDG Damping factor taken for IIR-filter: %f", _dampingHdg );
  INFO( "Heel/Pitch Damping factor taken for IIR-filter: %f", _dampingRollPitch );

  // set filter type
  IMU_HdgFilter.setType(IIRFILTER_TYPE_DEG);
  IMU_HdgFilter.setFC(_dampingHdg);
  IMU_RollFilter.setType(IIRFILTER_TYPE_LINEAR);
  IMU_RollFilter.setFC(_dampingRollPitch);
  IMU_PitchFilter.setType(IIRFILTER_TYPE_LINEAR);
  IMU_PitchFilter.setFC(_dampingRollPitch);

  static const int col1 = 5;
  static const int col2 = 200;
  static const int row1 = 26;
  static const int row2 = 50;
  static const int row3 = 152;
  static const int row4 = 182;

  addLayer(new TextLayer(Point(col1, row1), Size(20, 20), "HDG ° Mag", ColorWhite, ColorBlack, FontDefault));
  //addLayer(new TextLayer(Point(col2, row1), Size(20, 20), "Sys/Mag/Acc", ColorWhite, ColorBlack, FontDefault));
  addLayer(new TextLayer(Point(col1, row3), Size(20, 20), "Heel °", ColorWhite, ColorBlack, FontDefault));
  addLayer(new TextLayer(Point(col2, row3), Size(20, 20), "Pitch °", ColorWhite, ColorBlack, FontDefault));

  _hdgTL = new TextLayer(Point(col1, row2), Size(20, 20), "--", ColorWhite, ColorBlack, FontLarge);
  _calTL = new TextLayer(Point(col2, row2), Size(20, 20), "--", ColorWhite, ColorBlack, FontLarge);
  _rollTL  = new TextLayer(Point(col1, row4), Size(20, 20), "--", ColorWhite, ColorBlack, FontLarge);
  _pitchTL = new TextLayer(Point(col2, row4), Size(20, 20), "--", ColorWhite, ColorBlack, FontLarge);

  addLayer(_hdgTL);
  //addLayer(_calTL);
  addLayer(_rollTL);
  addLayer(_pitchTL);

  hub.subscribe(this);
}

bool IMUMonitorPage2::processEvent(const ButtonEvent &be){
  if (be.clickType == ButtonEventTypeClick) {
    // Change page on single click.
    return false;
  }
  if (be.clickType == ButtonEventTypeLongClick) {
    //_imuService.setRollPitchOffset();
  }
  return true;
}

bool IMUMonitorPage2::processEvent(const TickEvent &te){
  // Tick event is every second, not needed here, because trigered by updateReceived
  return true;
}

void IMUMonitorPage2::updateReceived(const SKUpdate& up) {
  //if (up.getSource().getInput() != SKSourceInputNMEA2000) {
  if (up.getSource().getInput() != SKSourceInputKBoxIMU) {
    if ( up.hasNavigationHeadingMagnetic() ) {
      const SKValue& vm = up.getNavigationHeadingMagnetic();
      _IMU_HDGFiltered = IMU_HdgFilter.filter( round ( SKRadToDeg( vm.getNumberValue()) * 10) / 10.0 );
      //DEBUG("HDG: %f\n", round( SKRadToDeg( vm.getNumberValue()) * 10) / 10.0 );
    }

    if ( up.hasNavigationAttitude() ) {
      const SKValue& vm = up.getNavigationAttitude();
      _IMU_RollFiltered = IMU_RollFilter.filter( round( SKRadToDeg( vm.getAttitudeValue().roll ) * 10) / 10.0 );
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
        _hdgTL->setText(String( _IMU_HDGFiltered, 1) + "°        ");
      if ( _IMU_PitchFiltered < 181 && _IMU_PitchFiltered > -181 )
        _pitchTL->setText(String( _IMU_PitchFiltered, 1 ) + "°     ");
      if ( _IMU_RollFiltered < 181 && _IMU_RollFiltered > -181 )
        _rollTL->setText(String( _IMU_RollFiltered, 1 ) + "°     ");
    }
  }
}
