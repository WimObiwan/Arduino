#include <Wire.h>

#define ENABLE_LOGGING
#define ENABLE_RELAY
#define ENABLE_DHT
#define ENABLE_DISPLAY
// Buzzer is never tested
#define ENABLE_BUZZER
// Enable 1 (or no) LEVEL sensors:
//#define ENABLE_LEVELCAPACITIVE10
#define ENABLE_LEVELULTRASONIC

#define INITIAL_PUMP_ON 10
// Below this value, pump is turned off
#define THRESHOLD_LOW 15 //mm
// Above this value, pump is turned on
#define THRESHOLD_HIGH 45 //mm
// Alarm level for a buzzer
#define THRESHOLD_ALARM 50 //mm


// Height of the distance sensor (only used for Ultrasonic)
// when sensor measures this distance, level is 0mm
#define SENSOR_OFFSET 120 //mm

#define PUMPINTERVAL_MIN 45ul
#define PUMPINTERVAL_MAX 60ul
#define WAITINTERVAL 3600ul

//================================================================================
// Ultrasonic

#ifdef ENABLE_LEVELULTRASONIC

#include "Ultrasonic.h"

#define PIN_ULTRASONIC 2 // D2

Ultrasonic ultrasonic(PIN_ULTRASONIC);

inline void swap(long &a, long &b)
{
  long t = a;
  a = b;
  b = t;
}

long median5(long a, long b, long c, long d, long e)
{
  if (a > b) swap(a, b);
  if (c > d) swap(c, d);
  if (a > c) swap(a, c);
  if (b > d) swap(b, d);
  if (b > c) swap(b, c);
  if (d > e) swap(d, e);
  if (c > d) swap(c, d);
  return c;
}

int getLevelUltrasonic(char* szDebug)
{
  long v1 = ultrasonic.MeasureInMillimeters();
  delay(100);
  long v2 = ultrasonic.MeasureInMillimeters();
  delay(100);
  long v3 = ultrasonic.MeasureInMillimeters();
  delay(100);
  long v4 = ultrasonic.MeasureInMillimeters();
  delay(100);
  long v5 = ultrasonic.MeasureInMillimeters();

  int level = (int)median5(v1, v2, v3, v4, v5);

  if (level >= SENSOR_OFFSET)
    return 0;

  return SENSOR_OFFSET - level;
}

void initLevelUltrasonic() {
}

#endif // ENABLE_LEVELULTRASONIC


//================================================================================
// I2C LEVEL 10 CM

#ifdef ENABLE_LEVELCAPACITIVE10

unsigned char low_data[8] = {0};
unsigned char high_data[12] = {0};


#define NO_TOUCH       0xFE
#define THRESHOLD      100
#define ATTINY1_HIGH_ADDR   0x78
#define ATTINY2_LOW_ADDR   0x77

void getHigh12SectionValue(void)
{
  memset(high_data, 0, sizeof(high_data));
  Wire.requestFrom(ATTINY1_HIGH_ADDR, 12);
  while (12 != Wire.available());

  for (int i = 0; i < 12; i++) {
    high_data[i] = Wire.read();
  }
  delay(10);
}

void getLow8SectionValue(void)
{
  memset(low_data, 0, sizeof(low_data));
  Wire.requestFrom(ATTINY2_LOW_ADDR, 8);
  while (8 != Wire.available());

  for (int i = 0; i < 8 ; i++) {
    low_data[i] = Wire.read(); // receive a byte as character
  }
  delay(10);
}

int getLevelCapacitive10(char* szDebug)
{
  int sensorvalue_min = 250;
  int sensorvalue_max = 255;
  int low_count = 0;
  int high_count = 0;
//  while (1)
//  {
    uint32_t touch_val = 0;
    uint8_t trig_section = 0;
    low_count = 0;
    high_count = 0;
    getLow8SectionValue();
    getHigh12SectionValue();

    #ifdef ENABLE_LOGGING
    Serial.println("low 8 sections value = ");
    #endif
    for (int i = 0; i < 8; i++)
    {
      #ifdef ENABLE_LOGGING
      Serial.print(low_data[i]);
      Serial.print(".");
      #endif
      if (low_data[i] >= sensorvalue_min && low_data[i] <= sensorvalue_max)
      {
        low_count++;
      }
      #ifdef ENABLE_LOGGING
      if (low_count == 8)
      {
        Serial.print("      ");
        Serial.print("PASS");
      }
      #endif
    }
    #ifdef ENABLE_LOGGING
    Serial.println("  ");
    Serial.println("  ");
    Serial.println("high 12 sections value = ");
    #endif
    for (int i = 0; i < 12; i++)
    {
      #ifdef ENABLE_LOGGING
      Serial.print(high_data[i]);
      Serial.print(".");
      #endif

      if (high_data[i] >= sensorvalue_min && high_data[i] <= sensorvalue_max)
      {
        high_count++;
      }
      #ifdef ENABLE_LOGGING
      if (high_count == 12)
      {
        Serial.print("      ");
        Serial.print("PASS");
      }
      #endif
    }

    #ifdef ENABLE_LOGGING
    Serial.println("  ");
    Serial.println("  ");
    #endif

    for (int i = 0 ; i < 8; i++) {
      if (low_data[i] > THRESHOLD) {
        touch_val |= 1 << i;
      }
    }
    for (int i = 0 ; i < 12; i++) {
      if (high_data[i] > THRESHOLD) {
        touch_val |= (uint32_t)1 << (8 + i);          
      }
    }

    char szText[10];
    sprintf(szText, "%03x", touch_val);
    Serial.print("touch_val = ");
    Serial.println(szText);
    
    // ignore low touch_val
    if (touch_val != 0) {
      for (int i = 0; i < 20; i++) {
        if ((touch_val & ((uint32_t)1 << i)) != 0) break;
        touch_val |= (uint32_t)1 << i;
      }
    }

    sprintf(szText, "%03x", touch_val);
    Serial.print("touch_val = ");
    Serial.println(szText);
    
    if (szDebug) {
      for (int i = 0; i < 20; i++) {
        if ((touch_val & ((uint32_t)1 << i)) != 0) {
          szDebug[i] = '*';
        } else {
          szDebug[i] = '.';
        }
      }
    }
    
    while (touch_val & 0x01)
    {
      trig_section++;
      touch_val >>= 1;
    }
    int level = trig_section * 5;
    Serial.print("water level = ");
    Serial.print(level);
    Serial.println("% ");
    Serial.println(" ");
    Serial.println("*********************************************************");
//    delay(1000);
//  }
    return level;
}

void initLevelCapacitive10() {
}

#endif // ENABLE_LEVELCAPACITIVE10


//================================================================================
// BUZZER

#ifdef ENABLE_BUZZER

#define PIN_BUZZER 8 // D8

void turnOnBuzzer()
{
  digitalWrite(PIN_BUZZER, HIGH); // turn ON
}

void turnOffBuzzer()
{
  digitalWrite(PIN_BUZZER, LOW); // turn OFF
}

void initBuzzer() {
  pinMode(PIN_BUZZER, OUTPUT);
}

#endif


//================================================================================
// RELAY

#ifdef ENABLE_RELAY

bool relayClosed = false;

void initRelay() {
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(4, OUTPUT);
  relayClosed = false;
}  

void openRelay() {
  digitalWrite(4, LOW);
  relayClosed = false;
}

void closeRelay() {
  digitalWrite(4, HIGH);
  relayClosed = true;
}

bool isRelayClosed() {
  //return digitalRead(2) == HIGH;
  return relayClosed;
}

#endif // ENABLE_RELAY

//================================================================================
// TEMPERATURE HUMIDITY DHT20

#ifdef ENABLE_DHT

#include "DHT.h"

DHT dht(DHT20);

void initDHT() {
  dht.begin();
}

void readDHT(float* temp_hum_val) {
  if (dht.readTempAndHumidity(temp_hum_val)) {
    temp_hum_val[0] = 0.0f;
    temp_hum_val[1] = 0.0f;
  }
}

#endif // ENABLE_DHT

//================================================================================
// DISPLAY

#ifdef ENABLE_DISPLAY

#include "rgb_lcd.h"

rgb_lcd lcd;

void initDisplay() {
  lcd.begin(16, 2);

  lcd.setRGB(0, 0, 0);
  lcd.print("Initialize");
}

void redBacklightDisplay() {
  lcd.setRGB(100, 0, 0);
}

void greenBacklightDisplay() {
  lcd.setRGB(0, 100, 0);
}

void noBacklightDisplay() {
  lcd.setRGB(0, 0, 0);
}

void printDisplay(const char *text, const char *text2) {
  lcd.clear();
  lcd.print(text);
  lcd.setCursor(0,1);
  lcd.print(text2);
}

#endif // ENABLE_DISPLAY

//================================================================================
// MAIN

unsigned long lastRelayStart = 0;
unsigned long lastRelayStop = 0;
unsigned long lastBuzzerStart = 0;

void setup() {
  #ifdef ENABLE_LOGGING
  Serial.begin(115200);
  #endif
  
  Wire.begin();

  #if defined(ENABLE_LEVELCAPACITIVE10)
  initLevelCapacitive10();
  #elif defined(ENABLE_LEVELULTRASONIC)
  initLevelUltrasonic();
  #endif
  
  #ifdef ENABLE_BUZZER
  initBuzzer();
  #endif
  #ifdef ENABLE_RELAY
  initRelay();
  #endif
  #ifdef ENABLE_DHT
  initDHT();
  #endif
  #ifdef ENABLE_DISPLAY
  initDisplay();
  #endif

  #ifdef ENABLE_DISPLAY
  {
    greenBacklightDisplay();
    char szText[20];
    sprintf(szText, "Version: %s", __DATE__);
    printDisplay("Arduino Waterpump", szText);
    delay(4000);
    sprintf(szText, "%dmm - %dmm", THRESHOLD_LOW, THRESHOLD_HIGH);
    printDisplay("Using thresholds:", szText);
    delay(2000);
    sprintf(szText, "%lus - %lus - %lus", PUMPINTERVAL_MIN, PUMPINTERVAL_MAX, WAITINTERVAL);
    printDisplay("Using timeouts:", szText);
    delay(2000);
  }
  #endif

  closeRelay();
  #ifdef ENABLE_DISPLAY
  redBacklightDisplay();
  for (int i = INITIAL_PUMP_ON; i > 0; i--) {
    char szText[12];
    sprintf(szText, "%ds", i);
    printDisplay("Turn on pump...", szText);
    delay(1000);
  }
  noBacklightDisplay();
  #else
  delay(INITIAL_PUMP_ON * 1000);
  #endif
  openRelay();

  unsigned long now = millis();
  lastRelayStart = now;
  lastRelayStop = now;
}

void HandleRelayLogic(int level)
{
  unsigned long now = millis();
  if (now < lastRelayStart || now < lastRelayStop || now < lastBuzzerStart) {
    // Overflow (every 50 days)
    lastRelayStart = 0;
    lastRelayStop = 0;
    lastBuzzerStart = 0;
  }

  #define FORCE_OFF 0
  #define OFF 1
  #define DONT_CARE 2
  #define ON 3
  #define FORCE_ON 4
  
  // Steering from Level
  //#define THRESHOLD_LOW
  //#define THRESHOLD_HIGH
  int requestFromLevel;  // 0 = OFF, 1 = DONTCARE, 2 = ON
  if (level == -1) {
    requestFromLevel = DONT_CARE;
  } else if (level <= THRESHOLD_LOW) {
    requestFromLevel = OFF;
  } else if (level < THRESHOLD_HIGH) {
    requestFromLevel = DONT_CARE;
  } else { // if (level >= THRESHOLD_HIGH)
    requestFromLevel = ON;
  }

  // Steering from Timeouts
  //#define PUMPINTERVAL_MIN 30ul
  //#define PUMPINTERVAL_MAX 45ul
  //#define WAITINTERVAL 120ul -- Maximum pump interval if DONTCARE
  int requestFromTimeout;
  if (now - lastRelayStart < PUMPINTERVAL_MIN * 1000) {
    requestFromTimeout = FORCE_ON;
  } else if (lastRelayStop < lastRelayStart && now - lastRelayStart >= PUMPINTERVAL_MAX * 1000) {
    requestFromTimeout = FORCE_OFF;
  } else if (now - lastRelayStop >= WAITINTERVAL * 1000) {
    requestFromTimeout = FORCE_ON;
  } else {
    requestFromTimeout = DONT_CARE;
  }

  int request;
  if (requestFromLevel == FORCE_ON || requestFromTimeout == FORCE_ON) {
    request = ON;
  } else if (requestFromLevel == FORCE_OFF || requestFromTimeout == FORCE_OFF) {
    request = OFF;
  } else if (requestFromLevel == ON || requestFromTimeout == ON) {
    request = ON;
  } else if (requestFromLevel == OFF || requestFromTimeout == OFF) {
    request = OFF;
  } else {
    request = DONT_CARE;
  }

  #ifdef ENABLE_LOGGING
  char szDebug[80] = {0};
  sprintf(szDebug, "Request: Level=%d, Timeout=%d --> %d", requestFromLevel, requestFromTimeout, request);
  Serial.println(szDebug);
  #endif

  if (request == DONT_CARE) {
    if (isRelayClosed()) {
      // Histeresis --> GREEN
      #ifdef ENABLE_DISPLAY
      greenBacklightDisplay();
      #endif
    } else {
      #ifdef ENABLE_DISPLAY
      noBacklightDisplay();
      #endif
    }
  } else if (request == ON) {
    #ifdef ENABLE_LOGGING
    Serial.println("==> CLOSE RELAY");
    #endif
    if (!isRelayClosed()) {
      lastRelayStart = millis();
    }
    closeRelay();
    #ifdef ENABLE_DISPLAY
    redBacklightDisplay();
    #endif
  } else if (request == OFF) {
    #ifdef ENABLE_LOGGING
    Serial.println("==> OPEN RELAY (LEVEL)");
    #endif
    if (isRelayClosed()) {
      lastRelayStop = millis();
    }
    openRelay();
    #ifdef ENABLE_DISPLAY
    noBacklightDisplay();
    #endif
  }
}


void loop()
{
  char szLine2[20] = {0};

  #if defined(ENABLE_LEVELCAPACITIVE10)
  int level = getLevelCapacitive10(szLine2);
  #elif defined(ENABLE_LEVELULTRASONIC)
  int level = getLevelUltrasonic(szLine2);
  #else
  int level = -1;
  #endif

  #ifdef ENABLE_LOGGING
  Serial.print("water level = ");
  Serial.print(level);
  Serial.println("% ");
  #endif

  // Check alarm
  #ifdef ENABLE_BUZZER
  // TODO TEST, ADD HISTERESIS, TURN OFF PERIODICALLY
  unsigned long now = millis();
  if (level < THRESHOLD_ALARM)
  {
    Serial.println("Buzzer: Turning OFF (no alarm)");
    lastBuzzerStart = 0;
    turnOffBuzzer();
  }
  else  if (lastBuzzerStart == 0)
  {
    Serial.println("Buzzer: Turning ON (alarm)");    
    lastBuzzerStart = now;
    turnOnBuzzer();
  }
  else if ((now - lastBuzzerStart) % 10000 >= 5000)
  {
    Serial.println("Buzzer: Turning OFF (after timeout)");
    turnOffBuzzer();
  }
  else
  {
    Serial.println("Buzzer: Turning ON (after timeout )");
    turnOnBuzzer();
  }
  #endif

  HandleRelayLogic(level);
  
  float temp_hum_val[2] = {0};
  #ifdef ENABLE_DHT
  readDHT(temp_hum_val);
  #endif

  #ifdef ENABLE_DISPLAY
  char szTemp[6];
  dtostrf(temp_hum_val[1], 4, 1, szTemp);
  char szHum[6];
  dtostrf(temp_hum_val[0], 4, 1, szHum);

  // Print a message to the LCD.
  char line[20];
  sprintf(line, "%dmm  %sC %s%%", level, szTemp, szHum);
  printDisplay(line, szLine2);
  #endif
    
  delay(1000);
}
