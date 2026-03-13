// ============================================================
//  IEEE SoutheastCon 2026 — FULL COMPETITION PROGRAM
//  Board   : mBot Ultimate 2.0 (mCore2 / ATmega2560)
//  Sensors : Line Follower Port 7, Ultrasonic Port 6
//  Gripper : Port 3 (lift = pin 25, claw = pin 26)
//
//  PHASE 1  Wait for white LED start bar         +15 pts
//  PHASE 2  Leave start zone                     +10 pts
//  PHASE 3  Plant JSU flag                       +10 pts
//  PHASE 4  Return to start zone (reverse home)
//  PHASE 5  Leave start zone again (crater run)
//  PHASE 6  Follow white dividing line rightward
//  PHASE 7  Enter crater (wheels cross crater line) +20 pts
//  PHASE 8  Exit crater + full lap around crater    +35 pts
//  PHASE 9  Drive to crater center + return home  +15 pts
//
//  TARGET   105 pts + 15 design comp = 120 pts
// ============================================================

// ── CHANGE THIS TO TEST INDIVIDUAL PHASES ───────────────────
//  0  = Full competition run (all phases)
//  1  = Phase 1 only — LED bar detection
//  2  = Phase 2 only — leave start zone
//  3  = Phase 3 only — plant flag (gripper test)
//  4  = Phase 4 only — return home via ultrasonic
//  5  = Phase 5+6 — leave start + follow line (stops at crater edge)
//  6  = Phase 7 only — enter + exit crater (place robot at edge first)
//  7  = Phase 8 only — full crater lap (place robot at edge first)
//  8  = Phase 9 only — drive to center + return home
//  9  = Sensor debug — prints all sensor values, robot does not move
//
#define TEST_MODE  0

// ── LIBRARIES ───────────────────────────────────────────────
#include "MeUltrasonicSensor.h"
#include "MeLineFollower.h"
#include "MeEncoderMotor.h"
#include "MeServo.h"
#include "Arduino.h"

// ── HARDWARE ────────────────────────────────────────────────
MeUltrasonicSensor ultrasonic(PORT_6);
MeLineFollower     lineSensor(PORT_7);
MeEncoderMotor     motorL(0x09, SLOT1);
MeEncoderMotor     motorR(0x09, SLOT2);
MeServo            gripperLift;
MeServo            gripperClaw;

#define GRIPPER_LIFT_PIN  25
#define GRIPPER_CLAW_PIN  26

// ── TUNING CONSTANTS ────────────────────────────────────────
// --- Light detection ---
#define LED_BAR_THRESHOLD   800   // white start LED bar spike value
                                  // CALIBRATE: run TEST_MODE 9, note room
                                  // light reading, flash white light above
                                  // sensor, set threshold between the two

// --- Line following ---
#define LINE_DETECTED       600   // white dividing line OR crater line reading
                                  // CALIBRATE: run TEST_MODE 9, slowly roll
                                  // robot over the white painted line, note
                                  // peak value, set this ~50 below that peak
#define LINE_LOST_TIME      800   // ms of not seeing the line before we
                                  // assume we've passed the crater area

// --- Motor speeds ---
#define DRIVE_SPEED         60    // normal forward speed (0-255)
#define SLOW_SPEED          35    // slow speed for crater entry/exit
#define RETURN_SPEED        45    // reverse speed returning home
#define LAP_SPEED           40    // crater lap arc speed
#define LINE_FOLLOW_SPEED   45    // speed while following the dividing line

// --- Distances ---
#define HOME_DISTANCE       10    // cm — stop when this close to start wall
#define CRATER_STOP_DIST    15    // cm — ultrasonic stop distance inside crater
                                  // flat base is 8" diameter, stop before
                                  // hitting antenna #3 at the center

// --- Timing ---
#define FLAG_DRIVE_TIME     1700  // ms to drive ~20 inches for flag plant
                                  // CALIBRATE: measure actual 20" at DRIVE_SPEED

#define TURN_90_MS          550   // ms for a 90 degree pivot turn
                                  // CALIBRATE: watch robot turn, adjust until
                                  // it turns exactly 90 degrees

#define ENTER_DEPTH_MS      800   // ms to drive deeper into crater after
                                  // detecting the crater line on entry

#define LAP_TIME_MS         7200  // ms for one full crater circumference lap
                                  // Crater outer rim = 12" radius
                                  // Lap radius ~9" (below crater line)
                                  // Circumference = 2*PI*9" = ~56.5 inches
                                  // At LAP_SPEED 40 (~200mm/s) = ~7200ms
                                  // CALIBRATE: watch if robot completes
                                  // exactly one loop, adjust accordingly

#define CENTER_DRIVE_MS     600   // ms to drive from crater lap position
                                  // down into the flat 8" base center
                                  // CALIBRATE: should stop near antenna #3

// --- Gripper ---
#define LIFT_DOWN   30    // arm resting position while driving
#define LIFT_UP     120   // arm raised to release flag
#define CLAW_CLOSED 20    // gripping the flag
#define CLAW_OPEN   90    // releasing the flag

// ── MOVEMENT HELPERS ────────────────────────────────────────

void driveForward(int spd) {
  motorL.run(spd);
  motorR.run(-spd);   // right motor mounted mirrored — sign flipped
                      // if robot spins instead of going straight,
                      // change -spd to spd here
}

void driveBackward(int spd) {
  motorL.run(-spd);
  motorR.run(spd);
}

void turnLeft(int spd) {
  motorL.run(-spd);   // both treads same direction = pivot
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

// steerLeft: slight left correction while line following
// left tread slows down, right continues — gentle curve left
void steerLeft(int spd) {
  motorL.run(spd / 2);
  motorR.run(-spd);
}

// steerRight: slight right correction while line following
// right tread slows down, left continues — gentle curve right
void steerRight(int spd) {
  motorL.run(spd);
  motorR.run(-(spd / 2));
}

// circleLeft: differential speed arc for crater lap
// outer (right) tread faster, inner (left) tread slower = left arc
void circleLeft(int outerSpd, int innerSpd) {
  motorL.run(innerSpd);
  motorR.run(-outerSpd);
}

// ── PHASE FUNCTIONS ─────────────────────────────────────────

// ----------------------------------------------------------
// PHASE 1: Wait for white LED start bar  (+15 pts)
// ----------------------------------------------------------
// Sits frozen until line sensor detects the 1-second white
// LED flash from the arena start bar mounted on the wall
// above the starting area.
void phase1_waitForStart() {
  Serial.println("PHASE 1: Waiting for LED start bar...");
  bool started = false;
  while (!started) {
    int val = lineSensor.readSensors();
    Serial.print("Light: "); Serial.println(val);
    if (val > LED_BAR_THRESHOLD) {
      started = true;
      Serial.println("PHASE 1: START DETECTED!");
    }
    delay(50);  // poll every 50ms — catches 1-second flash easily
  }
  delay(200);   // brief pause after detection before driving
}

// ----------------------------------------------------------
// PHASE 2: Leave starting area  (+10 pts)
// ----------------------------------------------------------
// Drive straight forward for FLAG_DRIVE_TIME milliseconds
// to clear the 12"x12" start zone and reach the flag plant
// position (~20 inches from the start wall).
void phase2_leaveStartZone() {
  Serial.println("PHASE 2: Leaving start zone...");
  driveForward(DRIVE_SPEED);
  delay(FLAG_DRIVE_TIME);   // drive ~20 inches
  stopMotors();
  delay(300);
  Serial.println("PHASE 2: Done — past start zone.");
}

// ----------------------------------------------------------
// PHASE 3: Plant school flag  (+10 pts)
// ----------------------------------------------------------
// Gripper lifts arm up, opens claw, JSU flag drops onto
// the playing surface. Arm lowers back after release.
void phase3_plantFlag() {
  Serial.println("PHASE 3: Planting JSU flag...");
  gripperLift.write(LIFT_UP);     // raise arm
  delay(700);
  gripperClaw.write(CLAW_OPEN);   // open claw — flag drops
  delay(600);
  gripperLift.write(LIFT_DOWN);   // lower arm back down
  delay(500);
  Serial.println("PHASE 3: Flag planted.");
}

// ----------------------------------------------------------
// PHASE 4: Return to start zone
// ----------------------------------------------------------
// Robot needs to go back to start before the crater run.
// Turn 180°, drive forward toward start wall, stop when
// ultrasonic detects the arena wall behind start zone.
void phase4_returnToStart() {
  Serial.println("PHASE 4: Returning to start zone...");

  // turn 180 degrees — calibrate TURN_90_MS first then double it
  turnRight(DRIVE_SPEED);
  delay(TURN_90_MS * 2);    // 2x the 90-degree turn time = 180 degrees
  stopMotors();
  delay(300);

  // drive toward start wall until close enough
  driveForward(RETURN_SPEED);
  while (ultrasonic.distanceCm() > HOME_DISTANCE) {
    Serial.print("Distance: "); Serial.println(ultrasonic.distanceCm());
    delay(50);
  }
  stopMotors();
  Serial.println("PHASE 4: Inside start zone.");
  delay(500);   // pause inside start zone before crater run
}

// ----------------------------------------------------------
// PHASE 5: Leave start zone again (crater run begins)
// ----------------------------------------------------------
// Robot exits start zone and turns to face the white
// dividing line between Area 1 and Area 3.
// The dividing line runs north-south across the center
// of the board. From start zone (lower-left), robot drives
// forward then turns right to face the line.
void phase5_leaveForCrater() {
  Serial.println("PHASE 5: Leaving start zone for crater run...");

  // drive forward to clear the start zone
  driveForward(DRIVE_SPEED);
  delay(800);   // short burst — just enough to clear the zone
  stopMotors();
  delay(200);

  // turn right 90° to face toward the dividing line
  // (dividing line runs vertically, robot needs to face right/east)
  turnRight(DRIVE_SPEED);
  delay(TURN_90_MS);
  stopMotors();
  delay(300);

  Serial.println("PHASE 5: Facing dividing line — ready to follow.");
}

// ----------------------------------------------------------
// PHASE 6: Follow white dividing line to crater
// ----------------------------------------------------------
// Robot follows the white painted dividing line between
// Area 1/2 (left) and Area 3 (right) across the board.
// Line following logic:
//   - Line sensor sees white → on the line → drive straight
//   - Line sensor drops below threshold → drifted off line
//     → steer back toward it
//
// The line runs from the top wall to the bottom wall of the
// board. Robot approaches from the left (Area 1 side) and
// drives along it rightward toward the crater zone.
//
// We stop following when the crater line is detected OR
// ultrasonic detects we're close to the crater.
void phase6_followLineToCrater() {
  Serial.println("PHASE 6: Following dividing line to crater...");

  // first: drive forward until we reach the dividing line
  // the line is roughly in the center of the board
  driveForward(LINE_FOLLOW_SPEED);

  bool lineFound = false;
  unsigned long searchStart = millis();

  // search for the dividing line — drive until sensor detects it
  while (!lineFound) {
    int val = lineSensor.readSensors();
    Serial.print("Searching for line: "); Serial.println(val);

    if (val > LINE_DETECTED) {
      lineFound = true;
      Serial.println("PHASE 6: Dividing line found!");
    }

    // safety timeout — if no line found in 4 seconds something is wrong
    if (millis() - searchStart > 4000) {
      Serial.println("PHASE 6: WARNING — line not found, continuing anyway.");
      break;
    }
    delay(50);
  }

  // now follow the line rightward toward the crater
  // line following uses simple bang-bang control:
  //   on line → go straight
  //   off line → steer back
  Serial.println("PHASE 6: Following line toward crater...");

  unsigned long lastLineTime = millis();  // tracks when we last saw the line
  bool reachedCrater = false;

  while (!reachedCrater) {
    int lineVal = lineSensor.readSensors();
    float dist  = ultrasonic.distanceCm();

    Serial.print("Line: "); Serial.print(lineVal);
    Serial.print("  Dist: "); Serial.println(dist);

    if (lineVal > LINE_DETECTED) {
      // on the line — drive straight
      driveForward(LINE_FOLLOW_SPEED);
      lastLineTime = millis();   // reset lost-line timer
    } else {
      // off the line — steer right to find it again
      // dividing line is to our right so steer right when lost
      steerRight(LINE_FOLLOW_SPEED);
    }

    // stop following if:
    // 1. Ultrasonic detects something close ahead (crater area)
    if (dist < 30 && dist > 0) {
      reachedCrater = true;
      Serial.println("PHASE 6: Obstacle ahead — crater zone reached.");
    }

    // 2. We've been off the line too long (passed the crater area)
    if (millis() - lastLineTime > LINE_LOST_TIME) {
      reachedCrater = true;
      Serial.println("PHASE 6: Line lost — assuming crater zone reached.");
    }

    delay(50);
  }

  stopMotors();
  delay(300);
  Serial.println("PHASE 6: Arrived at crater zone.");
}

// ----------------------------------------------------------
// PHASE 7: Enter and exit crater  (+20 pts)
// ----------------------------------------------------------
// Scoring rule: some portion of drive mechanism must touch
// the crater line (white ring 3" from crater rim).
//
// Strategy:
//   1. Crawl forward until crater line detected
//   2. Continue briefly to get wheels clearly past the line
//   3. Reverse back out until crater line detected again
//   4. Continue reversing briefly to fully clear the crater
void phase7_enterExitCrater() {
  Serial.println("PHASE 7: Enter + exit crater...");

  // -- ENTER --
  Serial.println("PHASE 7: Crawling toward crater line...");
  driveForward(SLOW_SPEED);

  bool lineHit = false;
  while (!lineHit) {
    int val = lineSensor.readSensors();
    Serial.print("Crater approach: "); Serial.println(val);
    if (val > LINE_DETECTED) {
      lineHit = true;
      Serial.println("PHASE 7: CRATER LINE HIT — entering!");
    }
    delay(50);
  }

  // drive deeper to ensure wheels fully cross the line
  delay(ENTER_DEPTH_MS);
  stopMotors();
  Serial.println("PHASE 7: Inside crater.");
  delay(400);

  // -- EXIT --
  Serial.println("PHASE 7: Reversing out...");
  driveBackward(SLOW_SPEED);

  // drive back a fixed amount first before checking for line
  // (avoids immediate re-detection of the same line)
  delay(300);

  lineHit = false;
  while (!lineHit) {
    int val = lineSensor.readSensors();
    Serial.print("Crater exit: "); Serial.println(val);
    if (val > LINE_DETECTED) {
      lineHit = true;
      Serial.println("PHASE 7: CRATER LINE HIT on exit!");
    }
    delay(50);
  }

  // drive a little further to fully clear the crater rim
  delay(500);
  stopMotors();
  Serial.println("PHASE 7: CRATER ENTER+EXIT COMPLETE — 20 pts!");
  delay(400);
}

// ----------------------------------------------------------
// PHASE 8: Full crater lap  (+35 pts)
// ----------------------------------------------------------
// Scoring rule: robot wheels/tracks must complete one full
// loop around the crater while staying below the crater line
// (i.e. inside the 3" band from the rim).
//
// Strategy:
//   1. Turn 90° to face tangent to the crater circle
//   2. Nudge forward to position at the crater line
//   3. Run differential arc (inner slower, outer faster)
//   4. Use line sensor to stay on the crater line as guide
//   5. Complete LAP_TIME_MS milliseconds = one full circle
void phase8_fullCraterLap() {
  Serial.println("PHASE 8: Full crater lap...");

  // position robot tangent to the crater by turning 90°
  turnRight(DRIVE_SPEED);
  delay(TURN_90_MS);
  stopMotors();
  delay(300);

  // nudge forward to get back onto the crater line
  driveForward(SLOW_SPEED);
  delay(400);
  stopMotors();
  delay(200);

  // start the arc
  // left tread = inner (slower), right tread = outer (faster)
  // difference in speed determines the arc radius
  // CALIBRATE: if robot spirals inward → increase innerSpd
  //            if robot drifts outside line → decrease innerSpd
  int outerSpd = LAP_SPEED;
  int innerSpd = LAP_SPEED / 3;   // ~1/3 = tight arc around crater

  Serial.println("PHASE 8: Circling...");
  unsigned long lapStart = millis();

  while (millis() - lapStart < LAP_TIME_MS) {
    int lineVal = lineSensor.readSensors();

    // line sensor correction to stay on crater line during lap
    if (lineVal < LINE_DETECTED - 100) {
      // drifted too far inward (off line toward center)
      // speed up inner tread slightly to push outward
      motorL.run(innerSpd + 8);
      motorR.run(-outerSpd);
    } else if (lineVal > LINE_DETECTED + 100) {
      // drifted too far outward (past the line)
      // slow inner tread to pull arc inward
      motorL.run(innerSpd - 8);
      motorR.run(-outerSpd);
    } else {
      // on track — normal arc speeds
      circleLeft(outerSpd, innerSpd);
    }

    Serial.print("Lap: "); Serial.println(lineVal);
    delay(50);
  }

  stopMotors();
  Serial.println("PHASE 8: FULL LAP COMPLETE — 35 pts!");
  delay(400);
}

// ----------------------------------------------------------
// PHASE 9: Drive to crater center + return home  (+15 pts)
// ----------------------------------------------------------
// After the lap, drive down into the flat 8" base at the
// center of the crater (where antenna #3 sits).
// Then reverse out, navigate back to start zone,
// and stop inside for the final +15 pts.
void phase9_centerAndReturnHome() {
  Serial.println("PHASE 9: Driving to crater center...");

  // turn to face the crater center (inward)
  turnLeft(DRIVE_SPEED);
  delay(TURN_90_MS);
  stopMotors();
  delay(300);

  // drive slowly toward the flat base
  // stop when ultrasonic detects antenna #3 or the crater wall
  driveForward(SLOW_SPEED);
  delay(CENTER_DRIVE_MS);
  stopMotors();
  Serial.println("PHASE 9: At crater center.");
  delay(500);

  // ── NOW RETURN HOME ──────────────────────────────────────
  Serial.println("PHASE 9: Returning to start zone...");

  // reverse out of crater
  driveBackward(SLOW_SPEED);
  delay(CENTER_DRIVE_MS + 400);   // slightly longer to fully exit
  stopMotors();
  delay(300);

  // turn to face the start zone (left side of board)
  // from crater we need to turn left ~90° to face the start wall
  turnLeft(DRIVE_SPEED);
  delay(TURN_90_MS);
  stopMotors();
  delay(300);

  // drive toward start wall — ultrasonic stops us when close
  Serial.println("PHASE 9: Driving toward start wall...");
  driveForward(RETURN_SPEED);

  while (ultrasonic.distanceCm() > HOME_DISTANCE) {
    Serial.print("Distance: "); Serial.println(ultrasonic.distanceCm());
    delay(50);
  }

  stopMotors();
  Serial.println("PHASE 9: HOME — inside start zone! +15 pts");
}

// ── SENSOR DEBUG ────────────────────────────────────────────
void sensorDebug() {
  Serial.println("SENSOR DEBUG — robot will not move.");
  Serial.println("Roll over lines, approach wall, flash LED above sensor.");
  while (true) {
    int  lineVal = lineSensor.readSensors();
    float dist   = ultrasonic.distanceCm();
    Serial.print("Line: ");      Serial.print(lineVal);
    Serial.print("  |  Dist: "); Serial.print(dist);
    Serial.println(" cm");
    delay(200);
  }
}

// ── SETUP ────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  Serial.println("=== SoutheastCon 2026 ===");
  Serial.print("TEST MODE: "); Serial.println(TEST_MODE);

  gripperLift.attach(GRIPPER_LIFT_PIN);
  gripperClaw.attach(GRIPPER_CLAW_PIN);
  gripperLift.write(LIFT_DOWN);    // arm down — flag loaded
  gripperClaw.write(CLAW_CLOSED);  // claw closed — holding flag
  delay(800);

  motorL.begin();
  motorR.begin();
  delay(500);

  Serial.println("Ready.");
}

// ── MAIN LOOP ────────────────────────────────────────────────
void loop() {

  switch (TEST_MODE) {

    case 0:
      // ── FULL COMPETITION RUN ──
      phase1_waitForStart();       // +15 pts
      phase2_leaveStartZone();     // +10 pts
      phase3_plantFlag();          // +10 pts
      phase4_returnToStart();      // back to start zone
      phase5_leaveForCrater();     // leave start again
      phase6_followLineToCrater(); // follow dividing line
      phase7_enterExitCrater();    // +20 pts
      phase8_fullCraterLap();      // +35 pts
      phase9_centerAndReturnHome();// +15 pts

      Serial.println("==========================================");
      Serial.println("MATCH COMPLETE");
      Serial.println("  Auto-start (LED bar)  : 15 pts");
      Serial.println("  Left start zone        : 10 pts");
      Serial.println("  Planted flag           : 10 pts");
      Serial.println("  Crater enter + exit    : 20 pts");
      Serial.println("  Full crater lap        : 35 pts");
      Serial.println("  Returned home          : 15 pts");
      Serial.println("  TOTAL                  : 105 pts");
      Serial.println("  + Design competition   : 15 pts");
      Serial.println("  GRAND TOTAL            : 120 pts");
      Serial.println("==========================================");
      break;

    case 1:
      phase1_waitForStart();
      Serial.println("TEST DONE — adjust LED_BAR_THRESHOLD if needed.");
      break;

    case 2:
      phase2_leaveStartZone();
      Serial.println("TEST DONE — measure distance, adjust FLAG_DRIVE_TIME.");
      break;

    case 3:
      phase3_plantFlag();
      Serial.println("TEST DONE — check arm height and flag drop.");
      break;

    case 4:
      phase4_returnToStart();
      Serial.println("TEST DONE — confirm robot stops inside 12 inch zone.");
      break;

    case 5:
      phase5_leaveForCrater();
      phase6_followLineToCrater();
      Serial.println("TEST DONE — check robot arrives at crater zone.");
      break;

    case 6:
      Serial.println("Place robot at crater edge then reset board.");
      delay(3000);
      phase7_enterExitCrater();
      Serial.println("TEST DONE — adjust ENTER_DEPTH_MS if needed.");
      break;

    case 7:
      Serial.println("Place robot at crater edge then reset board.");
      delay(3000);
      phase8_fullCraterLap();
      Serial.println("TEST DONE — adjust innerSpd and LAP_TIME_MS.");
      break;

    case 8:
      Serial.println("Place robot near crater then reset board.");
      delay(3000);
      phase9_centerAndReturnHome();
      Serial.println("TEST DONE — confirm robot reaches center then home.");
      break;

    case 9:
      sensorDebug();
      break;

    default:
      Serial.println("ERROR: Invalid TEST_MODE. Use 0-9.");
      break;
  }

  // halt — stops loop() from restarting after test
  while (true) { delay(1000); }
}

// ============================================================
//  CALIBRATION ORDER — do these in order, never skip ahead
// ============================================================
//
//  TEST 9 first — sensor debug, robot doesn't move
//    Watch line sensor over white dividing line → set LINE_DETECTED
//    Watch line sensor under LED flash → set LED_BAR_THRESHOLD
//    Watch ultrasonic approaching a wall → confirm readings
//
//  TEST 3 — gripper only
//    Arm should rise cleanly, flag should drop, arm lowers back
//    Tune LIFT_UP, LIFT_DOWN, CLAW_OPEN angles
//
//  TEST 2 — leave start zone
//    Measure actual distance at DRIVE_SPEED
//    Adjust FLAG_DRIVE_TIME until robot travels ~20 inches
//
//  TEST 4 — return home
//    Tune TURN_90_MS — robot should turn exactly 180 degrees
//    Confirm robot stops cleanly inside the 12 inch start zone
//
//  TEST 1 — auto start
//    Flash white LED above sensor 3 times
//    Should trigger reliably all 3 times
//
//  TEST 5 — line follow
//    Robot should find the dividing line and track it
//    rightward toward the crater zone without drifting
//
//  TEST 6 — crater enter/exit
//    Place robot at crater edge manually
//    Confirm "CRATER LINE HIT" appears on entry AND exit
//
//  TEST 7 — crater lap
//    Place robot at crater edge manually
//    Adjust innerSpd if arc is too tight or too wide
//    Adjust LAP_TIME_MS until exactly one full loop
//
//  TEST 8 — center + return home
//    Confirm robot descends to crater base and returns home
//
//  TEST 0 — full run
//    Run 3 times in a row — all 3 must complete cleanly
// ============================================================