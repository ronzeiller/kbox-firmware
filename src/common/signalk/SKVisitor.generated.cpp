// This file was automatically generated by sk-code-generator.py
// DO NOT MODIFY! YOUR CHANGES WOULD BE OVERWRITTEN
// Look at ./src/common/signalk/SKVisitor.cpp.tmpl instead or modify the script
// Generated on 2017-12-09 23:24:25.838573

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

#include "SKVisitor.generated.h"


void SKVisitor::visit(const SKUpdate& u, const SKPath &p, const SKValue &v) {
  if (p.getStaticPath() == SKPathEnvironmentDepthBelowKeel) {
    visitSKEnvironmentDepthBelowKeel(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentDepthBelowTransducer) {
    visitSKEnvironmentDepthBelowTransducer(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentDepthBelowSurface) {
    visitSKEnvironmentDepthBelowSurface(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentDepthTransducerToKeel) {
    visitSKEnvironmentDepthTransducerToKeel(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentDepthSurfaceToTransducer) {
    visitSKEnvironmentDepthSurfaceToTransducer(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentWaterTemperature) {
    visitSKEnvironmentWaterTemperature(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentOutsideApparentWindChillTemperature) {
    visitSKEnvironmentOutsideApparentWindChillTemperature(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentOutsidePressure) {
    visitSKEnvironmentOutsidePressure(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentWindAngleApparent) {
    visitSKEnvironmentWindAngleApparent(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentWindAngleTrueGround) {
    visitSKEnvironmentWindAngleTrueGround(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentWindAngleTrueWater) {
    visitSKEnvironmentWindAngleTrueWater(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentWindDirectionTrue) {
    visitSKEnvironmentWindDirectionTrue(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentWindDirectionMagnetic) {
    visitSKEnvironmentWindDirectionMagnetic(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentWindSpeedTrue) {
    visitSKEnvironmentWindSpeedTrue(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentWindSpeedOverGround) {
    visitSKEnvironmentWindSpeedOverGround(u, p, v);
  }
  if (p.getStaticPath() == SKPathEnvironmentWindSpeedApparent) {
    visitSKEnvironmentWindSpeedApparent(u, p, v);
  }
  if (p.getStaticPath() == SKPathElectricalBatteriesVoltage) {
    visitSKElectricalBatteriesVoltage(u, p, v);
  }
  if (p.getStaticPath() == SKPathNavigationAttitude) {
    visitSKNavigationAttitude(u, p, v);
  }
  if (p.getStaticPath() == SKPathNavigationCourseOverGroundTrue) {
    visitSKNavigationCourseOverGroundTrue(u, p, v);
  }
  if (p.getStaticPath() == SKPathNavigationHeadingMagnetic) {
    visitSKNavigationHeadingMagnetic(u, p, v);
  }
  if (p.getStaticPath() == SKPathNavigationHeadingTrue) {
    visitSKNavigationHeadingTrue(u, p, v);
  }
  if (p.getStaticPath() == SKPathNavigationLog) {
    visitSKNavigationLog(u, p, v);
  }
  if (p.getStaticPath() == SKPathNavigationMagneticVariation) {
    visitSKNavigationMagneticVariation(u, p, v);
  }
  if (p.getStaticPath() == SKPathNavigationPosition) {
    visitSKNavigationPosition(u, p, v);
  }
  if (p.getStaticPath() == SKPathNavigationSpeedOverGround) {
    visitSKNavigationSpeedOverGround(u, p, v);
  }
  if (p.getStaticPath() == SKPathNavigationSpeedThroughWater) {
    visitSKNavigationSpeedThroughWater(u, p, v);
  }
  if (p.getStaticPath() == SKPathNavigationTripLog) {
    visitSKNavigationTripLog(u, p, v);
  }
  if (p.getStaticPath() == SKPathSteeringRudderAngle) {
    visitSKSteeringRudderAngle(u, p, v);
  }
  if (p.getStaticPath() == SKPathSteeringRudderAngleTarget) {
    visitSKSteeringRudderAngleTarget(u, p, v);
  }
  if (p.getStaticPath() == SKPathPerformanceLeeway) {
    visitSKPerformanceLeeway(u, p, v);
  }
}

