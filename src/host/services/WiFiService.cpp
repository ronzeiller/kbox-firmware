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

// Über WiFi wird im Moment 23.1.2018 entweder $PCDIN oder NMEA0183 gesendet

#include <KBoxLogging.h>
#include <KBoxHardware.h>
#include <config/WiFiConfig.h>
#include "common/signalk/SKNMEAConverter.h"
#include "common/signalk/KMessageNMEAVisitor.h"
#include "common/signalk/SKJSONVisitor.h"
#include "stats/KBoxMetrics.h"

#include "WiFiService.h"

WiFiService::WiFiService(const WiFiConfig &config, SKHub &skHub, GC &gc) :
  Task("WiFi"), _config(config), _hub(skHub), _slip(WiFiSerial, 2048) {
  // We will need gc at some point to be able to take screenshot
}

void WiFiService::setup() {
  KBox.espInit();
  if (_config.enabled) {
    DEBUG("booting ESP!");
    KBox.espRebootInProgram();
  }

  _hub.subscribe(this);
}

void WiFiService::loop() {
  if (_slip.available()) {
    uint8_t *frame;
    size_t len = _slip.peekFrame(&frame);

    KommandReader kr = KommandReader(frame, len);

    KommandHandler *handlers[] = { &_pingHandler, &_wifiLogHandler, 0 };
    if (KommandHandler::handleKommandWithHandlers(handlers, kr, _slip)) {
      KBoxMetrics.event(KBoxEventWiFiValidKommand);
    }
    else {
      KBoxMetrics.event(KBoxEventWiFiInvalidKommand);
      ERROR("Invalid command received from WiFi module (id=%i size=%i)", kr.getKommandIdentifier(), kr.dataSize());
      frame[len-1] = 0;
      DEBUG("> %s", frame);
    }

    _slip.readFrame(0, 0);
  }
}

// ****************************************************************************
// Vom NMEA2000Service kommend mit sendMessage() geht im KGenerator
// die Message an alle connected Receivers, wo der Aufruf processMessage()
// aufgerufen wird.
// Je nach KMessage NMEA2000 oder NMEA0183 wird $PCDIN oder ein NMEA0183 String
// mit getNMEAContent() abgeholt und über
// _slip.writeFrame() an das WiFi geschrieben
// ****************************************************************************
void WiFiService::processMessage(const KMessage &m) {
  if (!_config.enabled) {
    return;
  }

  KMessageNMEAVisitor v(_config.dataFormatConfig);
  m.accept(v);

  // $PCDIN oder NMEA0183 je nach KMessage Typ NMEASentence oder NMEA2000Message
  String data = v.getNMEAContent();
  // Output 0 bei "NMEA"
  // Output $PCDIN,01F802,0000002C,0D,0000AD5A3F01FFFF*2D (inkl. \r\n) bei "SeaSmart"
  // DEBUG("processMessage: %s", data.c_str());

  // TODO
  // Should not limit like this to only 256 because there could be many nmea lines in this message.
  // Let's figure this out when we pass SignalK data.
  FixedSizeKommand<256> k(KommandNMEASentence);
  k.appendNullTerminatedString(data.c_str());

  _slip.writeFrame(k.getBytes(), k.getSize());
}

// ****************************************************************************
// Gegenstück zu processMessage()
// Hier werden alle SKSubscriber von einem SignalK Update informiert
// Im SKNMEAConverter wird das SignalK JSON in ein NMEA0183 konvertiert
// über convert(u, *this); --> wird im SKNMEAConverter write(SKNMEASentence)
// aufgerufen und der NMEA0183 Satz geschrieben.
//
// Danach "Now send in JSON format" scheint nicht zu funktionieren.....
// ****************************************************************************
void WiFiService::updateReceived(const SKUpdate& u) {
  /* This is where we convert the data in SignalK format that floats inside KBox
   * to messages that will be sent to WiFi clients.
   */

  if (!_config.enabled) {
    return;
  }

  SKNMEAConverter nmeaConverter(_config.nmeaConverter);
  // The converter will call this->write(NMEASentence) for every generated sentence
  // convert(const SKUpdate& update, SKNMEAOutput& output)
  nmeaConverter.convert(u, *this);

  // Now send in JSON format
  /*
  StaticJsonBuffer<1024> jsonBuffer;
  SKJSONVisitor jsonVisitor(jsonBuffer);
  JsonObject &jsonData = jsonVisitor.processUpdate(u);
  FixedSizeKommand<1024> k(KommandSKData);
  jsonData.printTo(k);
  k.write(0);
  _slip.writeFrame(k.getBytes(), k.getSize());
  */
}

// ****************************************************************************
// Aufruf von write erfolgt im SKNMEAConverter mit output.write(sb.toNMEA());
// womit der NMEA0183 Satz an das WiFi geschrieben wird
// ****************************************************************************
bool WiFiService::write(const SKNMEASentence& sentence) {
  // NMEA Sentences should always be 82 bytes or less
  FixedSizeKommand<100> k(KommandNMEASentence);
  String s = sentence + "\r\n";
  k.appendNullTerminatedString(s.c_str());
  _slip.writeFrame(k.getBytes(), k.getSize());
  return true;
}
