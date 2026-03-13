
//  Sensors : Line Follower on Port 7, Ultrasonic on Port 6
//  Gripper : Port 3 (flag held closed at start, lift + open to release)

// ============================================================

// ── SECTION 1: LIBRARIES ────────────────────────────────────
#include "MeUltrasonicSensor.h"   // ultrasonic distance sensor
#include "MeLineFollower.h"       // line follower light sensor
#include "MeEncoderMotor.h"       // encoder drive motors
#include "MeServo.h"              // gripper uses servo-based motor
#include "Arduino.h"

// ── SECTION 2: HARDWARE DECLARATIONS ────────────────────────
MeUltrasonicSensor ultrasonic(PORT_6);    // ultrasonic on port 6
MeLineFollower     lineSensor(PORT_7);    // line follower on port 7
MeEncoderMotor     motorL(0x09, SLOT1);   // left drive motor
MeEncoderMotor     motorR(0x09, SLOT2);   // right drive motor

// Gripper on Port 3
// mBot Ultimate gripper has two servos:
//   - one servo lifts the arm up/down
//   - one servo opens/closes the claw
// Port 3 on mCore2 maps to pins 25 (S1) and 26 (S2)
MeServo gripperLift;    // controls arm height (up/down)
MeServo gripperClaw;    // controls claw (open/close)

#define GRIPPER_LIFT_PIN   25   // Port 3, S1 — lift arm servo
#define GRIPPER_CLAW_PIN   26   // Port 3, S2 — claw open/close servo

// Gripper angle constants — adjust these during calibration
#define LIFT_DOWN    30    // arm lowered (flag resting position during drive)
#define LIFT_UP      120   // arm raised (lift before releasing flag)
#define CLAW_CLOSED  20    // claw gripping the flag
#define CLAW_OPEN    90    // claw fully open (flag released)

// ── SECTION 3: TUNING CONSTANTS ─────────────────────────────
#define LIGHT_THRESHOLD   800    // line sensor reading that means LED bar detected
                                 // CALIBRATE: open Serial Monitor, note reading under
                                 // normal room light, then flash bright white LED above
                                 // sensor. Set this value between the two readings.

#define DRIVE_SPEED       60     // forward drive speed (0–255)
#define RETURN_SPEED      45     // return speed — slower for more control

// 20 inches = 508mm
// CALIBRATE: at DRIVE_SPEED 60, measure how far robot goes in 1000ms.
// Formula: FLAG_DRIVE_TIME = (508 / mm_per_1000ms) * 1000
// Example: if robot does 300mm/s → FLAG_DRIVE_TIME = (508/300)*1000 = 1693ms
#define FLAG_DRIVE_TIME   1700   // ms to drive ~20 inches forward before releasing flag

// After flag release, continue forward a bit more before turning back
// This ensures the flag is well clear of the start zone
#define EXTRA_DRIVE_TIME  500    // ms of extra forward drive after flag drop

#define HOME_DISTANCE     10     // stop reversing when ultrasonic reads < 10 cm
                                 // the arena wall sits right behind the start zone
                                 // increase to 12–15 if robot overshoots

// ── SECTION 4: MOVEMENT HELPERS ─────────────────────────────

// drive both motors forward at given speed
void driveForward(int spd) {
  motorL.run(spd);     // left motor forward
  motorR.run(-spd);    // right motor forward (mounted mirrored so sign flipped)
                       // if robot spins instead of going straight → flip sign on motorR
}

// drive both motors in reverse at given speed
void driveBackward(int spd) {
  motorL.run(-spd);    // left motor reverse
  motorR.run(spd);     // right motor reverse
}

// stop both drive motors immediately
void stopMotors() {
  motorL.run(0);
  motorR.run(0);
}

// ── SECTION 5: TASK FUNCTIONS ────────────────────────────────

// ----------------------------------------------------------
// TASK 1: Wait for LED start bar  (+15 pts)
// ----------------------------------------------------------
// Robot sits frozen until the white LED bar flashes at match start.
// Line follower detects the brightness spike and breaks the loop.
void waitForStart() {
  Serial.println("STATUS: Waiting for LED start bar...");

  bool started = false;

  while (!started) {
    int lightVal = lineSensor.readSensors();   // read current light level

    // print to Serial Monitor so you can watch the value live
    Serial.print("Light reading: ");
    Serial.println(lightVal);

    if (lightVal > LIGHT_THRESHOLD) {
      started = true;   // flash detected — exit waiting loop
      Serial.println("STATUS: START DETECTED — run begins!");
    }

    delay(50);   // poll every 50ms — fast enough to catch a 1-second flash
  }

  // short pause after detection — lets the LED bar finish its flash
  // and prevents the sensor from causing false triggers later
  delay(200);
}

// ----------------------------------------------------------
// TASK 2: Leave starting area  (+10 pts)
// ----------------------------------------------------------
// Drive straight forward for FLAG_DRIVE_TIME milliseconds.
// At DRIVE_SPEED 60 this covers ~20 inches — past the 12" start zone.
// We plant the flag at this point (see plantFlag below),
// then drive a little further before any turning.
void leaveStartZone() {
  Serial.println("STATUS: Leaving start zone, driving 20 inches...");

  driveForward(DRIVE_SPEED);
  delay(FLAG_DRIVE_TIME);   // drive exactly 20 inches
  stopMotors();

  Serial.println("STATUS: 20 inches reached — at flag plant position.");
  delay(300);   // brief pause before gripper moves
}

// ----------------------------------------------------------
// TASK 3: Plant school flag  (+10 pts)
// ----------------------------------------------------------
// Gripper starts closed, holding the JSU flag.
// Sequence:
//   1. Lift arm up (raise flag off robot body)
//   2. Open claw (release flag — it drops onto surface)
//   3. Lower arm back down (clear of the flag)
void plantFlag() {
  Serial.println("STATUS: Planting JSU flag...");

  // step 1 — lift arm up
  gripperLift.write(LIFT_UP);
  delay(700);   // wait for arm to fully raise (servo needs time)
  Serial.println("STATUS: Arm raised.");

  // step 2 — open claw to release flag
  gripperClaw.write(CLAW_OPEN);
  delay(600);   // wait for claw to fully open and flag to drop
  Serial.println("STATUS: Flag released!");

  // step 3 — lower arm back down so it doesn't catch on anything
  gripperLift.write(LIFT_DOWN);
  delay(500);   // wait for arm to lower

  Serial.println("STATUS: Flag planted successfully.");
  delay(400);   // pause before continuing
}

// ----------------------------------------------------------
// TASK 4: Return to starting area  (+15 pts)
// ----------------------------------------------------------
// Drive a little further forward first (flag is behind us now,
// we don't want to reverse over it immediately).
// Then turn 180 degrees and reverse straight back.
// Ultrasonic detects the arena wall behind the start zone
// and stops the robot inside the 12" zone.
void returnHome() {
  Serial.println("STATUS: Starting return journey...");

  // drive forward a little more so we have room to turn
  // without the robot bumping into the dropped flag
  driveForward(DRIVE_SPEED);
  delay(EXTRA_DRIVE_TIME);
  stopMotors();
  delay(300);

  // turn 180 degrees so we're facing the start zone
  // CALIBRATE: adjust delay until robot turns ~180 degrees
  // test: at speed 60, time a full 360 spin, then use half that time here
  Serial.println("STATUS: Turning 180 degrees...");
  motorL.run(60);    // both motors same direction = spin in place
  motorR.run(60);
  delay(1100);       // adjust this value until robot turns ~180 degrees
  stopMotors();
  delay(300);

  // now drive forward (toward start zone) until ultrasonic detects wall
  Serial.println("STATUS: Driving toward start zone...");
  driveForward(RETURN_SPEED);

  while (ultrasonic.distanceCm() > HOME_DISTANCE) {
    // print distance so you can watch it close in on Serial Monitor
    Serial.print("Distance to wall: ");
    Serial.print(ultrasonic.distanceCm());
    Serial.println(" cm");
    delay(50);   // check every 50ms while driving
  }

  stopMotors();
  Serial.println("STATUS: HOME — inside start zone!");
}

// ── SECTION 6: SETUP ─────────────────────────────────────────
void setup() {
  Serial.begin(9600);   // open serial at 9600 baud for debugging
  Serial.println("STATUS: System initialising...");

  // attach gripper servos to their pins
  gripperLift.attach(GRIPPER_LIFT_PIN);
  gripperClaw.attach(GRIPPER_CLAW_PIN);

  // set gripper to starting position:
  // arm down, claw closed (holding the flag)
  gripperLift.write(LIFT_DOWN);
  gripperClaw.write(CLAW_CLOSED);
  delay(800);   // wait for both servos to reach start position

  // initialise drive motors
  motorL.begin();
  motorR.begin();

  delay(500);
  Serial.println("STATUS: Ready. Waiting for start signal.");
}

// ── SECTION 7: MAIN RUN SEQUENCE ─────────────────────────────
void loop() {

  // PHASE 1 — wait for LED bar flash          +15 pts (auto-start)
  waitForStart();

  // PHASE 2 — drive 20 inches forward         +10 pts (leave start zone)
  leaveStartZone();

  // PHASE 3 — lift arm and release flag       +10 pts (plant flag)
  plantFlag();

  // PHASE 4 — return to start zone            +15 pts (end in start zone)
  returnHome();

  // ── MATCH COMPLETE ──
  Serial.println("==============================================");
  Serial.println("MATCH COMPLETE");
  Serial.println("Points scored:");
  Serial.println("  Auto-start (LED bar)   : 15 pts");
  Serial.println("  Left start zone        : 10 pts");
  Serial.println("  Planted flag           : 10 pts");
  Serial.println("  Returned to start zone : 15 pts");
  Serial.println("  TOTAL                  : 50 pts");
  Serial.println("  + Design competition   : 15 pts");
  Serial.println("  GRAND TOTAL            : 65 pts");
  Serial.println("==============================================");

  // halt forever — stops loop() from restarting the program
  while (true) {
    delay(1000);
  }
}

// ============================================================
//  CALIBRATION GUIDE — do these in order before competition
// ============================================================
//
//  STEP 1 — LIGHT THRESHOLD
//  Open Serial Monitor (9600 baud). Power on robot in normal
//  room lighting. Note the "Light reading" value printed.
//  Then hold a bright white LED/torch above the line sensor.
//  Note the new higher value. Set LIGHT_THRESHOLD between them.
//  Example: room light = 350, LED flash = 920 → set to 700
//
//  STEP 2 — FLAG_DRIVE_TIME (20 inches)
//  Put a tape measure on the floor. Upload code with only
//  leaveStartZone() in loop(). Run it and measure actual distance.
//  If robot travels 17 inches → increase FLAG_DRIVE_TIME by ~15%
//  If robot travels 23 inches → decrease FLAG_DRIVE_TIME by ~15%
//
//  STEP 3 — GRIPPER ANGLES
//  Test each angle constant one by one:
//  LIFT_DOWN  : arm should sit flat/low while driving
//  LIFT_UP    : arm should raise flag clear of robot body
//  CLAW_CLOSED: gripper should firmly hold the flag
//  CLAW_OPEN  : gripper should fully open and release flag
//
//  STEP 4 — 180 DEGREE TURN
//  In returnHome(), the delay(1100) controls turn amount.
//  Test: does robot turn ~180 degrees? If overshooting → reduce.
//  If undershooting → increase. Aim for ±10 degrees accuracy.
//
//  STEP 5 — HOME_DISTANCE
//  Run returnHome() alone. Watch Serial Monitor for distance
//  readings. Confirm robot stops cleanly inside the 12" zone.
//  If it stops too far out → increase HOME_DISTANCE value.
//  If it hits the wall → decrease HOME_DISTANCE value.
//
//  STEP 6 — FULL DRY RUN
//  Run the complete program 3 times in a row. All 3 should
//  complete cleanly before competition day.
//
// ============================================================
//  LIBRARY INSTALL (Arduino IDE Library Manager)
//  Search: "Makeblock" → install "Makeblock Electronics"
//
//  BOARD SETTING
//  Tools → Board → Arduino AVR Boards → Arduino Mega 2560
//  Tools → Port → your COM port
// ============================================================