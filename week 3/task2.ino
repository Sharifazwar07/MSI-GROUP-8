#include <Servo.h>
Servo myservo;
const int potPin = A0;   // Potentiometer pin
const int ledPin = 7;    // LED pin
const int servoPin = 9;  // Servo pin
bool isRunning = true;
void setup() {
  Serial.begin(9600);
  myservo.attach(servoPin);
  pinMode(ledPin, OUTPUT);
}
void loop() {
  // Stop if 'q' command is received from the computer
  if (Serial.available() > 0) {
    char key = Serial.read();
    if (key == 'q' || key == 'Q') {
      isRunning = false;
      myservo.write(90);          // Stop 360 servo
      digitalWrite(ledPin, LOW);  // Turn off LED
    }
  }
  if (isRunning) {
    int potValue = analogRead(potPin);              // Read potentiometer (0 - 1023)
    int servoPos = map(potValue, 0, 1023, 0, 180); // Convert to 0 - 180 scale  
    myservo.write(servoPos);                       // Move servo
    // Turn on LED if potentiometer is greater than half (> 512)
    if (potValue > 512) {
      digitalWrite(ledPin, HIGH);
    } else {
      digitalWrite(ledPin, LOW);
    }
    Serial.println(potValue); // Send value to Python
    delay(100);
  }
