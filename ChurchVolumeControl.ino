#include <Wire.h>
#include <Adafruit_DS1841.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#include "Rotary.h"

#define LED 2

// soft AP
const char *AP_ssid = "ChurchVolumeControl";
const char *AP_password = "stmattsav";

bool wifiConnected = false;
bool configMode = false;

ESP8266WebServer webServer;

Adafruit_7segment matrix = Adafruit_7segment();
Adafruit_DS1841  ds;

int wiperValue = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("7 Segment Backpack Test");

  if (!matrix.begin(0x70)) {
    Serial.println("Failed to find 7 segment display");
    while (1) { delay(10); }
  }
  Serial.println("7 segment display Found!");

  if (!ds.begin()) {
    Serial.println("Failed to find DS1841 chip");
    while (1) { delay(10); }
  }
  Serial.println("DS1841 Found!");

  setupRotaryInterrupt();

  // start the wifi soft AP
  Serial.print("Establishing ");
  Serial.println(AP_ssid);
  WiFi.softAP(AP_ssid, AP_password);
 
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // mount the flash filesystem
  if (SPIFFS.begin())
    Serial.println("SPIFFS mounted");
  else
    Serial.println("SPIFFS mount failed");

  webServer.on("/connect", []() {
    Serial.println("Got /connect request");
    String ssid, passwd;
    if (webServer.hasArg("ssid")) {
      ssid = webServer.arg("ssid");
    }
    if (webServer.hasArg("passwd")) {
      passwd = webServer.arg("passwd");
    }
    if (ssid.length() != 0) {
      Serial.print("Connecting to ");
      Serial.println(ssid);
      WiFi.mode(WIFI_AP_STA);
      WiFi.begin(ssid, passwd);
      configMode = false;
    }
    else {
      Serial.println("SSID is empty");
    }
    webServer.send(200, "text/html", "");
  });

  webServer.on("/config-mode", []() {
    Serial.println("Got /config-mode request");
    webServer.send(200, "text/html", String(configMode ? 1 : 0));
  });

  webServer.on("/volume", []() {
    Serial.println("Got /volume request");
    webServer.send(200, "text/html", String(wiperValue));
  });

  webServer.on("/set-volume", HTTP_POST, []() {
    Serial.print("Got /set-volume ");
    Serial.print(wiperValue);
    Serial.println(" request");
    if (webServer.hasArg("volume")) {
      wiperValue = webServer.arg("volume").toInt();
    }
    webServer.send(200, "text/html", "");
  });

  webServer.onNotFound([&]() {
    Serial.print("file request: ");
    Serial.println(webServer.uri());
    if (!handleFileRead(webServer.uri())) {
      webServer.send(404);
    }
  });
  
  webServer.begin();

  // start connecting to the target wifi AP if the encoder button is pressed
  if (digitalRead(ROTARY_BUTTON) == 0) {
    Serial.println("Entering Configuration Mode");
    WiFi.mode(WIFI_AP);
    digitalWrite(LED, 1);
    configMode = true;
  }
  else {
    String ssid = WiFi.SSID();
    if (ssid.length() != 0) {
      Serial.print("Connecting to ");
      Serial.println(ssid);
      WiFi.mode(WIFI_AP_STA);
      WiFi.begin();
      wifiConnected = false;
    }
    else {
      Serial.println("No SSID set. Entering Configuration Mode");
      WiFi.mode(WIFI_AP);
      digitalWrite(LED, 0);
      configMode = true;
    }
  }

  Serial.print("Current VCC Voltage:"); Serial.print(ds.getVoltage()); Serial.println("mV");
  Serial.print("Temperature: ");Serial.print(ds.getTemperature());Serial.println(" degrees C");
}

void loop() {

  // check to see if wifi is still connected
  if (wifiConnected && WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
  }
    
  // handle wifi connection to the target
  if (!wifiConnected) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected to target");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      if (MDNS.begin("churchvolumecontrol"))
        Serial.println("mDNS started");
      wifiConnected = true;
    }
  }

  RotaryState rotaryState;
  if (getRotaryState(rotaryState)) {

    wiperValue += rotaryState.delta;

    if (wiperValue > 127)
      wiperValue = 127;
    else if (wiperValue < 0)
      wiperValue = 0;

    matrix.print(wiperValue);
    matrix.writeDisplay();

    ds.setWiper(wiperValue);
  }
  
  webServer.handleClient();
}

//code from fsbrowser example, consolidated.
bool handleFileRead(String path) {

  // default to index.html if nothing else was specified
  if (path.length() == 0) path = "/";
  if (path.endsWith("/")) path += "index.html";

  // dtermine content type
  String contentType;
  if (path.endsWith(".html")) contentType = "text/html";
  else if (path.endsWith(".css")) contentType = "text/css";
  else if (path.endsWith(".js")) contentType = "application/javascript";
  else if (path.endsWith(".png")) contentType = "image/png";
  else if (path.endsWith(".gif")) contentType = "image/gif";
  else if (path.endsWith(".jpg")) contentType = "image/jpeg";
  else if (path.endsWith(".ico")) contentType = "image/x-icon";
  else if (path.endsWith(".xml")) contentType = "text/xml";
  else if (path.endsWith(".pdf")) contentType = "application/x-pdf";
  else if (path.endsWith(".zip")) contentType = "application/x-zip";
  else if (path.endsWith(".gz")) contentType = "application/x-gzip";
  else if (path.endsWith(".json")) contentType = "application/json";
  else contentType = "text/plain";

  // split filepath and extension
  String prefix = path, ext = "";
  int lastPeriod = path.lastIndexOf('.');
  if (lastPeriod >= 0) {
    prefix = path.substring(0, lastPeriod);
    ext = path.substring(lastPeriod);
  }

  //look for smaller versions of file
  //minified file, good (myscript.min.js)
  if (SPIFFS.exists(prefix + ".min" + ext)) path = prefix + ".min" + ext;
  //gzipped file, better (myscript.js.gz)
  if (SPIFFS.exists(prefix + ext + ".gz")) path = prefix + ext + ".gz";
  //min and gzipped file, best (myscript.min.js.gz)
  if (SPIFFS.exists(prefix + ".min" + ext + ".gz")) path = prefix + ".min" + ext + ".gz";

  // send the file if it exists
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    if (webServer.hasArg("download"))
      webServer.sendHeader("Content-Disposition", " attachment;");
    if (webServer.uri().indexOf("nocache") < 0)
      webServer.sendHeader("Cache-Control", " max-age=172800");

    //optional alt arg (encoded url), server sends redirect to file on the web
    if (WiFi.status() == WL_CONNECTED && webServer.hasArg("alt")) {
      webServer.sendHeader("Location", webServer.arg("alt"), true);
      webServer.send ( 302, "text/plain", "");
    } else {
      //server sends file
      size_t sent = webServer.streamFile(file, contentType);
    }
    file.close();
    return true;
  }

  // file not found
  return false;
}
