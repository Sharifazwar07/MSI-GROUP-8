// MCTA3203 Week 5: Step 4 Only (Observation Table Runner)
// Cycles through PWM 255, 150, 75 in Forward and Reverse automatically

// Direction Pins
const int DIR1 = 12; 
const int DIR2 = 13; 

// All possible PWM Speed Pins across L298P shield variants
const int PWM_A1 = 3;   // Lab sheet ENA
const int PWM_B1 = 5;   // Lab sheet ENB
const int PWM_A2 = 10;  // Keyes/FunduMoto ENA (E1)
const int PWM_B2 = 11;  // Keyes/FunduMoto ENB (E2)

// PWM values from the Observation Table
const int tablePWM[3] = {255, 150, 75};

void setup() {
  Serial.begin(9600);

  pinMode(DIR1, OUTPUT);
  pinMode(DIR2, OUTPUT);
  pinMode(PWM_A1, OUTPUT);
  pinMode(PWM_B1, OUTPUT);
  pinMode(PWM_A2, OUTPUT);
  pinMode(PWM_B2, OUTPUT);

  Serial.println("=== Starting Step 4: Observation Table Loop ===");
}

// Helper function to set direction and PWM speed
void setMotors(bool forward, int speed) {
  if (forward) {
    digitalWrite(DIR1, HIGH);
    digitalWrite(DIR2, LOW);
  } else {
    digitalWrite(DIR1, LOW);
    digitalWrite(DIR2, HIGH);
  }

  analogWrite(PWM_A1, speed); // D3
  analogWrite(PWM_B1, speed); // D5
  analogWrite(PWM_A2, speed); // D10
  analogWrite(PWM_B2, speed); // D11
}

void loop() {
  // 1. FORWARD ROWS (PWM 255, 150, 75)
  for (int i = 0; i < 3; i++) {
    Serial.print("Direction: FORWARD | PWM: ");
    Serial.print(tablePWM[i]);
    Serial.println(" (Hold for 8s - Measure Voltage & RPM now)");
    
    setMotors(true, tablePWM[i]);
    delay(8000); // 8 seconds to take your multimeter & RPM reading
  }

  // Brief 2-second stop before reversing direction
  Serial.println("--- Stopping briefly before Reverse ---");
  setMotors(true, 0);
  delay(2000);

  // 2. REVERSE ROWS (PWM 255, 150, 75)
  for (int i = 0; i < 3; i++) {
    Serial.print("Direction: REVERSE | PWM: ");
    Serial.print(tablePWM[i]);
    Serial.println(" (Hold for 8s - Measure Voltage & RPM now)");
    
    setMotors(false, tablePWM[i]);
    delay(8000); // 8 seconds to take your multimeter & RPM reading
  }

  // 4-second pause before repeating the entire table from the top
  Serial.println("=== Cycle Complete! Restarting in 4 seconds... ===\n");
  setMotors(true, 0);
  delay(4000);
}
