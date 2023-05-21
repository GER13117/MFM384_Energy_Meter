//MFM live page example by reaper7

#define READMFMEVERY  2000                                                      //read MFM every 2000ms
#define NBREG   5                                                               //number of MFM registers to read
//#define USE_STATIC_IP

/*  mh et esp32 minikit
                     ______________________
                    /   L T L T L T L T    \
                    |                      |
                    |O O RST      TX HW O O|
                    |O O SVP      RX HW O O|
                    |O O IO26      IO22 O O|
                    |O O IO18      IO21 O O|
                    |O O IO19      IO17 O O|
                    |O O IO23      IO16 O O|
                    |O O IO05       GND O O|
                    |O O 3V3        VCC O O|
                    |O O TCK        TDO O O|
                    |O O SD3        SD0 O O|
                     --|                   |
                       |___________________|
*/

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

#include <AsyncTCP.h>                                                           //https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h>                                                  //https://github.com/me-no-dev/ESPAsyncWebServer

#include <MFM.h>                                                                //https://github.com/reaper7/MFM_Energy_Meter

#include "index_page.h"

#if !defined ( USE_HARDWARESERIAL )
  #error "This example works with Hardware Serial on esp32, please uncomment #define USE_HARDWARESERIAL in MFM_Config_User.h"
#endif
//------------------------------------------------------------------------------
AsyncWebServer server(80);

MFM MFM(Serial, MFM_UART_BAUD, NOT_A_PIN, SERIAL_8N1, 3, 1);                             //esp32 default pins for Serial0 => RX pin 3, TX pin 1

//------------------------------------------------------------------------------
String devicename = "PWRMETER";

#if defined ( USE_STATIC_IP )
IPAddress ip(192, 168, 0, 130);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
#endif

const char* wifi_ssid = "YOUR_SSID";
const char* wifi_password = "YOUR_PASSWORD";

unsigned long readtime;
//------------------------------------------------------------------------------
typedef volatile struct {
  volatile float regvalarr;
  const uint16_t regarr;
} sdm_struct;

volatile sdm_struct sdmarr[NBREG] = {
  {0.00, MFM_PHASE_1_VOLTAGE},                                                  //V
  {0.00, MFM_PHASE_1_CURRENT},                                                  //A
  {0.00, MFM_PHASE_1_POWER},                                                    //W
  {0.00, MFM_PHASE_1_POWER_FACTOR},                                             //PF
  {0.00, MFM_FREQUENCY}                                                         //Hz
};
//------------------------------------------------------------------------------
String getUptimeString() {
  uint16_t days;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;

  #define SECS_PER_MIN  60
  #define SECS_PER_HOUR 3600
  #define SECS_PER_DAY  86400

  time_t uptime = millis() / 1000;

  seconds = uptime % SECS_PER_MIN;
  uptime -= seconds;
  minutes = (uptime % SECS_PER_HOUR) / SECS_PER_MIN;
  uptime -= minutes * SECS_PER_MIN;
  hours = (uptime % SECS_PER_DAY) / SECS_PER_HOUR;
  uptime -= hours * SECS_PER_HOUR;
  days = uptime / SECS_PER_DAY;

  char buffer[20];
  sprintf(buffer, "%4u days %02d:%02d:%02d", days, hours, minutes, seconds);
  return buffer;
}
//------------------------------------------------------------------------------
void xmlrequest(AsyncWebServerRequest *request) {
  String XML = F("<?xml version='1.0'?><xml>");
  for (int i = 0; i < NBREG; i++) {
    XML += "<response" + (String)i + ">";
    XML += String(sdmarr[i].regvalarr,2);
    XML += "</response" + (String)i + ">";
  }
  XML += F("<sdmcnt>");
  XML += String(MFM.getSuccCount());
  XML += F("</sdmcnt>");
  XML += F("<errtotal>");
  XML += String(MFM.getErrCount());
  XML += F("</errtotal>");
  XML += F("<errcode>");
  XML += String(MFM.getErrCode());
  XML += F("</errcode>");
  XML += F("<upt>");
  XML += getUptimeString();
  XML += F("</upt>");
  XML += F("<freeh>");
  XML += String(ESP.getFreeHeap());
  XML += F("</freeh>");
  XML += F("</xml>");
  request->send(200, "text/xml", XML);
}
//------------------------------------------------------------------------------
void indexrequest(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", index_page);
}
//------------------------------------------------------------------------------
void ledOn() {
  digitalWrite(LED_BUILTIN, HIGH);
}
//------------------------------------------------------------------------------
void ledOff() {
  digitalWrite(LED_BUILTIN, LOW);
}
//------------------------------------------------------------------------------
void ledSwap() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}
//------------------------------------------------------------------------------
void otaInit() {
  ArduinoOTA.setHostname(devicename.c_str());

  ArduinoOTA.onStart([]() {
    ledOn();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    ledSwap();
  });
  ArduinoOTA.onEnd([]() {
    ledOff();
  });
  ArduinoOTA.onError([](ota_error_t error) {
    ledOff();
  });
  ArduinoOTA.begin();
}
//------------------------------------------------------------------------------
void serverInit() {
  server.on("/", HTTP_GET, indexrequest);
  server.on("/xml", HTTP_PUT, xmlrequest);
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404);
  });
  server.begin();
}
//------------------------------------------------------------------------------
static void wifiInit() {
  WiFi.persistent(false);                                                       // Do not write new connections to FLASH
  WiFi.mode(WIFI_STA);
#if defined ( USE_STATIC_IP )
  WiFi.config(ip, gateway, subnet);                                             // Set fixed IP Address
#endif
  WiFi.begin(wifi_ssid, wifi_password);

  while( WiFi.status() != WL_CONNECTED ) {                                      //  Wait for WiFi connection
    ledSwap();
    delay(100);
  }
}
//------------------------------------------------------------------------------
void sdmRead() {
  float tmpval = NAN;

  for (uint8_t i = 0; i < NBREG; i++) {
    tmpval = MFM.readVal(sdmarr[i].regarr);

    if (isnan(tmpval))
      sdmarr[i].regvalarr = 0.00;
    else
      sdmarr[i].regvalarr = tmpval;

    yield();
  }
}
//------------------------------------------------------------------------------
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  ledOn();

  wifiInit();
  otaInit();
  serverInit();
  MFM.begin();

  readtime = millis();

  ledOff();
}
//------------------------------------------------------------------------------
void loop() {
  ArduinoOTA.handle();

  if (millis() - readtime >= READMFMEVERY) {
    sdmRead();
    readtime = millis();
  }

  yield();
}
