#include <Arduino.h>
#include <Bluepad32.h>

// =====================================================
// PS4 Controller
// =====================================================

ControllerPtr controller = nullptr;

unsigned long lastControllerData = 0;
const unsigned long FAILSAFE_TIME = 500;

const int JOYSTICK_DEADZONE = 60;

bool lastCrossState = false;

// ความเร็วช้า / ปานกลาง
int speedMode = 1;   // เริ่มต้นที่ความเร็วช้า
float speedMultiplier = 0.35f;

const float SPEED_SLOW = 0.35f;
const float SPEED_MEDIUM = 0.65f;


// =====================================================
// Cytron MDD3A
//
// MDD3A ใช้ PWM 2 ขาต่อมอเตอร์ 1 ตัว
// PWM1A/PWM1B = มอเตอร์ซ้าย
// PWM2A/PWM2B = มอเตอร์ขวา
// =====================================================

// ล้อซ้าย
const int M1A_PIN = 25;
const int M1B_PIN = 26;

// ล้อขวา
const int M2A_PIN = 32;
const int M2B_PIN = 33;


// =====================================================
// PWM
// =====================================================

const int PWM_FREQ = 5000;
const int PWM_RES = 8;  // duty: 0 ถึง 255

const int M1A_CH = 0;
const int M1B_CH = 1;
const int M2A_CH = 2;
const int M2B_CH = 3;


// =====================================================
// ส่ง PWM ไปมอเตอร์หนึ่งตัว
//
// speed: -1.0 = ถอยหลังเต็ม
//         0.0 = หยุด
//         1.0 = เดินหน้าเต็ม
// =====================================================

void setMotor(int forwardChannel, int reverseChannel, float speed) {
  speed = constrain(speed, -1.0f, 1.0f);

  int duty = (int)(abs(speed) * 255.0f);

  if (speed > 0.0f) {
    ledcWrite(forwardChannel, duty);
    ledcWrite(reverseChannel, 0);
  }
  else if (speed < 0.0f) {
    ledcWrite(forwardChannel, 0);
    ledcWrite(reverseChannel, duty);
  }
  else {
    ledcWrite(forwardChannel, 0);
    ledcWrite(reverseChannel, 0);
  }
}


// =====================================================
// Differential Drive
// =====================================================

void driveMotors(float leftSpeed, float rightSpeed) {
  leftSpeed = constrain(leftSpeed, -1.0f, 1.0f);
  rightSpeed = constrain(rightSpeed, -1.0f, 1.0f);

  setMotor(M1A_CH, M1B_CH, leftSpeed);
  setMotor(M2A_CH, M2B_CH, rightSpeed);
}


void stopRobot() {
  driveMotors(0.0f, 0.0f);
}


// =====================================================
// Deadzone joystick
// =====================================================

float applyDeadzone(int value, int deadzone) {
  if (abs(value) <= deadzone) {
    return 0.0f;
  }

  if (value > 0) {
    return (float)(value - deadzone) / (512.0f - deadzone);
  }

  return (float)(value + deadzone) / (512.0f - deadzone);
}


// =====================================================
// Bluepad32 callbacks
// =====================================================

void onConnectedController(ControllerPtr ctl) {
  if (controller == nullptr) {
    controller = ctl;
    lastControllerData = millis();

    Serial.println();
    Serial.println("PS4 controller connected");
    Serial.print("Model: ");
    Serial.println(ctl->getModelName());
  }
}


void onDisconnectedController(ControllerPtr ctl) {
  if (controller == ctl) {
    controller = nullptr;
    stopRobot();

    Serial.println();
    Serial.println("PS4 controller disconnected");
  }
}


// =====================================================
// อ่านจอย PS4
// =====================================================

void processController() {
  if (controller == nullptr ||
      !controller->isConnected() ||
      !controller->isGamepad()) {
    stopRobot();
    return;
  }

  if (!controller->hasData()) {
    return;
  }

  lastControllerData = millis();

  // ---------------------------------------------------
  // ปุ่ม X: สลับความเร็ว ช้า <-> ปานกลาง
  // Bluepad32: a() มักตรงกับ Cross/X
  // ---------------------------------------------------

  bool crossState = controller->a();

  if (crossState && !lastCrossState) {
    if (speedMode == 1) {
      speedMode = 2;
      speedMultiplier = SPEED_MEDIUM;

      Serial.println("Speed: MEDIUM");
    }
    else {
      speedMode = 1;
      speedMultiplier = SPEED_SLOW;

      Serial.println("Speed: SLOW");
    }
  }

  lastCrossState = crossState;

  // ---------------------------------------------------
  // ก้านซ้าย Y: เดินหน้า/ถอยหลัง
  // ก้านขวา X: เลี้ยว
  // ---------------------------------------------------

  float throttle = applyDeadzone(
    controller->axisY(),
    JOYSTICK_DEADZONE
  );

  float steering = -applyDeadzone(
    controller->axisRX(),
    JOYSTICK_DEADZONE
  );

  // จำกัดความเร็วตามโหมดที่เลือก
  throttle *= speedMultiplier;
  steering *= speedMultiplier;

  // Differential mixing
  float leftMotor = throttle + steering;
  float rightMotor = throttle - steering;

  // ป้องกันไม่ให้เกิน -1 ถึง +1
  float maxValue = max(abs(leftMotor), abs(rightMotor));

  if (maxValue > 1.0f) {
    leftMotor /= maxValue;
    rightMotor /= maxValue;
  }

  driveMotors(leftMotor, rightMotor);

  // แสดงผลใน Serial Monitor
  static unsigned long lastPrint = 0;

  if (millis() - lastPrint >= 200) {
    lastPrint = millis();

    Serial.print("Mode: ");
    Serial.print(speedMode == 1 ? "SLOW" : "MEDIUM");

    Serial.print(" | Left: ");
    Serial.print(leftMotor, 2);

    Serial.print(" | Right: ");
    Serial.println(rightMotor, 2);
  }
}


// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("PS4 Differential Drive Robot");
  Serial.println("ESP32 + Cytron MDD3A");
  Serial.println("================================");

  // ทำให้ทุกขาเป็น LOW ก่อน เพื่อไม่ให้ล้อหมุนเอง
  pinMode(M1A_PIN, OUTPUT);
  pinMode(M1B_PIN, OUTPUT);
  pinMode(M2A_PIN, OUTPUT);
  pinMode(M2B_PIN, OUTPUT);

  digitalWrite(M1A_PIN, LOW);
  digitalWrite(M1B_PIN, LOW);
  digitalWrite(M2A_PIN, LOW);
  digitalWrite(M2B_PIN, LOW);

  // สร้าง PWM 4 ช่อง
  ledcSetup(M1A_CH, PWM_FREQ, PWM_RES);
  ledcSetup(M1B_CH, PWM_FREQ, PWM_RES);
  ledcSetup(M2A_CH, PWM_FREQ, PWM_RES);
  ledcSetup(M2B_CH, PWM_FREQ, PWM_RES);

  ledcAttachPin(M1A_PIN, M1A_CH);
  ledcAttachPin(M1B_PIN, M1B_CH);
  ledcAttachPin(M2A_PIN, M2A_CH);
  ledcAttachPin(M2B_PIN, M2B_CH);

  // ยืนยันว่าเริ่มต้นด้วยการหยุด
  stopRobot();

  // Bluetooth gamepad
  BP32.setup(
    &onConnectedController,
    &onDisconnectedController
  );

  Serial.println("Ready. Connect PS4 controller.");
  Serial.println("X button: SLOW <-> MEDIUM");
}


// =====================================================
// Loop
// =====================================================

void loop() {
  BP32.update();

  processController();

  // Failsafe: จอยหลุดหรือไม่มีข้อมูล -> หยุด
  if (millis() - lastControllerData > FAILSAFE_TIME) {
    stopRobot();
  }

  delay(10);
}
