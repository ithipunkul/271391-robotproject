#include <Arduino.h>
#include <ESP32Servo.h>

#define SERVO1_PIN 18
#define SERVO2_PIN 15

Servo servo1;
Servo servo2;

const int STOP_PULSE = 1500;

// เริ่มต้นความเร็ว 50% (CCW = 1300, CW = 1700)
int ccwPulse = 1300; 
int cwPulse  = 1700;

// Dynamic Timeout สำหรับแก้ปัญหากดค้างแล้วกระตุก
const unsigned long FIRST_PRESS_TIMEOUT = 260;
const unsigned long HOLD_TIMEOUT        = 100;

// ตัวแปรติดตามสถานะ มอเตอร์ 1
unsigned long lastCmdTime1 = 0;
bool isMoving1 = false;
bool isFirstPress1 = true;

// ตัวแปรติดตามสถานะ มอเตอร์ 2
unsigned long lastCmdTime2 = 0;
bool isMoving2 = false;
bool isFirstPress2 = true;

void printCurrentPulse() {
  Serial.print(">> [ระดับความเร็วปัจจุบัน] CCW: ");
  Serial.print(ccwPulse);
  Serial.print(" µs | CW: ");
  Serial.print(cwPulse);
  Serial.println(" µs");
}

void setup() {
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);

  servo1.setPeriodHertz(50);
  servo1.attach(SERVO1_PIN, 500, 2500);
  servo1.writeMicroseconds(STOP_PULSE);

  servo2.setPeriodHertz(50);
  servo2.attach(SERVO2_PIN, 500, 2500);
  servo2.writeMicroseconds(STOP_PULSE);

  Serial.println("==========================================");
  Serial.println("มอเตอร์ 1 (Pin 18): 'a' = ทวนเข็ม | 'd' = ตามเข็ม");
  Serial.println("มอเตอร์ 2 (Pin 15): 'w' = ทวนเข็ม | 's' = ตามเข็ม");
  Serial.println("กด 'q' -> ลดความเร็ว (ช้าลงทั้ง 2 ทิศทาง)");
  Serial.println("กด 'e' -> เพิ่มความเร็ว (เร็วขึ้นทั้ง 2 ทิศทาง)");
  Serial.println("==========================================");
  printCurrentPulse();
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    if (cmd != '\r' && cmd != '\n') {
      
      // --- ปุ่ม Q: ลดความเร็วลง (จำกัดขอบเขตช้าสุดที่ 1350 และ 1650 เพื่อให้มอเตอร์ยังมีแรงหมุน) ---
      if (cmd == 'q' || cmd == 'Q') {
        ccwPulse = constrain(ccwPulse + 100, 1000, 1350); 
        cwPulse  = constrain(cwPulse - 100, 1650, 2000); 
        Serial.print("Action: ลดความเร็ว -> ");
        printCurrentPulse();
      }
      // --- ปุ่ม E: เพิ่มความเร็วขึ้น ---
      else if (cmd == 'e' || cmd == 'E') {
        ccwPulse = constrain(ccwPulse - 100, 1000, 1350); 
        cwPulse  = constrain(cwPulse + 100, 1650, 2000); 
        Serial.print("Action: เพิ่มความเร็ว -> ");
        printCurrentPulse();
      }

      // --- ควบคุม มอเตอร์ 1 (Pin 18) ---
      if (cmd == 'a' || cmd == 'A') {
        servo1.writeMicroseconds(ccwPulse);
        lastCmdTime1 = millis();
        isMoving1 = true;
      } 
      else if (cmd == 'd' || cmd == 'D') {
        servo1.writeMicroseconds(cwPulse);
        lastCmdTime1 = millis();
        isMoving1 = true;
      }

      // --- ควบคุม มอเตอร์ 2 (Pin 15) ---
      if (cmd == 'w' || cmd == 'W') {
        servo2.writeMicroseconds(ccwPulse);
        lastCmdTime2 = millis();
        isMoving2 = true;
      } 
      else if (cmd == 's' || cmd == 'S') {
        servo2.writeMicroseconds(cwPulse);
        lastCmdTime2 = millis();
        isMoving2 = true;
      }
    }
  }

  // จัดการ Timeout มอเตอร์ 1
  if (isMoving1) {
    unsigned long timeout1 = isFirstPress1 ? FIRST_PRESS_TIMEOUT : HOLD_TIMEOUT;
    if (millis() - lastCmdTime1 > timeout1) {
      servo1.writeMicroseconds(STOP_PULSE);
      isMoving1 = false;
      isFirstPress1 = true;
    } 
    else if (millis() - lastCmdTime1 > 50) {
      isFirstPress1 = false;
    }
  }

  // จัดการ Timeout มอเตอร์ 2
  if (isMoving2) {
    unsigned long timeout2 = isFirstPress2 ? FIRST_PRESS_TIMEOUT : HOLD_TIMEOUT;
    if (millis() - lastCmdTime2 > timeout2) {
      servo2.writeMicroseconds(STOP_PULSE);
      isMoving2 = false;
      isFirstPress2 = true;
    } 
    else if (millis() - lastCmdTime2 > 50) {
      isFirstPress2 = false;
    }
  }
}