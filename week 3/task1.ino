const int potPin = A0; // Analog input pin connected to the potentiometer
const int ledPin = 8; // Digital output pin connected to the LED with a 220 ohm resistor

void setup() {
Serial.begin(9600); // Initialize serial communication at 9600 baud rate
pinMode(ledPin, OUTPUT); // Set the LED pin as an output
}

void loop() {
// Read the potentiometer value (range: 0 to 1023) and send it over the serial port
int potValue = analogRead(potPin);
Serial.println(potValue);

// Check if there is an incoming command from the Python script
if (Serial.available() > 0) {
char command = Serial.read();
if (command == '1') {
digitalWrite(ledPin, HIGH); // Turn LED ON
} else if (command == '0') {
digitalWrite(ledPin, LOW); // Turn LED OFF
}
}

delay(200); // Add a short delay to avoid sending data too fast
