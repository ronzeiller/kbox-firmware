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

/*
  Das Service liest von der und schreibt an die serielle Schnittstelle
  Im Moment nur zwei Schnittstellen konfiguriert und nur für NMEA0183
  Theoretisch sind auch andere SKSourceInput Formate möglich

  Daten Eingang -> Loop:
  1.) Daten werden über sendMessage() an den NMEAVisitor gesendet
  (Alle connected Receivers, also WiFi und SD-Card verarbeiten die NMEA Sätze
  über processMessage)
  2.) Sätze werden geparst (SKNMEAParser) und als SKUpdate an den Hub gesendet
  (Hub.publish(SKUpdate))

  Schreiben: Über updateReceived werden SKUpdates vom Hub über den
  SKNMEAConverter in NMEA0183 Sätze umgeformt, die über die virtuelelle Funktion
  write(SKNMEASentence) direkt an die Serielle Schnittstelle geschrieben werden.

  Anmerkung:
  Sind In und Out konfiguriert, werden theoretisch empfangene Sätze über Hub update
  auch wieder zurückgeschrieben!

*/

#include "SerialService.h"

#include <KBoxLogging.h>
#include <Arduino.h>
#include <KBoxHardware.h>
#include <signalk/SKNMEAConverter.h>
#include "common/algo/List.h"
#include "common/stats/KBoxMetrics.h"
#include "common/signalk/SKNMEAParser.h"
#include "common/signalk/SKSource.h"
#include "host/config/SerialConfig.h"


// Linked list used to buffer messages
static LinkedList<NMEASentence> *received2 = 0;
static LinkedList<NMEASentence> *received3 = 0;

// This is called by yield() whenever data is available.
// We move received data into a linked list of NMEA sentences.
void serialEvent2() {
  static uint8_t buffer[MAX_NMEA_SENTENCE_LENGTH];
  static int index = 0;

  if (received2 == 0) {
    return;
  }

  // 64 is the RX_BUFFER_SIZE defined in serial2.c (teensy3 framework)
  if (Serial2.available() == 64) {
    KBoxMetrics.event(KBoxEventNMEA1RXBufferOverflow);
    DEBUG("serialEvent2 found a full rx buffer - we probably lost some data");
  }

  while (Serial2.available()) {
    buffer[index++]= (uint8_t) Serial2.read();

    // Check if we are at the end of the buffer
    // The -1 is because we need space to add a terminating NULL character.
    if (index >= MAX_NMEA_SENTENCE_LENGTH - 1) {
      buffer[MAX_NMEA_SENTENCE_LENGTH - 1] = 0;
      DEBUG("Discarding incomplete sequence: %s", buffer);
      index = 0;
      KBoxMetrics.event(KBoxEventNMEA1RXError);
    }
    else {
      // Check if we have reached end of sentence
      if (buffer[index-1] == '\r' || buffer[index-1] == '\n') {
        if (index > 1) {
          // Only send message if it's greater than 0 bytes.
          // And we know that index is < MAX_NMEA_SENTENCE_LENGTH
          // because we tested buffer.
          buffer[index-1] = 0;
          NMEASentence s((char*)buffer);
          received2->add(s);
        }
        // Start again from scratch
        index = 0;
      }
    }
  }
}

void serialEvent3() {
  static uint8_t buffer[MAX_NMEA_SENTENCE_LENGTH];
  static int index = 0;

  if (received3 == 0) {
    return;
  }

  // 64 is the RX_BUFFER_SIZE defined in serial3.c (teensy3 framework)
  if (Serial3.available() == 64) {
    KBoxMetrics.event(KBoxEventNMEA2RXBufferOverflow);
    DEBUG("serialEvent3 found a full rx buffer - we probably lost some data");
  }

  while (Serial3.available()) {
    buffer[index++]= (uint8_t) Serial3.read();

    // Check if we are at the end of the buffer
    // The -1 is because we need space to add a terminating NULL character.
    if (index >= MAX_NMEA_SENTENCE_LENGTH - 1) {
      buffer[MAX_NMEA_SENTENCE_LENGTH - 1] = 0;
      DEBUG("Discarding incomplete sequence: %s", buffer);
      index = 0;
      KBoxMetrics.event(KBoxEventNMEA2RXError);
    }
    else {
      // Check if we have reached end of sentence
      if (buffer[index-1] == '\r' || buffer[index-1] == '\n') {
        if (index > 1) {
          // Only send message if it's greater than 0 bytes.
          // And we know that index is < MAX_NMEA_SENTENCE_LENGTH
          // because we tested buffer.
          buffer[index-1] = 0;
          NMEASentence s((char*)buffer);
          received3->add(s);
        }
        // Start again from scratch
        index = 0;
      }
    }
  }
}

// ****************************************************************************
// In board.h wurde definiert:
// static const pin_t nmea1_out_enable = 24;
// #define NMEA1_SERIAL Serial2
// Gestartet wird das Service in main.cpp (für Serial3 analog):
// SerialService *reader1 = new SerialService(config.serial1Config, skHub, NMEA1_SERIAL);
// ****************************************************************************
SerialService::SerialService(SerialConfig &config, SKHub &hub, HardwareSerial &s) :
                  Task("NMEA Service"), _config(config), _hub(hub), stream(s) {

  if (&s == &Serial2) {
    if (_config.inputMode == SerialModeNMEA) {
      received2 = &receiveQueue;
    }
    _taskName = "Serial Service 1";
    _rxValidEvent = KBoxEventNMEA1RX;
    _rxErrorEvent = KBoxEventNMEA1RXError;
    _txValidEvent = KBoxEventNMEA1TX;
    _txOverflowEvent = KBoxEventNMEA1TXOverflow;
    _skSourceInput = SKSourceInputNMEA0183_1;
  }
  if (&s == &Serial3) {
    if (_config.inputMode == SerialModeNMEA) {
      received3 = &receiveQueue;
    }
    _taskName = "Serial Service 2";
    _rxValidEvent = KBoxEventNMEA2RX;
    _rxErrorEvent = KBoxEventNMEA2RXError;
    _txValidEvent = KBoxEventNMEA2TX;
    _txOverflowEvent = KBoxEventNMEA2TXOverflow;
    _skSourceInput = SKSourceInputNMEA0183_2;
  }
}

// ****************************************************************************
// Für die beiden SKSourceInputs wird die serielle Schnittstelle gestartet - begin()
// Wenn output auch zum Schreiben konfiguriert -> digitalWrite(pin, value)
// ****************************************************************************
void SerialService::setup() {
  if (_skSourceInput == SKSourceInputNMEA0183_1) {
    NMEA1_SERIAL.begin(_config.baudRate);
    digitalWrite(nmea1_out_enable, _config.outputMode != SerialModeDisabled);
    DEBUG("SerialService[1] Baudrate: %i Input: %s Output: %s",
          _config.baudRate,
          _config.inputMode == SerialModeNMEA ? "true" : "false",
          _config.outputMode == SerialModeNMEA ? "true" : "false");
  }
  if (_skSourceInput == SKSourceInputNMEA0183_2) {
    NMEA2_SERIAL.begin(_config.baudRate);
    digitalWrite(nmea2_out_enable, _config.outputMode != SerialModeDisabled);
    INFO("SerialService[2] Baudrate: %i Input: %s Output: %s",
          _config.baudRate,
          _config.inputMode == SerialModeNMEA ? "true" : "false",
          _config.outputMode == SerialModeNMEA ? "true" : "false");
  }

  _hub.subscribe(this);
}

// ****************************************************************************
//  In der Loop wird die receivedQueue Liste abgearbeitet und auf Null gesetzt
//  KBoxMetrics wird hochgezählt (Error, Valid)
//  1.) Daten werden über sendMessage() an den NMEAVisitor gesendet
//  (Alle connected Receivers, also WiFi und SD-Card verarbeiten die NMEA Sätze
//  über processMessage)
//  2.) Sätze werden geparst (SKNMEAParser) und als SKUpdate an den Hub gesendet
//  (Hub.publish(SKUpdate))
// ****************************************************************************
void SerialService::loop() {
  if (receiveQueue.size() == 0) {
    return;
  }
  // Send all queue sentences
  DEBUG("Serial[%i]: Found %i sentences waiting",
        _skSourceInput == SKSourceInputNMEA0183_1 ? 1 : 2,
        receiveQueue.size());
  for (LinkedList<NMEASentence>::iterator it = receiveQueue.begin(); it != receiveQueue.end(); it++) {
    if (it->isValid()) {
      KBoxMetrics.event(_rxValidEvent);
      this->sendMessage(*it);

      SKNMEAParser p;
      //FIXME: Get the time properly here!
      const SKUpdate &update = p.parse(_skSourceInput, (*it).getSentence(), SKTime(0));
      if (update.getSize() > 0) {
        _hub.publish(update);
      }
    }
    else {
      KBoxMetrics.event(_rxErrorEvent);
    }
  }
  // FIXME: the current linked list implementation would probably continue to
  // work if some data is added while we are running this loop but it would be
  // best if we had a clearer contract or a better way to manage this.
  receiveQueue.clear();
}

// ****************************************************************************
//  SKHub benachrichtigt alle Subscriber, wenn ein neues Update angekommen ist.
//  Ist Serial OUT auf NMEA konfiguriert, werden updates über den SKNMEAConverter
//  in NMEA0183 Sätze konvertiert und über write() direkt von dort aufgerufen
//  an die Schnittstelle geschrieben
// ****************************************************************************
void SerialService::updateReceived(const SKUpdate &update) {
  if (_config.outputMode == SerialModeNMEA) {
    SKNMEAConverter nmeaConverter(_config.nmeaConverter);
    nmeaConverter.convert(update, *this);
  }
}

// ****************************************************************************
//  In SKNMEAConverter als virtuelle Funktion definiert, wird write von dort
//  aufgerufen und der NMEA0183 Satz wird auf die Schnittstelle geschrieben
// ****************************************************************************
bool SerialService::write(const SKNMEASentence &nmeaSentence) {
  /*
  DEBUG("Writing NMEA to Serial[%i] output: %s",
        _skSourceInput == SKSourceInputNMEA0183_1 ? 1 : 2,
        nmeaSentence.c_str());
  */
  if ((size_t)stream.availableForWrite() >= nmeaSentence.length() + 2) {
    stream.write(nmeaSentence.c_str());
    stream.write("\r\n");
    KBoxMetrics.event(_txValidEvent);
    return true;
  }
  else {
    KBoxMetrics.event(_txOverflowEvent);
    return false;
  }
}
