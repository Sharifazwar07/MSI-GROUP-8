#include <Wire.h>
#include <MPU6050.h>
#include <Servo.h>

MPU6050 mpu;
Servo servo;
int servoPosition = 90; // Locked position

void setup() {
  Serial.begin(9600);

  // 1. Initialize Servo and LEDs first
  servo.attach(9);
  servo.write(servoPosition);
  pinMode(4, OUTPUT); // Green LED
  pinMode(3, OUTPUT); // Red LED

  // Default State: Locked (Red LED ON)
  digitalWrite(4, LOW);
  digitalWrite(3, HIGH);

  // 2. Initialize MPU6050
  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (1);
  }
}

void loop() {
  // 1. Check for 'A' (Access Granted) or 'D' (Access Denied) from Python
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'A') {
      servo.write(180);      // Unlock
      digitalWrite(4, HIGH); // Green LED ON
      digitalWrite(3, LOW);  // Red LED OFF
    } else if (command == 'D') {
      servo.write(90);       // Lock
      digitalWrite(4, LOW);  // Green LED OFF
      digitalWrite(3, HIGH); // Red LED ON
    }
  }

  // 2. Send MPU6050 accelerometer data to Python
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  Serial.print(ax); Serial.print(",");
  Serial.print(ay); Serial.print(",");
  Serial.println(az);

  delay(100);
}