
#include <Stepper.h>
#include <Servo.h>


const int STEPS_PER_REV      = 2048;              
const int STEPS_FOR_180      = STEPS_PER_REV / 2;
const int TRACK_STEP_SIZE    = 8;
const int LDR_DEADBAND       = 15;           
const int DAYLIGHT_THRESHOLD = 200;

const unsigned long TRACK_INTERVAL_MS = 1000UL;

const int SERVO_CLOSED_ANGLE     = 90;
const int SERVO_LEFT_OPEN_ANGLE  = 180;
const int SERVO_RIGHT_OPEN_ANGLE = 0;

const int PIN_LDR_EAST    = A0;
const int PIN_LDR_WEST    = A1;
const int PIN_SERVO_LEFT  = 5;
const int PIN_SERVO_RIGHT = 6;

Stepper stepper(STEPS_PER_REV, 8, 10, 9, 11);
Servo servoLeft;
Servo servoRight;

int stepperPosition = 0;
bool panelOpen = false;
unsigned long lastCheckTime = 0;

void setup() {
  Serial.begin(9600);
  stepper.setSpeed(10);

  servoLeft.attach(PIN_SERVO_LEFT);
  servoRight.attach(PIN_SERVO_RIGHT);
  closePanel();

  lastCheckTime = millis();
}

void loop() {
  if (millis() - lastCheckTime < TRACK_INTERVAL_MS) return;
  lastCheckTime = millis();

  int east = readLDR(PIN_LDR_EAST);
  int west = readLDR(PIN_LDR_WEST);
  int ambient = (east + west) / 2;

  Serial.print("East: "); Serial.print(east);
  Serial.print("  West: "); Serial.print(west);
  Serial.print("  Ambient: "); Serial.print(ambient);
  Serial.print("  Position: "); Serial.println(stepperPosition);

  bool isDaylight = ambient > DAYLIGHT_THRESHOLD;

  if (isDaylight && !panelOpen) {
    openPanel();
  } else if (!isDaylight && panelOpen) {
    closePanel();
    returnToRest();
    return;
  }

  if (isDaylight) {
    trackSun(east, west);
  }
}

int readLDR(int pin) {
  long total = 0;
  const int samples = 8;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(5);
  }
  return total / samples;
}

void trackSun(int east, int west) {
  int diff = west - east;
  if (abs(diff) < LDR_DEADBAND) return;

  int direction = (diff > 0) ? 1 : -1;

  if (direction > 0 && stepperPosition >= STEPS_FOR_180) return;
  if (direction < 0 && stepperPosition <= 0) return;

  int moveSteps = TRACK_STEP_SIZE;
  if (direction > 0) moveSteps = min(moveSteps, STEPS_FOR_180 - stepperPosition);
  else moveSteps = min(moveSteps, stepperPosition);

  stepper.step(direction * moveSteps);
  stepperPosition += direction * moveSteps;
}

void returnToRest() {
  int target = STEPS_FOR_180;
  while (stepperPosition != target) {
    int direction = (target > stepperPosition) ? 1 : -1;
    int moveSteps = min(TRACK_STEP_SIZE, abs(target - stepperPosition));
    stepper.step(direction * moveSteps);
    stepperPosition += direction * moveSteps;
  }
}

void openPanel() {
  servoLeft.write(SERVO_LEFT_OPEN_ANGLE);
  servoRight.write(SERVO_RIGHT_OPEN_ANGLE);
  panelOpen = true;
}

void closePanel() {
  servoLeft.write(SERVO_CLOSED_ANGLE);
  servoRight.write(SERVO_CLOSED_ANGLE);
  panelOpen = false;
}
