/*
  SD card read/write

  This example shows how to read and write data to and from an SD card file
  The circuit:
   SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13

  created   Nov 2010
  by David A. Mellis
  modified 9 Apr 2012
  by Tom Igoe

  This example code is in the public domain.

*/
//DRIVERS FOR SCREEN AND SD MEMORY CARD
#include <SPI.h>
#include "SdFat.h" //use version 1.1.4
#include "sdios.h"
#include <Adafruit_GFX.h>      // include adafruit graphics library
#include <Adafruit_PCD8544.h>  // include adafruit PCD8544 (Nokia 5110) library
#include "RTClib.h"
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//DRIVERS FOR INTERNET PROTOCOLS
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
//DRIVERS FOR CUSTOM FONTS AND USER INTERFACE
#include "PropFont.h"
#include "c64enh_font.h"
#include "tiny3x7_font.h"
#include "Digits3x5_font.h"
#include "Small4x5_font.h"
#include "Term8x14_font.h"
#define SCR_WD   84
#define SCR_HT   48
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
#define TEST_FILE "Cluster.test"
// Nokia 5110 LCD module connections (CLK, DIN, D/C, CS, RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(2, 0, 3, 1, 16);
SdFat SD;
csd_t csd;
#define SD_CS_PIN 15
File mon_dossier;
SdFile dossier;
ArduinoOutStream cout(Serial);
ESP8266WebServer server(80);
PropFont font;
// needed for PropFont library initialization, define your drawPixel and fillRect
void customPixel(int x, int y, int c) {
  display.drawPixel(x, y, c);
}
void customRect(int x, int y, int w, int h, int c) {
  display.fillRect(x, y, w, h, c);
}
String capacity;
uint8_t lineNum = 0;
#define lineHeight 6
void dessine(char* input, bool NewLine) {
  if (NewLine)lineNum += lineHeight; else customRect(0, lineNum, 84, lineHeight, 0);
  font.printStr(ALIGN_LEFT, lineNum, input);
  display.display();
  if (lineNum >= 84) {
    lineNum = 0;
    display.clearDisplay();
  }
}

int i = 0;
int statusCode;
const char* ssid = "text";
const char* passphrase = "text";
String st;
String content;
long offline_timer = 0;

void launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}

char* HSSID = "";
const char* HPSWD = "INALTERISVITIS";
bool Hapsify() {
  bool coniectens = 0;
  for (int i = 0; i <= WiFi.scanNetworks(); i++) {
    ESP.wdtFeed();
    if (WiFi.SSID(i).substring(0, 6) == "HAPSIS") {
      HSSID = (char*)WiFi.SSID(i).c_str();
      Serial.println(HSSID);
      coniectens = 1;
      WiFi.mode(WIFI_STA);
      WiFi.begin(HSSID, HPSWD);
      return (coniectens);
    } else {
      Serial.println("Essayant pour obtenir connexion");
    }
  }
  return (0);
}
void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);

    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP("HAPSIS:HIEROMNEMON", "");
  Serial.println("softap");
  launchWeb();
  Serial.println("over");
}

void createWebServer()
{ server.on("/", []() {
    IPAddress ip = WiFi.softAPIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    content = "<!DOCTYPE HTML>\r\n<html><h1>Hieromnemon Configuration Interface <br/></h1>";
    content += "<form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><br/><label>PASSCODE: </label><input name='pass' length=64><br/><label>Sensor hotspot prefix: </label><input name='prefix' length=32><br/><label>Sensor data source HTTP address: </label><input name='addr' length=32><br/><label>TIMEZONE: </label><input name='timezone' length=32><br/><label>SAVE:</label><input type='submit'></form>";
    content += "</html>";
    server.send(200, "text/html", content);
  });
  server.on("/scan", []() {
    //setupAP();
    IPAddress ip = WiFi.softAPIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

    content = "<!DOCTYPE HTML>\r\n<html>go back";
    server.send(200, "text/html", content);
  });

  server.on("/setting", []() {
    String qsid = server.arg("ssid");
    String qpass = server.arg("pass");
    String qcity = server.arg("prefix");
    String qcountry = server.arg("addr");
    String qtimezone = server.arg("timezone");
    if (qsid.length() > 0 && qpass.length() > 0) {
      Serial.println("clearing eeprom");
      for (int i = 0; i < 256; ++i) {
        EEPROM.write(i, 0);
      }
      Serial.println(qsid);
      Serial.println("");
      Serial.println(qpass);
      Serial.println("");

      Serial.println("writing eeprom ssid:");
      for (int i = 0; i < qsid.length(); ++i)
      {
        EEPROM.write(i, qsid[i]);
        Serial.print("Wrote: ");
        Serial.println(qsid[i]);
      }
      Serial.println("writing eeprom pass:");
      for (int i = 0; i < qpass.length(); ++i)
      {
        EEPROM.write(32 + i, qpass[i]);
        Serial.print("Wrote: ");
        Serial.println(qpass[i]);
      } for (int i = 0; i < qcity.length(); ++i)
      {
        EEPROM.write(96 + i, qcity[i]);
        Serial.print("Wrote: ");
        Serial.println(qcity[i]);
      } for (int i = 0; i < qcountry.length(); ++i)
      {
        EEPROM.write(128 + i, qcountry[i]);
        Serial.print("Wrote: ");
        Serial.println(qcountry[i]);
      } for (int i = 0; i < qtimezone.length(); ++i)
      {
        EEPROM.write(160 + i, qtimezone[i]);
        Serial.print("Wrote: ");
        Serial.println(qtimezone[i]);
      }
      EEPROM.commit();

      content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
      statusCode = 200;
      ESP.reset();
    }
    else {
      content = "{\"Error\":\"404 not found\"}";
      statusCode = 404;
      Serial.println("Sending 404");
    }
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(statusCode, "application/json", content);

  });
}
void screenInit() {
  font.init(customPixel, customRect, SCR_WD, SCR_HT); // custom drawPixel and fillRect function and screen width and height values
  font.setFont(Small4x5PL);
  display.begin();
  display.setRotation(2);
  display.setContrast(58);
  font.setScale(1);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.clearDisplay();
}
void rtcBegin() {
  rtc.begin();
  if (rtc.lostPower()) {
    NTP_RTC_update();
  }
}
String esid;
String epass = "";
String etimezone = "";
void mountEEPROM() {
  EEPROM.begin(512); //Initialasing EEPROM
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  char espace = ' ';
  String ecity = "";
  for (int i = 96; i < 128; ++i)
  {
    if (char(EEPROM.read(i)) != espace)
      ecity += char(EEPROM.read(i));
    else ecity += "\0";
  }
  ecity.trim();
  String ecountry = "";
  for (int i = 128; i < 160; ++i)
  {
    if (char(EEPROM.read(i)) != espace)
      ecountry += char(EEPROM.read(i));
    else ecountry += "\0";
  }
  for (int i = 160; i < 192; ++i)
  {
    if (char(EEPROM.read(i)) != espace)
      etimezone += char(EEPROM.read(i));
    else etimezone += "\0";
  }
  ecountry.trim();
}
void mountSD() {
  dessine("SYSTEMA INITIANS", 0);
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("initialization failed!");
    dessine("!ERROR", 0);
    return;
  }
  SD.card()->readCSD(&csd);
  capacity = String(((float)SD.freeClusterCount() / SD.clusterCount()) * 0.000512 * csd.capacity());
  dessine((char*)String(capacity + "Mo LIBER").c_str(), 0); //display free space
}
bool extendu = 0;
long uptimeTimer = 0;
void extendConfigPage() {
  if (!extendu) {
    launchWeb();
    setupAP();
    extendu = 1;
  }
  ESP.wdtFeed();
  server.handleClient();
}
void NTP_init() {
  timeClient.begin();
  timeClient.setTimeOffset(etimezone.toInt() * 3600);
}
void NTP_RTC_update() {
  dessine("SYNC DE NTP",1);
  WiFi.mode(WIFI_STA);
  WiFi.begin(esid, epass);
  int conxTimer = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    if(millis()-conxTimer>10000){
      clear_display();
      dessine("!SINE CONEXIONE",1);
      dessine("---------------------",1);
      dessine("PAGINA",1);
      dessine("CONFIGURATIONIS",1);
      dessine("HAPSIS:HIEROMNEMON",1);
      dessine("@192.168.4.1",1);
      while(1){
        extendConfigPage();
        
      }
    }
  }
  while (!timeClient.update()) {
      timeClient.forceUpdate();
  }
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  rtc.adjust(DateTime(ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()));
}
void setup() {
  ESP.wdtDisable();
  *((volatile uint32_t*) 0x60000900) &= ~(1);  //disable hardware wdt
  screenInit();
  mountEEPROM();
  mountSD();
  NTP_init();
  rtcBegin();
  WiFi.forceSleepBegin();
}
void clear_display() {
  lineNum = 0;
  display.clearDisplay();
}
void loop() {
  DateTime now = rtc.now();
  clear_display();
  dessine((char*)(String(rtc.getTemperature()) + "'C " + capacity + "Mo").c_str(), 0);
  String zeroH="";
  String zeroM="";
  String zeroS="";
  if(now.hour()<10)zeroH="0";
  if(now.minute()<10)zeroM="0";
  if(now.second()<10)zeroS="0";
  dessine((char*)(String(now.year(), DEC) + "/" + String(now.month(), DEC) + "/" + String(now.day(), DEC) + " " +zeroH+ String(now.hour(), DEC) + ":" +zeroM+ String(now.minute(), DEC) + ":" +zeroS+ String(now.second(), DEC)).c_str(), 1);
  dessine("---------------------",1);
  dessine("INSPICIENS HOSPITES",1);
  dessine("---------------------",1);
  delay(1000);
}
