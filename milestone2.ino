#include <LiquidCrystal.h>

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

#define MOTOR_EN  3
#define MOTOR_IN1 6
#define MOTOR_IN2 13
#define GREEN_LED 4
#define RED_LED   5
#define ENCODER_PIN 2

volatile int pulseCount = 0;
float rpm = 0;
float maxRPM = 0;
float targetRPM = 0;
unsigned long lastTime = 0;
unsigned long rampStartTime = 0;
bool rampDone = false;
bool faultActive = false;
bool faultCleared = false;
unsigned long faultStartTime = 0;

void countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Firebird-1");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");
  delay(1000);

  pinMode(MOTOR_EN,  OUTPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED,   OUTPUT);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED,   LOW);

  pinMode(ENCODER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countPulse, RISING);

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
  lcd.print("Speed: 0");
  Serial.println("Ramping up motor...");

  // Slow ramp up over 2 seconds
  // Show RPM during ramp on LCD
  for (int speed = 0; speed <= 255; speed += 5) {
    analogWrite(MOTOR_EN, speed);

    // Calculate RPM during ramp
    detachInterrupt(digitalPinToInterrupt(ENCODER_PIN));
    float rampRPM = (pulseCount / 20.0) * 60.0;
    pulseCount = 0;
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countPulse, RISING);

    // Show on LCD
    lcd.setCursor(0, 1);
    lcd.print("Speed:");
    lcd.print(rampRPM);
    lcd.print("    ");

    if (rampRPM > maxRPM) maxRPM = rampRPM;

    delay(40);
  }

  // Target is 75% of max RPM seen
  targetRPM = maxRPM * 0.75;

  Serial.print("Max RPM during ramp: ");
  Serial.println(maxRPM);
  Serial.print("Target 75% RPM: ");
  Serial.println(targetRPM);

  rampStartTime = millis();
  lastTime = millis();
}

void loop() {
  if (millis() - lastTime >= 1000) {

    detachInterrupt(digitalPinToInterrupt(ENCODER_PIN));
    rpm = (pulseCount / 20.0) * 60.0;
    pulseCount = 0;
    lastTime = millis();

    Serial.print("RPM: ");
    Serial.println(rpm);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RPM:");
    lcd.print(rpm);

    unsigned long elapsed = millis() - rampStartTime;

    if (rpm >= targetRPM && targetRPM > 0) {
      // SUCCESS
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      faultActive = false;
      lcd.setCursor(0, 1);
      lcd.print("Status: GOOD!");
      Serial.println("Status: GOOD!");

    } else if (elapsed <= 2000) {
      // Still in ramp window
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
      lcd.setCursor(0, 1);
      lcd.print("Checking...");

    } else if (elapsed > 2000 && elapsed <= 7000) {
      // Fault window - trying for 5 more seconds
      if (!faultActive) {
        faultActive = true;
        faultStartTime = millis();
      }
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);
      lcd.setCursor(0, 1);
      lcd.print("FAULT:Low Speed!");
      Serial.println("FAULT: Low Speed!");

    } else if (elapsed > 7000) {
      // Failed completely
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);
      lcd.setCursor(0, 1);
      lcd.print("MOTOR FAILED!");
      Serial.println("MOTOR FAILED!");
    }

    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countPulse, RISING);
  }
}