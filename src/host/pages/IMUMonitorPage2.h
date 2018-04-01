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

#include "common/ui/Page.h"
#include "common/ui/TextLayer.h"
#include "common/signalk/SKHub.h"
#include "common/signalk/SKSubscriber.h"
//TODO: damping and automatic damping according to service frequency
#include "host/config/IMUConfig.h"
#include "util/IIRFilter.h"

class IMUMonitorPage2 : public Page, public SKSubscriber {
  private:
    TextLayer *_hdgTL, *_rollTL, *_pitchTL, *_calTL;
    IMUConfig &_config;

    double _pitch, _roll, _heading;

    unsigned long int _nextPrint = 0;
    uint16_t _IMUServiceInterval = 100;

    iirfilter   IMU_HdgFilter;
    iirfilter   IMU_RollFilter;
    iirfilter   IMU_PitchFilter;

    double _dampingHdg;
    double _dampingRollPitch;

    double _IMU_HDGFiltered;
    double _IMU_PitchFiltered;
    double _IMU_RollFiltered;

  public:
    IMUMonitorPage2(IMUConfig &config, SKHub& hub);

    virtual void updateReceived(const SKUpdate& up);

    bool processEvent(const TickEvent &te);
    bool processEvent(const ButtonEvent &be);
};
