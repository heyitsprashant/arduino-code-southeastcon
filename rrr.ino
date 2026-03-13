// These define which Arduino pins connect to what
#define MOTOR_EN  9    // Controls motor SPEED (PWM)
#define MOTOR_IN1 10   // Controls motor DIRECTION
#define MOTOR_IN2 11   // Controls motor DIRECTION
#define ENCODER_PIN 2  // Reads pulses from IR sensor
#define GREEN_LED 4    // Green LED pin
#define RED_LED   5    // Red LED pin

// pulseCount increases every time disc slot passes sensor
// volatile means it can change inside interrupt at any time
volatile int pulseCount = 0;
float rpm = 0;
unsigned long lastTime = 0;

// This function runs INSTANTLY every time sensor detects a slot
void countPulse() {
  pulseCount++;  // Add 1 every time a slot passes
}

void setup() {
  Serial.begin(9600);  // Start communication with laptop
  
  // Tell Arduino these pins are outputs (sending signal)
  pinMode(MOTOR_EN,  OUTPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED,   OUTPUT);
  
  // Tell Arduino pin 2 is input (receiving signal from sensor)
  pinMode(ENCODER_PIN, INPUT_PULLUP);
  
  // Attach interrupt — means: every time pin 2 CHANGES
  // instantly run countPulse() function
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countPulse, CHANGE);
  
  // 3 second countdown before motor starts
  Serial.println("Waiting 3 seconds...");
  digitalWrite(RED_LED, HIGH);  // Red LED on during wait
  delay(1000);                  // Wait 1 second
  Serial.println("2...");
  delay(1000);                  // Wait 1 second
  Serial.println("1...");
  delay(1000);                  // Wait 1 second
  digitalWrite(RED_LED, LOW);   // Red LED off
  
  // Start motor — IN1 HIGH and IN2 LOW = forward direction
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  analogWrite(MOTOR_EN, 255);   // 255 = full speed
  
  Serial.println("Motor Started!");
  digitalWrite(GREEN_LED, HIGH); // Green LED on = success
  
  lastTime = millis();  // Record current time
}

void loop() {
  // Every 1000 milliseconds (1 second) calculate RPM
  if (millis() - lastTime >= 1000) {
    
    // Stop interrupts briefly so count is accurate
    noInterrupts();
    int count = pulseCount;  // Save the count
    pulseCount = 0;          // Reset count for next second
    interrupts();            // Resume interrupts
    
    // RPM formula:
    // count = pulses in 1 second
    // divide by 40 (20 slots x 2 edges)
    // multiply by 60 (convert to per minute)
    rpm = (count / 40.0) * 60.0;
    lastTime = millis();
    
    Serial.print("RPM: ");
    Serial.println(rpm);
    
    // Green LED if motor running, Red LED if stopped
    if(rpm > 0) {
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
    } else {
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, HIGH);
    }
  }
}