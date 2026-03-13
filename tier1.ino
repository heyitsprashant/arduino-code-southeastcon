// ============================================================
//  IEEE SoutheastCon 2026 — TIER 1 COMPLETE PROGRAM
//  Board   : mBot Ultimate 2.0 (mCore2 / ATmega2560)
//  Libs    : MeUltrasonicSensor, MeLineFollower, MeEncoderOnBoard
//  Sensors : Line Follower Port 7, Ultrasonic Port 6
//  NOTE    : No servo/gripper — flag is released mechanically
//            (rubber band, gravity drop, or passive release)
// ============================================================

// ── LIBRARIES ───────────────────────────────────────────────
#include <MeUltrasonicSensor.h>
#include <MeLineFollower.h>
#include <MeEncoderOnBoard.h>
#include <Arduino.h>

// ── HARDWARE ────────────────────────────────────────────────
MeUltrasonicSensor ultrasonic(PORT_6);
MeLineFollower     lineSensor(PORT_7);
MeEncoderOnBoard   motorL(SLOT1);
MeEncoderOnBoard   motorR(SLOT2);

// ── TUNING CONSTANTS ────────────────────────────────────────
#define LIGHT_THRESHOLD   800    // white LED bar detection value
                                 // CALIBRATE: Serial Monitor → note room
                                 // light reading, flash white LED above
                                 // sensor, set between the two values

#define DRIVE_SPEED       200    // forward speed (-255 to 255)
#define RETURN_SPEED      150    // slower speed for return journey

#define FLAG_DRIVE_TIME   1700   // ms to drive ~20 inches (508mm)
                                 // CALIBRATE: measure actual distance,
                                 // adjust until robot travels 508mm

#define EXTRA_DRIVE_TIME  500    // ms extra forward after flag drop
                                 // clears space before 180 turn

#define HOME_DISTANCE     10     // cm — stop when this close to start wall

#define TURN_180_MS       1100   // ms to spin 180 degrees
                                 // CALIBRATE: time a full 360 spin,
                                 // use exactly half that value here

// ── MOVEMENT HELPERS ────────────────────────────────────────

void driveForward(int spd) {
  motorL.setMotorPwm(spd);
  motorR.setMotorPwm(-spd);    // mirrored mount — sign flipped
                                // if robot spins → swap sign on motorR
}

void driveBackward(int spd) {
  motorL.setMotorPwm(-spd);
  motorR.setMotorPwm(spd);
}

void spinRight(int spd) {
  motorL.setMotorPwm(spd);     // both same direction = pivot right
  motorR.setMotorPwm(spd);
}

void stopMotors() {
  motorL.setMotorPwm(0);
  motorR.setMotorPwm(0);
}

// ── TASK FUNCTIONS ───────────────────────────────────────────

// ----------------------------------------------------------
// TASK 1: Wait for white LED start bar  (+15 pts)
// ----------------------------------------------------------
void waitForStart() {
  Serial.println("STATUS: Waiting for LED start bar...");
  bool started = false;

  while (!started) {
    int val = lineSensor.readSensors();
    Serial.print("Light: "); Serial.println(val);

    if (val > LIGHT_THRESHOLD) {
      started = true;
      Serial.println("STATUS: START DETECTED!");
    }
    delay(50);
  }
  delay(200);
}

// ----------------------------------------------------------
// TASK 2: Leave starting area  (+10 pts)
// ----------------------------------------------------------
void leaveStartZone() {
  Serial.println("STATUS: Leaving start zone...");
  driveForward(DRIVE_SPEED);
  delay(FLAG_DRIVE_TIME);
  stopMotors();
  Serial.println("STATUS: 20 inches reached.");
  delay(300);
}

// ----------------------------------------------------------
// TASK 3: Plant school flag  (+10 pts)
// ----------------------------------------------------------
// No servo — flag is passively released at this position.
// Robot has already driven 20 inches out of start zone.
// Flag drops/releases by whatever passive mechanism is used
// (gravity tilt, rubber band trigger, wedge release etc).
// If your release needs a digital pin trigger, add it here.
void plantFlag() {
  Serial.println("STATUS: Planting JSU flag...");

  // ── ADD YOUR FLAG RELEASE MECHANISM HERE ──────────────
  // Example — digital pin trigger (uncomment if needed):
  // pinMode(A0, OUTPUT);
  // digitalWrite(A0, HIGH);   // trigger release
  // delay(300);
  // digitalWrite(A0, LOW);
  // ──────────────────────────────────────────────────────

  // pause so flag has time to settle on the surface
  delay(600);
  Serial.println("STATUS: Flag planted.");
}

// ----------------------------------------------------------
// TASK 4: Return to starting area  (+15 pts)
// ----------------------------------------------------------
void returnHome() {
  Serial.println("STATUS: Returning home...");

  // drive a little further so we don't reverse over the flag
  driveForward(DRIVE_SPEED);
  delay(EXTRA_DRIVE_TIME);
  stopMotors();
  delay(300);

  // spin 180 degrees to face the start zone
  Serial.println("STATUS: Turning 180...");
  spinRight(DRIVE_SPEED);
  delay(TURN_180_MS);
  stopMotors();
  delay(300);

  // drive toward start wall until ultrasonic detects it
  Serial.println("STATUS: Driving toward start wall...");
  driveForward(RETURN_SPEED);

  while (ultrasonic.distanceCm() > HOME_DISTANCE) {
    Serial.print("Distance: "); Serial.print(ultrasonic.distanceCm());
    Serial.println(" cm");
    delay(50);
  }

  stopMotors();
  Serial.println("STATUS: HOME — inside start zone!");
}

// ── SETUP ────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  Serial.println("STATUS: Initialising...");

  motorL.begin();
  motorR.begin();
  delay(500);

  Serial.println("STATUS: Ready.");
}

// ── MAIN RUN SEQUENCE ────────────────────────────────────────
void loop() {

  waitForStart();     // +15 pts
  leaveStartZone();   // +10 pts
  plantFlag();        // +10 pts
  returnHome();       // +15 pts

  Serial.println("==============================================");
  Serial.println("MATCH COMPLETE");
  Serial.println("  Auto-start      : 15 pts");
  Serial.println("  Left start zone : 10 pts");
  Serial.println("  Planted flag    : 10 pts");
  Serial.println("  Returned home   : 15 pts");
  Serial.println("  TOTAL           : 50 pts");
  Serial.println("  + Design comp   : 15 pts");
  Serial.println("  GRAND TOTAL     : 65 pts");
  Serial.println("==============================================");

  while (true) { delay(1000); }
}

// ============================================================
//  CALIBRATION ORDER
// ============================================================
//
//  1. LIGHT_THRESHOLD
//     Serial Monitor → watch "Light:" under room light
//     Flash white LED above sensor → note spike
//     Set LIGHT_THRESHOLD between the two values
//
//  2. FLAG_DRIVE_TIME
//     Tape measure on floor → run leaveStartZone() alone
//     Target: 508mm (20 inches)
//     Adjust FLAG_DRIVE_TIME until distance matches
//
//  3. TURN_180_MS
//     Mark robot center on floor
//     Run spinRight() for TURN_180_MS → measure angle
//     Adjust until exactly 180 degrees
//
//  4. HOME_DISTANCE
//     Run returnHome() alone → watch Serial Monitor
//     Confirm robot stops cleanly inside 12 inch zone
//
//  5. Full dry run x3
//
// ============================================================
//  LIBRARIES — Arduino IDE Library Manager
//  Search "Makeblock" → install "Makeblock Electronics"
//
//  BOARD
//  Tools → Board → Arduino Mega 2560
//  Tools → Port → your COM port
// ============================================================