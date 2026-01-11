/*
  Web client

  This sketch connects to a website (http://www.google.com)
  using the WiFi module.

  This example is written for a network using WPA encryption. For
  WEP or WPA, change the WiFi.begin() call accordingly.

  This example is written for a network using WPA encryption. For
  WEP or WPA, change the WiFi.begin() call accordingly.


  created 13 July 2010
  by dlf (Metodo2 srl)
  modified 31 May 2012
  by Tom Igoe

  Find the full UNO R4 WiFi Network documentation here:
  https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples#wi-fi-web-client
 */


#include "WiFiS3.h"
#include "ArduinoHttpClient.h"
#include "Adafruit_NeoPixel.h"
#include "ArduinoJson.h"

#include "arduino_secrets.h" 

#define LEDCOUNT 10
int LedToGroup[LEDCOUNT] = { 46, 16, 31, 48, 21, 17, 0, 0, 0, 0 };
#define DIM 10

/* -------------------------------------------------------------------------- */
void setup() 
/* -------------------------------------------------------------------------- */
{
  initializeLeds();
  
  ConnectWiFi();
}


/* -------------------------------------------------------------------------- */
void loop() {
/* -------------------------------------------------------------------------- */
  uint32_t colors[LEDCOUNT] { 0xffffffff};

  read_mon(colors);

  //for (int i = 0; i < LEDCOUNT; i++)
  //  colors[i] = GetColor(0, 0, 255);
  //colors[0] = GetColor(100, 0, 0);
  //colors[1] = GetColor(0, 100, 0);
  //colors[2] = GetColor(0, 0, 100);

  setLeds(colors);
  
  delay(10000);
}




///////////////////////////////////////////////////
// COMMUNICATION
///////////////////////////////////////////////////

#define DEBUG_WIFI
//#define DEBUG_MON

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
char server[] = "node.foxinnovations.be";    // name address for Google (using DNS)

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiSSLClient client;
HttpClient httpClient(client, server, 443);

/* -------------------------------------------------------------------------- */
void ConnectWiFi()
/* -------------------------------------------------------------------------- */
{
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
     
    // wait 10 seconds for connection:
    delay(10000);
  }

  #ifdef DEBUG_WIFI
  printWifiStatus();
  #endif
}

/* -------------------------------------------------------------------------- */
void read_mon(uint32_t* colors)
/* -------------------------------------------------------------------------- */
{
  String url = "/mon/rollup?groupids=[";
  bool first = true;
  for (int i = 0; i < LEDCOUNT; i++) {
    int group = LedToGroup[i];
    if (group != 0) {
      if (first) {
        first = false;      
      } else {
        url = url + ',';
      }
      url = url + group;
    }
  }

  url = url + "]&token=" + TOKEN;
  
  //char url[256];
  //sprintf(url, "/mon/rollup?groupids=[46,16,31,48,21,17]&token=%s", TOKEN);

  Serial.print("URL: ");
  Serial.println(url);
  
  httpClient.get(url);

  // read the status code and body of the response
  int statusCode = httpClient.responseStatusCode();

  Serial.print("Status code: ");
  Serial.println(statusCode);

  String response = httpClient.responseBody();
  Serial.print("Response: ");
  Serial.println(response);

  // if the server's disconnected, stop the client:
  //if (!client.connected()) {
  //  Serial.println();
  //  Serial.println("disconnecting from server.");
  //  client.stop();
  //}

  if (statusCode == 200) {
    read_response(response, colors);
  } else {
    Serial.println("API call failed");    
  }
}

/* -------------------------------------------------------------------------- */
void read_response(const String& response, uint32_t* colors)
/* -------------------------------------------------------------------------- */  
{
  Serial.println("JsonDocument:");
  JsonDocument doc;  

  //deserializeJson(doc, httpClient.stream());
  
  //const char *test = "[{\"groupid\":48,\"maxSeverity\":null,\"rgb\":[0,89,0]},{\"groupid\":31,\"maxSeverity\":3,\"rgb\":[255,160,89]},{\"groupid\":16,\"maxSeverity\":2,\"rgb\":[255,200,89]},{\"groupid\":46,\"maxSeverity\":2,\"rgb\":[255,200,89]},{\"groupid\":17,\"maxSeverity\":null,\"rgb\":[0,89,0]},{\"groupid\":21,\"maxSeverity\":4,\"rgb\":[233,118,89]}])";
  //deserializeJson(doc, test);

  deserializeJson(doc, response);

  for (int i = 0; i < LEDCOUNT; i++) {
    colors[i] = GetColor(0, 0, 0);
  }
    
  for (int i = 0; i < 10; i++) {
    int groupId = doc[i]["groupid"].as<int>();
    byte r = doc[i]["rgb"][0].as<byte>();
    byte g = doc[i]["rgb"][1].as<byte>();
    byte b = doc[i]["rgb"][2].as<byte>();

    if (groupId > 0) {
      for (int j = 0; j < LEDCOUNT; j++) {
        if (LedToGroup[j] == groupId) {
          colors[j] = GetColor(r, g, b);
          Serial.println(String("Led ") + j + ": " + colors[j]);
        }
      }
      char sz[50];
      sprintf(sz, "GroupId %i RGB %i %i %i", groupId, (int)r, (int)g, (int)b);
      Serial.println(sz);
    }
  }
}

#ifdef DEBUG_WIFI
/* -------------------------------------------------------------------------- */
void printWifiStatus() {
/* -------------------------------------------------------------------------- */  
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
#endif




///////////////////////////////////////////////////
// LEDS
///////////////////////////////////////////////////

#define LEDPIN 2

Adafruit_NeoPixel strip(LEDCOUNT, LEDPIN, NEO_GRB + NEO_KHZ800);

uint32_t GetColor(byte r, byte g, byte b) // RGB+white
{
  char sz[50];
  sprintf(sz, "COLOR FROM RGB %i %i %i", (int)r, (int)g, (int)b);
  Serial.println(sz);

  r = (byte)(((int)r) * DIM / 100);
  g = (byte)(((int)g) * DIM / 100);
  b = (byte)(((int)b) * DIM / 100);
  
  sprintf(sz, "COLOR TO RGB %i %i %i", (int)r, (int)g, (int)b);
  Serial.println(sz);

  return strip.Color(r, g, b);
}

void initializeLeds()
{
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void setLeds(uint32_t* colors)
{
  for (int i = 0; i < LEDCOUNT; i++) {
    Serial.println(String("Led ") + i + ": " + colors[i]);
    strip.setPixelColor(i, colors[i]);
  }

  strip.show();
}
