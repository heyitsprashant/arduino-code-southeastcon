// ============================================================
// this file contains the first tier points where we can grab 65 to 85 points
// ============================================================

// ── SECTION 1: LIBRARIES ────────────────────────────────────
#include "MeUltrasonicSensor.h"
#include "MeLineFollower.h"
#include "MeEncoderMotor.h"
#include "MeServo.h"
#include "Arduino.h"

// ── SECTION 2: HARDWARE DECLARATIONS ────────────────────────
MeUltrasonicSensor ultrasonic(PORT_6);
MeLineFollower     lineSensor(PORT_7);
MeEncoderMotor     motorL(0x09, SLOT1);
MeEncoderMotor     motorR(0x09, SLOT2);
MeServo            flagServo;

// ── SECTION 3: TUNING CONSTANTS ─────────────────────────────
#define LIGHT_THRESHOLD   800
#define DRIVE_SPEED       60
#define RETURN_SPEED      50
#define LEAVE_TIME        1500
#define HOME_DISTANCE     10
#define FLAG_SERVO_OPEN   90
#define FLAG_SERVO_CLOSED 0
#define SERVO_PIN         A0

// ── SECTION 4: MOVEMENT HELPERS ─────────────────────────────
// Small reusable functions — driveForward, driveBackward, stopMotors
// These get called by every task below

void driveForward(int spd) {
  motorL.run(spd);
  motorR.run(-spd);
}

void driveBackward(int spd) {
  motorL.run(-spd);
  motorR.run(spd);
}

void turnLeft(int spd) {
  motorL.run(-spd);
  motorR.run(-spd);
}

void turnRight(int spd) {
  motorL.run(spd);
  motorR.run(spd);
}

void stopMotors() {
  motorL.run(0);
  motorR.run(0);
}

// ── SECTION 5: TASK FUNCTIONS ────────────────────────────────
// One function per competition task
// Easy to enable/disable individual tasks during testing

void waitForStart() {
  Serial.println("Waiting for LED start bar...");
  bool started = false;
  while (!started) {
    if (lineSensor.readSensors() > LIGHT_THRESHOLD) {
      started = true;
      Serial.println("START DETECTED!");
    }
    delay(50);
  }
  delay(200);
}

void leaveStartZone() {
  Serial.println("Leaving start zone...");
  driveForward(DRIVE_SPEED);
  delay(LEAVE_TIME);
  stopMotors();
  delay(300);
}

void plantFlag() {
  Serial.println("Planting flag...");
  flagServo.write(FLAG_SERVO_OPEN);
  delay(600);
}

void rescueDucks() {
  // ── ADD DUCK RESCUE CODE HERE ──
  // Push ducks toward the blue Lunar Landing Area
  Serial.println("Rescuing ducks...");
}

void doAntennaOne() {
  // ── ADD ANTENNA #1 CODE HERE ──
  // Navigate to Antenna #1, press button 3 times
  Serial.println("Antenna 1 task...");
}

void doAntennaTwo() {
  // ── ADD ANTENNA #2 CODE HERE ──
  // Navigate to Antenna #2, rotate crank 540 degrees
  Serial.println("Antenna 2 task...");
}

void doAntennaThree() {
  // ── ADD ANTENNA #3 CODE HERE ──
  // Navigate to crater, lift duck off pressure plate
  Serial.println("Antenna 3 task...");
}

void doAntennaFour() {
  // ── ADD ANTENNA #4 CODE HERE ──
  // Navigate to Antenna #4, input keypad code 73738#
  Serial.println("Antenna 4 task...");
}

void returnHome() {
  Serial.println("Returning home...");
  driveBackward(RETURN_SPEED);
  while (ultrasonic.distanceCm() > HOME_DISTANCE) {
    delay(50);
  }
  stopMotors();
  Serial.println("Match complete!");
}

// ── SECTION 6: SETUP ─────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  flagServo.attach(SERVO_PIN);
  flagServo.write(FLAG_SERVO_CLOSED);
  motorL.begin();
  motorR.begin();
  delay(500);
  Serial.println("System ready.");
}

// ── SECTION 7: MAIN RUN SEQUENCE ─────────────────────────────
// This is your match script — comment out tasks you haven't
// built yet, uncomment them as you add them

void loop() {
  waitForStart();       // +15 pts — auto start
  leaveStartZone();     // +10 pts — leave start zone
  plantFlag();          // +10 pts — plant JSU flag

  // -- comment out tasks below until you've coded them --
  // rescueDucks();     // +30 pts max (5 per duck)
  // doAntennaOne();    // +15 pts
  // doAntennaTwo();    // +15 pts
  // doAntennaThree();  // +15 pts
  // doAntennaFour();   // +15 pts

  returnHome();         // +15 pts — return to start

  while (true) { delay(1000); }  // halt after run
}