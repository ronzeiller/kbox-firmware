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

#include <KBoxHardware.h>
#include "common/os/TaskManager.h"
#include "common/signalk/SKHub.h"
#include "host/config/KBoxConfig.h"
#include "host/config/KBoxConfigParser.h"
#include "host/drivers/ILI9341GC.h"
#include "host/pages/BatteryMonitorPage.h"
#include "host/pages/StatsPage.h"
#include "host/services/MFD.h"
#include "host/services/ADCService.h"
#include "host/services/BarometerService.h"
#include "host/services/IMUService.h"
#include "host/pages/IMUMonitorPage.h"
#include "host/pages/IMUMonitorPage2.h"
#include "host/services/NMEA2000Service.h"
#include "host/services/SerialService.h"
#include "host/services/RunningLightService.h"
#include "host/services/SDCardTask.h"
#include "host/services/USBService.h"
#include "host/services/WiFiService.h"
#include "host/performance/Performance.h"

static const char *configFilename = "kbox-config.json";

ILI9341GC gc(KBox.getDisplay(), Size(320, 240));
MFD mfd(gc, KBox.getEncoder(), KBox.getButton());
TaskManager taskManager;
SKHub skHub;
KBoxConfig config;

USBService usbService(gc, skHub);

void setup() {
  // Enable float in printf:
  // https://forum.pjrc.com/threads/27827-Float-in-sscanf-on-Teensy-3-1
  asm(".global _printf_float");

  KBox.setup();

  // Clears the screen
  mfd.setup();

  delay(1000);

  Serial.begin(115200);
  KBoxLogging.setLogger(&usbService);

  DEBUG("Starting");

  digitalWrite(led_pin, 1);

  // Generate a default URN based on MCU serial number.
  const char *vesselURLFormat = "urn:mrn:signalk:uuid:%08lX-%04lX-4%03lX-%04lX-%04lX%08lX";
  char defaultVesselURN[80];
  tKBoxSerialNumber kboxSN;
  KBox.readKBoxSerialNumber(kboxSN);
  snprintf(defaultVesselURN, sizeof(defaultVesselURN), vesselURLFormat,
           kboxSN.dwords[0], (uint16_t)(kboxSN.dwords[1] >> 16), (uint16_t)(kboxSN.dwords[1] & 0x0fff),
           (uint16_t)(kboxSN.dwords[2] >> 16), (uint16_t)(kboxSN.dwords[2] & 0xffff), kboxSN.dwords[3]);

  // Load configuration if available
  KBoxConfigParser configParser(defaultVesselURN);
  configParser.defaultConfig(config);
  if (KBox.getSdFat().exists(configFilename)) {
    File configFile = KBox.getSdFat().open(configFilename);
    // We can afford to allocate a lot of memory on the stack for this because we have not started doing
    // anything real yet.
    StaticJsonBuffer<6144> jsonBuffer;
    JsonObject &root =jsonBuffer.parseObject(configFile);

    if (root.success()) {
      DEBUG("Loading configuration file %s from SDCard", configFilename);
      configParser.parseKBoxConfig(root, config);
    }
    else {
      ERROR("Failed to parse configuration file");
    }
  }
  else {
    DEBUG("No configuration file found. Using defaults.");
  }

  skHub.setSKHubConfig(config);

  // Test
  //Polar polar;
  //polar.readPolarDataFile();
  static const char *polarDataFileName = "polarData.pol";
  if (KBox.getSdFat().exists(polarDataFileName)) {
    DEBUG("Read Polardata file");
  }

  taskManager.addTask(&usbService);
  taskManager.addTask(&mfd);
  taskManager.addTask(new IntervalTask(new RunningLightService(), 250));

  delay(1000);  // for full DEBUG display befor N2k starts

  // NMEA 2000 SERVICE
  NMEA2000Service *n2kService = new NMEA2000Service(config.nmea2000Config, skHub);
  // SentenceRepeater will get tN2kMsg, which converts it into PCDIN
  if (config.nmea2000Config.repeatSentence) {
    n2kService->addSentenceRepeater(usbService);
  }
  taskManager.addTask(n2kService);

  // WIFI SERVICE
  WiFiService *wifi = new WiFiService(config.wifiConfig, skHub, gc);
  if (config.wifiConfig.enabled) {
    // enabled/disabled PCDIN sentences repeating to WiFi
    if (config.nmea2000Config.repeatSentence) {
      n2kService->addSentenceRepeater(*wifi);
    }
    taskManager.addTask(wifi);
  }

  // SD-CARD SERVICE
  SDCardTask *sdcardTask = new SDCardTask(config.sdcardConfig);
  if (config.sdcardConfig.enabled){
    n2kService->addSentenceRepeater(*sdcardTask);
    taskManager.addTask(sdcardTask);
  }

  // SERIAL SERVICES
  SerialService *reader1 = new SerialService(config.serial1Config, skHub, NMEA1_SERIAL);
  if (config.serial1Config.repeatSentence) {
    reader1->addRepeater(usbService);
    if (config.wifiConfig.enabled) {
      reader1->addRepeater(*wifi);
    }
    if (config.sdcardConfig.enabled){
      reader1->addRepeater(*sdcardTask);
    }
  }
  taskManager.addTask(reader1);

  SerialService *reader2 = new SerialService(config.serial2Config, skHub, NMEA2_SERIAL);
  if (config.serial2Config.repeatSentence) {
    reader2->addRepeater(usbService);
    if (config.wifiConfig.enabled) {
      reader2->addRepeater(*wifi);
    }
    if (config.sdcardConfig.enabled){
      reader2->addRepeater(*sdcardTask);
    }
  }
  taskManager.addTask(reader2);

  #ifdef NMEA3_SERIAL
    if (config.serial3Config.inputMode != SerialModeDisabled) {
      SerialService *reader3 = new SerialService(config.serial3Config, skHub, NMEA3_SERIAL);
      if (config.serial3Config.repeatSentence) {
        reader3->addRepeater(usbService);
        if (config.wifiConfig.enabled) {
          reader3->addRepeater(*wifi);
        }
        if (config.sdcardConfig.enabled){
          reader3->addRepeater(*sdcardTask);
        }
      }
      taskManager.addTask(reader3);
    }
  #endif

  #ifdef NMEA4_SERIAL
    if (config.serial4Config.inputMode != SerialModeDisabled) {
      SerialService  *reader4 = new SerialService(config.serial4Config, skHub, NMEA4_SERIAL);
      if (config.serial4Config.repeatSentence) {
        reader4->addRepeater(usbService);
        if (config.wifiConfig.enabled) {
          reader4->addRepeater(*wifi);
        }
        if (config.sdcardConfig.enabled){
          reader4->addRepeater(*sdcardTask);
        }
      }
      taskManager.addTask(reader4);
    }
  #endif

  if (config.barometerConfig.enabled) {
    BarometerService *baroService = new BarometerService(config.barometerConfig, skHub);
    taskManager.addTask(new IntervalTask(baroService, 1000 / config.barometerConfig.frequency));
  }

  if (config.imuConfig.enabled) {
    IMUService *imuService = new IMUService(config.imuConfig, skHub);
    taskManager.addTask(new IntervalTask(imuService, 1000 / config.imuConfig.frequency));
    // At the moment the IMUMonitorPage is working with built-in sensor only
    IMUMonitorPage *imuPage = new IMUMonitorPage(config.imuConfig, skHub, *imuService);
    mfd.addPage(imuPage);
  }

  // New page for external heel, pitch and hdg sensor
  // TODO: config to en/disable
  IMUMonitorPage2 *imuPage2 = new IMUMonitorPage2(config.imuConfig, skHub);
  mfd.addPage(imuPage2);

  if (config.adcConfig.enabled) {
    ADCService *adcService = new ADCService(config.adcConfig, skHub, KBox.getADC());
    taskManager.addTask(new IntervalTask(adcService, 1000 / config.adcConfig.frequency));
    BatteryMonitorPage *batPage = new BatteryMonitorPage(config.adcConfig, skHub);
    mfd.addPage(batPage);
  }

  StatsPage *statsPage = new StatsPage();
  if (config.sdcardConfig.enabled){
    statsPage->setSDCardTask(sdcardTask);
  }
  if (config.wifiConfig.enabled) {
    statsPage->setWiFiService(wifi);
  }

  mfd.addPage(statsPage);

  taskManager.setup();

  // Reinitialize debug here because in some configurations
  // (like logging to nmea2 output), the kbox setup might have messed
  // up the debug configuration.
  DEBUG("setup done");
}

void loop() {
  taskManager.loop();
}
