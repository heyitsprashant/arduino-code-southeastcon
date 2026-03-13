#include <LiquidCrystal.h>

// LCD pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Motor pins
#define MOTOR_EN  3
#define MOTOR_IN1 6
#define MOTOR_IN2 13

// LED pins
#define GREEN_LED 4
#define RED_LED   5

// Encoder pin
#define ENCODER_PIN 2

// RPM calculation
volatile int pulseCount = 0;
float rpm = 0;
unsigned long lastTime = 0;
bool faultCleared = false;
unsigned long rampStartTime = 0;

void countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);
  
  // LCD setup
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Firebird-1");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");
  delay(1000);
  
  // Motor pins
  pinMode(MOTOR_EN,  OUTPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  
  // LED pins
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED,   OUTPUT);
  
  // Turn off both LEDs
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED,   LOW);
  
  // Encoder
  pinMode(ENCODER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countPulse, RISING);

  // Set motor direction
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  
  // 3 second countdown on LCD
  for (int i = 3; i > 0; i--) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Launch in:");
    lcd.setCursor(0, 1);
    lcd.print(i);
    lcd.print(" seconds...");
    Serial.print("Countdown: ");
    Serial.println(i);
    delay(1000);
  }
  
  // Show ramping message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ramping up...");
  lcd.setCursor(0, 1);
  lcd.print("Please wait...");
  Serial.println("Ramping up motor...");
  
  // Slow ramp up over 2 seconds
  for (int speed = 0; speed <= 255; speed += 5) {
    analogWrite(MOTOR_EN, speed);
    delay(40);
  }
  
  rampStartTime = millis();
  lastTime = millis();
}

void loop() {
  if (millis() - lastTime >= 1000) {
    
    // Stop counting while we calculate
    detachInterrupt(digitalPinToInterrupt(ENCODER_PIN));
    
    // Calculate RPM
    rpm = (pulseCount / 20.0) * 60.0;
    pulseCount = 0;
    lastTime = millis();
    
    // Print to serial monitor
    Serial.print("RPM: ");
    Serial.println(rpm);
    
    // Show RPM on LCD line 1
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RPM: ");
    lcd.print(rpm);
    
    // Check speed and set LEDs
    if (rpm >= 100) {
      // Motor reached good speed
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      lcd.setCursor(0, 1);
      lcd.print("Status: GOOD!");
      Serial.println("Status: GOOD!");
      
    } else if (rpm < 100 && millis() - rampStartTime <= 7000) {
      // Motor still trying - show red fault
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);
      lcd.setCursor(0, 1);
      lcd.print("FAULT:Low Speed!");
      Serial.println("FAULT: Low Speed!");
      
    } else if (rpm < 100 && millis() - rampStartTime > 7000) {
      // Motor failed after 7 seconds total
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);
      analogWrite(MOTOR_EN, 0);
      lcd.setCursor(0, 1);
      lcd.print("MOTOR FAILED!");
      Serial.println("MOTOR FAILED!");
    }
    
    // Restart counting
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countPulse, RISING);
  }
}