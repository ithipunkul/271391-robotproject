#include <Arduino.h>
#include <Bluepad32.h>
#include <ESP32Servo.h>

constexpr int MOTOR1_PIN = 18;
constexpr int MOTOR2_PIN = 15;
constexpr int STOP_US = 1500;
constexpr int DEADZONE = 72;
constexpr int MIN_SPEED_US = 150;
constexpr int MAX_SPEED_US = 500;
constexpr int SPEED_STEP_US = 50;
constexpr uint32_t UPDATE_MS = 20;
constexpr uint32_t GAMEPAD_TIMEOUT_MS = 600;

Servo motor1;
Servo motor2;
ControllerPtr gamepad = nullptr;

int speedUs = 200;  // เริ่มที่ 1300 / 1700 µs
uint32_t lastUpdateMs = 0;
uint32_t lastGamepadDataMs = 0;
bool waitingForNeutral = true;
bool previousUp = false;
bool previousDown = false;
bool previousOptions = false;

void stopMotors() {
  motor1.writeMicroseconds(STOP_US);
  motor2.writeMicroseconds(STOP_US);
}

bool sticksAreNeutral() {
  return abs(gamepad->axisX()) <= DEADZONE &&
         abs(gamepad->axisRY()) <= DEADZONE;
}

int pulseForStick(int axis) {
  if (abs(axis) <= DEADZONE) return STOP_US;
  return axis < 0 ? STOP_US - speedUs : STOP_US + speedUs;
}

void printSpeed() {
  Serial.printf("CCW %d us | stop %d us | CW %d us\n",
                STOP_US - speedUs, STOP_US, STOP_US + speedUs);
}

void onConnectedController(ControllerPtr controller) {
  if (!controller->isGamepad() || gamepad != nullptr) return;

  gamepad = controller;
  waitingForNeutral = true;
  lastGamepadDataMs = millis();
  previousUp = (gamepad->dpad() & DPAD_UP) != 0;
  previousDown = (gamepad->dpad() & DPAD_DOWN) != 0;
  previousOptions = gamepad->miscStart();
  stopMotors();
  Serial.println("Gamepad connected; center both sticks first");
}

void onDisconnectedController(ControllerPtr controller) {
  if (controller != gamepad) return;

  stopMotors();
  gamepad = nullptr;
  waitingForNeutral = true;
  Serial.println("Gamepad disconnected; motors stopped");
}

void setup() {
  Serial.begin(115200);

  motor1.setPeriodHertz(50);
  motor2.setPeriodHertz(50);
  motor1.attach(MOTOR1_PIN, 500, 2500);
  motor2.attach(MOTOR2_PIN, 500, 2500);
  stopMotors();

  BP32.setup(&onConnectedController, &onDisconnectedController);
  Serial.println("Servo-only Bluepad32 test ready");
  Serial.println("Left stick X: motor 1 | Right stick Y: motor 2");
  Serial.println("D-pad up/down: speed | Options: stop");
  printSpeed();
}

void loop() {
  const bool dataUpdated = BP32.update();

  if (gamepad == nullptr || !gamepad->isConnected()) {
    stopMotors();
    delay(5);
    return;
  }

  const uint32_t now = millis();
  if (dataUpdated) lastGamepadDataMs = now;

  if (now - lastGamepadDataMs > GAMEPAD_TIMEOUT_MS) {
    stopMotors();
    waitingForNeutral = true;
    delay(1);
    return;
  }

  if (now - lastUpdateMs < UPDATE_MS) {
    delay(1);
    return;
  }
  lastUpdateMs = now;

  const bool up = (gamepad->dpad() & DPAD_UP) != 0;
  const bool down = (gamepad->dpad() & DPAD_DOWN) != 0;
  const bool options = gamepad->miscStart();

  if (up && !previousUp) {
    speedUs = min(MAX_SPEED_US, speedUs + SPEED_STEP_US);
    printSpeed();
  }
  if (down && !previousDown) {
    speedUs = max(MIN_SPEED_US, speedUs - SPEED_STEP_US);
    printSpeed();
  }
  if (options && !previousOptions) {
    waitingForNeutral = true;
    stopMotors();
  }

  previousUp = up;
  previousDown = down;
  previousOptions = options;

  // ต้องคืนสติ๊กเข้ากลางหลังเชื่อมจอยหรือกด Options
  if (waitingForNeutral && sticksAreNeutral() && !options)
    waitingForNeutral = false;

  if (waitingForNeutral || options) {
    stopMotors();
    return;
  }

  motor1.writeMicroseconds(pulseForStick(gamepad->axisX()));
  motor2.writeMicroseconds(pulseForStick(gamepad->axisRY()));
}
