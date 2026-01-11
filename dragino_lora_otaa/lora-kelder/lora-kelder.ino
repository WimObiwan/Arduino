#define INCLUDE_TEMP
#define INCLUDE_LOG

//https://forum.arduino.cc/index.php?topic=645057.0
// ""MCCI LoRaWAN LMIC Library"" doesn't work!
// Use deprecated ""IBM LMIC framework"" (1.5.1) instead!
#include <lmic.h>
#include <hal/hal.h>

#include "QuickMedianLib.h"

#ifdef INCLUDE_TEMP
#include "DHT.h"  // DHT Sensor Library
#endif

#define DEBUG

static uint8_t mydata[12];
static osjob_t sendjob;

//######################################
//# CONFIGURATION
//######################################
#include "arduino_secrets.h"
// Update interval:
//static const unsigned TX_INTERVAL = 12;
static const unsigned TX_INTERVAL = 1200-16; // 16 seconds overlap
const unsigned TARGET_MIN = 100; // 0.3m = 300mm = minimal of sensor
const unsigned TARGET_MAX = 1000; // 1m = 1000mm
const unsigned RETRIES = 10;

//######################################
//# SENSOR
//######################################

// defines pins numbers
static const int trigPin = 4;
static const int echoPin = 5;
// defines variables
long duration;
int distance_mm;

#ifdef INCLUDE_TEMP
static const int dhtPin = 3;
static const int dhtType = DHT11;

DHT dht(dhtPin, dhtType);
#endif

void init_data()
{
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  #ifdef INCLUDE_TEMP
  dht.begin();
  #endif
}

int get_data(uint8_t* buffer)
{
  int distance_mm = get_distance_with_retry();

  // distance_mm 0..8191 (8m)
  buffer[0] = distance_mm & 0x00ff;
  buffer[1] = (distance_mm & 0x1f00) >> 8;
  buffer[2] = 0;

  #ifdef INCLUDE_TEMP

  int humid_prc_x10 = get_humidity_x10();
  int temp_c_x10 = get_temp_x10();

  // humid_prc_x10 0..1023 ("102.3%")
  buffer[3] = humid_prc_x10 & 0x00ff;
  buffer[1] += ((humid_prc_x10 & 0x0300) >> 8) << 6;

  // temp_c_x10 0..512 (51.2°C)
  buffer[4] = temp_c_x10 & 0x0ff;
  buffer[1] += ((temp_c_x10 & 0x0100) >> 8) << 5;

  #else

  buffer[3] = 0;
  buffer[4] = 0;

  #endif

  buffer[5] = 0;

  return 6;
}

int get_distance_with_retry()
{
  int distance_mm[RETRIES];
  int count = 0;
  for (int tries = 0; tries < RETRIES; tries++) {
    if (tries > 0)
        delay(1000);
        
    distance_mm[count] = get_distance();
    if (distance_mm[count] >= TARGET_MIN && distance_mm[count] <= TARGET_MAX) {
      count++;
    }
  }

  // Prints the distance on the Serial Monitor
  #ifdef INCLUDE_LOG
  Serial.print("Distance: ");
  Serial.print(QuickMedian<int>::GetMedian(distance_mm, count));
  Serial.print("   Tries: ");
  Serial.println(count);
  #endif

  return QuickMedian<int>::GetMedian(distance_mm, count);
}

int get_distance()
{
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculating the distance
  // 331 m/s + 0.6 m/s/C * T --> 15°C ==> 339.4
  distance_mm = (int)(duration * 0.34 / 2);

  // Prints the distance on the Serial Monitor
  #ifdef INCLUDE_LOG
  Serial.print("Distance: ");
  Serial.println(distance_mm);
  #endif

  return distance_mm;
}

#ifdef INCLUDE_TEMP
int get_humidity_x10()
{
  float h = dht.readHumidity();
  // Prints the humidity on the Serial Monitor
  #ifdef INCLUDE_LOG
  Serial.print("Humidity_Prc: ");
  Serial.println(h);
  #endif

  if (isnan(h))
    return 0;
  else
    return (int)(h * 10);
}

int get_temp_x10()
{
  float t = dht.readTemperature();
  // Prints the temperature on the Serial Monitor

  #ifdef INCLUDE_LOG
  Serial.print("Temperature_C: ");
  Serial.println(t);
  #endif

  if (isnan(t))
    return 0;
  else
    return (int)(t * 10);
}
#endif


//######################################
//# LORA
//######################################

void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16);}

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 6, 7},
};

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        #if defined(INCLUDE_LOG) && defined(DEBUG)
        Serial.println("OP_TXRXPEND, not sending");
        #endif
    } else {
        // Prepare upstream data transmission at the next possible time.
        int len = get_data(mydata);
        LMIC_setTxData2(1, mydata, len, 0);
        #if defined(INCLUDE_LOG) && defined(DEBUG)
        Serial.println("Packet queued");
        Serial.println(LMIC.freq);
        #endif
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent (ev_t ev) {
    #ifdef DEBUG
    Serial.print(os_getTime());
    Serial.print(": ");
    Serial.println(ev);
    #endif
    switch(ev) {
        #if defined(INCLUDE_LOG) && defined(DEBUG)
        case EV_SCAN_TIMEOUT:
            Serial.println("EV_SCAN_TIMEOUT");
            break;
        case EV_BEACON_FOUND:
            Serial.println("EV_BEACON_FOUND");
            break;
        case EV_BEACON_MISSED:
            Serial.println("EV_BEACON_MISSED");
            break;
        case EV_BEACON_TRACKED:
            Serial.println("EV_BEACON_TRACKED");
            break;
        case EV_JOINING:
            Serial.println("EV_JOINING");
            break;
        #endif
        case EV_JOINED:
            #if defined(INCLUDE_LOG) && defined(DEBUG)
            Serial.println("EV_JOINED");
            #endif
            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);
            break;
        #if defined(INCLUDE_LOG) && defined(DEBUG)
        case EV_RFU1:
            Serial.println("EV_RFU1");
            break;
        case EV_JOIN_FAILED:
            Serial.println("EV_JOIN_FAILED");
            break;
        case EV_REJOIN_FAILED:
            Serial.println("EV_REJOIN_FAILED");
            break;
        #endif
        case EV_TXCOMPLETE:
            #if defined(INCLUDE_LOG) && defined(DEBUG)
            Serial.println("EV_TXCOMPLETE (includes waiting for RX windows)");
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if(LMIC.dataLen) {
                // data received in rx slot after tx
                Serial.print("Data Received: ");
                Serial.write(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
                Serial.println();
            }
            #endif
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        #if defined(INCLUDE_LOG) && defined(DEBUG)
        case EV_LOST_TSYNC:
            Serial.println("EV_LOST_TSYNC");
            break;
        case EV_RESET:
            Serial.println("EV_RESET");
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println("EV_RXCOMPLETE");
            break;
        case EV_LINK_DEAD:
            Serial.println("EV_LINK_DEAD");
            break;
        case EV_LINK_ALIVE:
            Serial.println("EV_LINK_ALIVE");
            break;
        default:
            Serial.println("Unknown event");
            break;
        #endif
    }
}

#if defined(INCLUDE_LOG) && defined(DEBUG2)
void printotaainformation(void)
{
  unsigned char i;
  unsigned char chartemp;
  unsigned char messagelength;

  Serial.println(F("OTAA mode to join network"));
  Serial.print("DevEui: ");
  for (i = 0; i <= 7; i++)
  {
    chartemp = pgm_read_word_near(DEVEUI+7-i);
    covertandprint((chartemp >> 4) & 0xf);
    covertandprint(chartemp & 0xf);    
  }
  Serial.println("");
  Serial.print("AppEui: ");
  for (i = 0; i <=7; i++)
  {
    chartemp = pgm_read_word_near(APPEUI+7-i);
    covertandprint((chartemp >> 4) & 0xf);
    covertandprint(chartemp & 0xf);    
  }

  Serial.println("");
  Serial.print("AppKey: ");
  for (i = 0; i <= 15; i++)
  {
    chartemp = pgm_read_word_near(APPKEY+i);
    //Serial.print(buftemp[i],HEX);
    covertandprint((chartemp >> 4) & 0xf);
    covertandprint(chartemp & 0xf);
  }
  Serial.println("");
  
  Serial.println("In this SW will send following information to network(uplink), you can see them in ThingPark Platform Wireless Logger window");
  Serial.print((char*)mydata);
  Serial.println("");
  Serial.println(""); // add one new line
}

void covertandprint(unsigned char value)
{
  switch (value)
  {
    case 0  : Serial.print("0"); break;
    case 1  : Serial.print("1"); break;
    case 2  : Serial.print("2"); break;
    case 3  : Serial.print("3"); break;
    case 4  : Serial.print("4"); break;
    case 5  : Serial.print("5"); break;
    case 6  : Serial.print("6"); break;
    case 7  : Serial.print("7"); break;
    case 8  : Serial.print("8"); break;
    case 9  : Serial.print("9"); break;
    case 10  : Serial.print("A"); break;
    case 11  : Serial.print("B"); break;
    case 12  : Serial.print("C"); break;
    case 13  : Serial.print("D"); break;
    case 14  : Serial.print("E"); break;
    case 15 :  Serial.print("F"); break;
    default :
      Serial.print("?");   break;
  }
}
#endif

//######################################
//# ARDUINO
//######################################

void setup() {
    init_data();

    #if defined(INCLUDE_LOG)
    Serial.begin(115200);
    while(!Serial);
    Serial.println("Starting");
    #endif

    // LMIC init
    os_init();
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 1/100);
    LMIC_disableChannel(1);
    LMIC_disableChannel(2);
    #if defined(INCLUDE_LOG) && defined(DEBUG2)
    printotaainformation();
    #endif

    // Start job
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
