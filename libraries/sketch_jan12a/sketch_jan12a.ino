#include 

void setup() {
  Serial.begin(9600);
  
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);

  a = 5;
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  a++;
  Serial.print("a=");
  Serial.println(a);
}
