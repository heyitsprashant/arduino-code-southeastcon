#define MOTOR_EN  9
#define MOTOR_IN1 10
#define MOTOR_IN2 11
#define ENCODER_PIN 2
#define GREEN_LED 4
#define RED_LED   5

volatile int pulseCount = 0;
float rpm = 0;
unsigned long lastTime = 0;

void countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);
  
  pinMode(MOTOR_EN,  OUTPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED,   OUTPUT);
  
  // Use CHANGE instead of RISING to catch more pulses
  pinMode(ENCODER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countPulse, CHANGE);
  
  Serial.println("Waiting 3 seconds...");
  digitalWrite(RED_LED, HIGH);
  delay(1000);
  Serial.println("2...");
  delay(1000);
  Serial.println("1...");
  delay(1000);
  digitalWrite(RED_LED, LOW);
  
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  analogWrite(MOTOR_EN, 255);
  
  Serial.println("Motor Started!");
  digitalWrite(GREEN_LED, HIGH);
  
  lastTime = millis();
}

void loop() {
  if (millis() - lastTime >= 1000) {
    
    noInterrupts();
    int count = pulseCount;
    pulseCount = 0;
    interrupts();
    
    // Divide by 40 because CHANGE catches both edges
    rpm = (count / 40.0) * 60.0;
    lastTime = millis();
    
    Serial.print("RPM: ");
    Serial.println(rpm);
    
    if(rpm > 0) {
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
    } else {
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, HIGH);
    }
  }
}