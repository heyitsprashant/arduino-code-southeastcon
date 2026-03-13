// Milestone 1 - Motor Control + RPM Display
#include <Wire.h>

// Motor pins
#define MOTOR_EN  9
#define MOTOR_IN1 10
#define MOTOR_IN2 11

// Encoder pin
#define ENCODER_PIN 2

// LED pins
#define GREEN_LED 4
#define RED_LED   5

// RPM calculation
volatile int pulseCount = 0;
float rpm = 0;
unsigned long lastTime = 0;

void countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);
  
  // Motor pins
  pinMode(MOTOR_EN,  OUTPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  
  // LED pins
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED,   OUTPUT);
  
  // Encoder interrupt
  pinMode(ENCODER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countPulse, RISING);
  
  // Wait 3 seconds before starting
  Serial.println("Waiting 3 seconds...");
  digitalWrite(RED_LED, HIGH);
  delay(3000);
  digitalWrite(RED_LED, LOW);
  
  // Start motor at full speed
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  analogWrite(MOTOR_EN, 255);
  
  Serial.println("Motor Started!");
  digitalWrite(GREEN_LED, HIGH);
  
  lastTime = millis();
}

void loop() {
  // Calculate RPM every second
  if (millis() - lastTime >= 1000) {
    detachInterrupt(digitalPinToInterrupt(ENCODER_PIN));
    
    rpm = (pulseCount / 20.0) * 60.0;
    pulseCount = 0;
    lastTime = millis();
    
    Serial.print("RPM: ");
    Serial.println(rpm);
    
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countPulse, RISING);
  }
}