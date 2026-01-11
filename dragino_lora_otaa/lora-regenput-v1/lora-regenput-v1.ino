//https://forum.arduino.cc/index.php?topic=645057.0
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#define ENABLE_LORA

//######################################
//# CONFIGURATION
//######################################
// Powershell to make byte arrays (optional reverse)
/*
$s = '70B3D57ED002BEF4'
$s = $s -replace '\s', ''
$a = [byte[]] ($s -replace '..', '0x$&,' -split ',' -ne '')
$as = $a | %{ "0x$($_.ToString("X2"))" }
$as = $as | Sort-Object -Descending {(++$Script:i)}  # Optional reverse
"{ $($as -join ', ') }"
*/

#include "arduino_secrets.h"
// Update interval:
const unsigned TX_INTERVAL = 10;
const unsigned TARGET_MIN = 25; // 0.3m = 300mm
const unsigned TARGET_MAX = 500; // 3m = 3000mm
const unsigned RETRIES = 3;

//######################################
//# SENSOR
//######################################

// defines pins numbers
const int trigPin = 4;
const int echoPin = 5;
// defines variables
long duration;
int distance_mm;

void init_data()
{
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
}

int get_data(uint8_t* buffer)
{
  int tries;
  int distance_mm = get_distance_with_retry(tries);

  buffer[0] = distance_mm & 0x00ff;
  buffer[1] = (distance_mm & 0xff00) >> 8;
  buffer[2] = tries;
  buffer[3] = 0;
  buffer[4] = 0;
  buffer[5] = 0;
  return 6;
}

int get_distance_with_retry(int &tries)
{
  int distance_mm;
  for (tries = 1; tries <= RETRIES; tries++) {
    distance_mm = get_distance();
    if (distance_mm >= TARGET_MIN && distance_mm <= TARGET_MAX) {
      break;
    }

    delay(500);
  }

  // Prints the distance on the Serial Monitor
  Serial.print("Distance: ");
  Serial.print(distance_mm);
  Serial.print("   Tries: ");
  Serial.println(tries);

  return distance_mm;
}

int get_distance()
{
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculating the distance
  // 331 m/s + 0.6 m/s/C * T --> 15°C ==> 339.4
  // 331 m/s + 0.6 m/s/C * T --> 15°C ==> 346.0
  distance_mm = (int)(duration * 0.34 / 2);

  return distance_mm;
}

//######################################
//# LORA
//######################################

static uint8_t mydata[12];

#ifdef ENABLE_LORA

void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16);}

static osjob_t sendjob;

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
        Serial.println("OP_TXRXPEND, not sending");
    } else {
        // Prepare upstream data transmission at the next possible time.
        int len = get_data(mydata);
        LMIC_setTxData2(1, mydata, len, 0);
        Serial.println("Packet queued");
        Serial.println(LMIC.freq);
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    Serial.println(ev);
    switch(ev) {
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
        case EV_JOINED:
            Serial.println("EV_JOINED");
            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);
            break;
        case EV_RFU1:
            Serial.println("EV_RFU1");
            break;
        case EV_JOIN_FAILED:
            Serial.println("EV_JOIN_FAILED");
            break;
        case EV_REJOIN_FAILED:
            Serial.println("EV_REJOIN_FAILED");
            break;
        case EV_TXCOMPLETE:
            Serial.println("EV_TXCOMPLETE (includes waiting for RX windows)");
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if(LMIC.dataLen) {
                // data received in rx slot after tx
                Serial.print("Data Received: ");
                Serial.write(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
                Serial.println();
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
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
    }
}

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
  
  //Serial.println("In this SW will send following information to network(uplink), you can see them in ThingPark Platform Wireless Logger window");
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

    Serial.begin(9600);
    while(!Serial);
    Serial.println("Starting");

    #ifdef ENABLE_LORA
    
    // LMIC init
    os_init();
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 1/100);
    LMIC_disableChannel(1);
    LMIC_disableChannel(2);
    printotaainformation();

    // Start job
    do_send(&sendjob);

    #endif
}

void loop() {
  
    #ifdef ENABLE_LORA
  
    os_runloop_once();

    #else

    int len = get_data(mydata);
    delay(1000 * TX_INTERVAL);
    
    #endif
}
