// C++ code
//
int i = 0;

void setup()
{
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(2, INPUT);

  digitalWrite(5, HIGH);
  digitalWrite(6, LOW);
  digitalWrite(6, LOW);
  digitalWrite(3, HIGH);
  digitalWrite(4, LOW);
}

void loop()
{
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, HIGH);
  //delay(3000); // Wait for 3000 millisecond(s)
  while (digitalRead(2) == LOW) {
  }
  digitalWrite(5, LOW);
  digitalWrite(6, HIGH);
  digitalWrite(7, LOW);
  delay(1000); // Wait for 1000 millisecond(s)
  digitalWrite(5, HIGH);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  delay(2000); // Wait for 1000 millisecond(s)
  digitalWrite(4, HIGH);
  digitalWrite(3, LOW);
  delay(5000); // Wait for 3000 millisecond(s)
  digitalWrite(3, HIGH);
  digitalWrite(4, LOW);
  delay(3000); // Wait for 1000 millisecond(s)
}
