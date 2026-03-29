#define EN_PIN 8
#define X_STEP 2
#define X_DIR 5
#define Y_STEP 3
#define Y_DIR 6
#define LIMIT_BWD_L 9
#define LIMIT_BWD_R 10
#define LIMIT_FWD_L A2
#define LIMIT_FWD_R A1
#define BTN_A 11
#define BTN_B 12
#define RELAY_PIN 7
#define LED_RED 4
#define LED_GREEN A3
#define LED_YELLOW 13
#define NORMAL_DELAY 150
#define HOMING_DELAY 250
#define CLEAR_STEPS 1500
#define BLIND_STEPS 2000
#define PUMP_TIME 7000
#define PUMP_PULSE_ON 500
#define PUMP_PULSE_OFF 500
#define ESTOP_HOLD_TIME 2000

unsigned long dualPressStartTime = 0;
unsigned long pumpStartTime = 0;
int currentState = 0;
bool lastBtnAState = HIGH;
bool lastBtnBState = HIGH;

void setup() {
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);
  pinMode(X_STEP, OUTPUT); pinMode(X_DIR, OUTPUT);
  pinMode(Y_STEP, OUTPUT); pinMode(Y_DIR, OUTPUT);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(LIMIT_FWD_L, INPUT_PULLUP);
  pinMode(LIMIT_FWD_R, INPUT_PULLUP);
  pinMode(LIMIT_BWD_L, INPUT_PULLUP);
  pinMode(LIMIT_BWD_R, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  digitalWrite(LED_YELLOW, HIGH);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
}

void stepMotors(int speedDelay) {
  digitalWrite(X_STEP, HIGH); digitalWrite(Y_STEP, HIGH);
  delayMicroseconds(20);
  digitalWrite(X_STEP, LOW); digitalWrite(Y_STEP, LOW);
  delayMicroseconds(speedDelay);
}

void setDirectionForward() {
  digitalWrite(X_DIR, LOW);
  digitalWrite(Y_DIR, HIGH);
}

void setDirectionBackward() {
  digitalWrite(X_DIR, HIGH);
  digitalWrite(Y_DIR, LOW);
}

bool checkBackwardLimit() {
  if (digitalRead(LIMIT_BWD_L) == LOW || digitalRead(LIMIT_BWD_R) == LOW) {
    delayMicroseconds(200);
    if (digitalRead(LIMIT_BWD_L) == LOW || digitalRead(LIMIT_BWD_R) == LOW) return true;
  }
  return false;
}

bool checkForwardLimit() {
  if (digitalRead(LIMIT_FWD_L) == LOW || digitalRead(LIMIT_FWD_R) == LOW) {
    delayMicroseconds(200);
    if (digitalRead(LIMIT_FWD_L) == LOW || digitalRead(LIMIT_FWD_R) == LOW) return true;
  }
  return false;
}

void clearLimitSwitch() {
  setDirectionForward();
  while(checkBackwardLimit() == true) {
    if (digitalRead(BTN_A) == LOW && digitalRead(BTN_B) == LOW) return;
    stepMotors(HOMING_DELAY);
  }
  for(long i = 0; i < CLEAR_STEPS; i++) {
    if (digitalRead(BTN_A) == LOW && digitalRead(BTN_B) == LOW) return;
    stepMotors(NORMAL_DELAY);
  }
}

void loop() {
  bool currentBtnA = digitalRead(BTN_A);
  bool currentBtnB = digitalRead(BTN_B);

  if (currentBtnA == LOW && currentBtnB == LOW) {
    if (currentState < 99) {
      currentState = 99;
      dualPressStartTime = millis();
      digitalWrite(RELAY_PIN, LOW);
    }
  } else {
    if (currentState > 99) {
      currentState = 0;
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_YELLOW, HIGH);
    }
  }

  if (currentState == 0) {
    if (currentBtnA == LOW && lastBtnAState == HIGH) {
      delay(50);
      if (digitalRead(BTN_A) == LOW) currentState = 1;
    }
    else if (currentBtnB == LOW && lastBtnBState == HIGH) {
      delay(50);
      if (digitalRead(BTN_B) == LOW) currentState = 6;
    }
  }
  lastBtnAState = currentBtnA;
  lastBtnBState = currentBtnB;

  switch(currentState) {
    case 1:
      digitalWrite(LED_YELLOW, LOW); digitalWrite(LED_GREEN, LOW); digitalWrite(LED_RED, HIGH);
      if (checkBackwardLimit()) { currentState = 2; break; }
      setDirectionBackward(); stepMotors(HOMING_DELAY);
      break;

    case 2:
      clearLimitSwitch();
      delay(500);
      digitalWrite(LED_RED, LOW); digitalWrite(LED_GREEN, HIGH);
      pumpStartTime = millis();
      digitalWrite(RELAY_PIN, LOW);
      setDirectionForward();
      for(long i = 0; i < BLIND_STEPS; i++) {
        if (digitalRead(BTN_A) == LOW && digitalRead(BTN_B) == LOW) break;
        if (millis() - pumpStartTime >= PUMP_TIME) {
          unsigned long pulseElapsed = millis() - pumpStartTime - PUMP_TIME;
          if (pulseElapsed % (PUMP_PULSE_ON + PUMP_PULSE_OFF) < PUMP_PULSE_ON) {
            digitalWrite(RELAY_PIN, HIGH);
          } else {
            digitalWrite(RELAY_PIN, LOW);
          }
        }
        stepMotors(NORMAL_DELAY);
      }
      currentState = 3;
      break;

    case 3:
      if (millis() - pumpStartTime >= PUMP_TIME) {
        unsigned long pulseElapsed = millis() - pumpStartTime - PUMP_TIME;
        if (pulseElapsed % (PUMP_PULSE_ON + PUMP_PULSE_OFF) < PUMP_PULSE_ON) {
          digitalWrite(RELAY_PIN, HIGH);
        } else {
          digitalWrite(RELAY_PIN, LOW);
        }
      }
      if (checkForwardLimit()) {
        digitalWrite(RELAY_PIN, LOW);
        currentState = 4; break;
      }
      setDirectionForward(); stepMotors(NORMAL_DELAY);
      break;

    case 4:
      digitalWrite(LED_GREEN, LOW); digitalWrite(LED_RED, HIGH);
      if (checkBackwardLimit()) { currentState = 5; break; }
      setDirectionBackward(); stepMotors(NORMAL_DELAY);
      break;

    case 5:
      clearLimitSwitch();
      digitalWrite(LED_RED, LOW); digitalWrite(LED_YELLOW, HIGH);
      currentState = 0;
      break;

    case 6:
      digitalWrite(LED_YELLOW, LOW); digitalWrite(LED_GREEN, LOW); digitalWrite(LED_RED, HIGH);
      if (checkBackwardLimit()) { currentState = 7; break; }
      setDirectionBackward(); stepMotors(HOMING_DELAY);
      break;

    case 7:
      clearLimitSwitch();
      delay(500);
      digitalWrite(LED_RED, LOW); digitalWrite(LED_GREEN, HIGH);
      setDirectionForward();
      for(long i = 0; i < BLIND_STEPS; i++) {
        if (digitalRead(BTN_A) == LOW && digitalRead(BTN_B) == LOW) break;
        stepMotors(NORMAL_DELAY);
      }
      currentState = 8;
      break;

    case 8:
      if (checkForwardLimit()) { currentState = 9; break; }
      setDirectionForward(); stepMotors(NORMAL_DELAY);
      break;

    case 9:
      digitalWrite(LED_GREEN, LOW); digitalWrite(LED_RED, HIGH);
      if (checkBackwardLimit()) { currentState = 10; break; }
      setDirectionBackward(); stepMotors(NORMAL_DELAY);
      break;

    case 10:
      clearLimitSwitch();
      digitalWrite(LED_RED, LOW); digitalWrite(LED_YELLOW, HIGH);
      currentState = 0;
      break;

    case 99:
      if (millis() - dualPressStartTime < ESTOP_HOLD_TIME) {
        if ((millis() / 150) % 2 == 0) {
          digitalWrite(LED_RED, HIGH); digitalWrite(LED_GREEN, HIGH); digitalWrite(LED_YELLOW, HIGH);
        } else {
          digitalWrite(LED_RED, LOW); digitalWrite(LED_GREEN, LOW); digitalWrite(LED_YELLOW, LOW);
        }
      } else {
        if (currentBtnA == LOW && currentBtnB == LOW) {
          currentState = 100;
          digitalWrite(LED_RED, HIGH); digitalWrite(LED_GREEN, HIGH); digitalWrite(LED_YELLOW, HIGH);
        } else {
          currentState = 0;
          digitalWrite(LED_RED, LOW); digitalWrite(LED_GREEN, LOW); digitalWrite(LED_YELLOW, HIGH);
        }
      }
      break;

    case 100:
      if (checkBackwardLimit()) {
        currentState = 101;
        break;
      }
      setDirectionBackward();
      stepMotors(HOMING_DELAY);
      break;

    case 101:
      clearLimitSwitch();
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_YELLOW, HIGH);
      currentState = 102;
      break;

    case 102:
      break;
  }
}