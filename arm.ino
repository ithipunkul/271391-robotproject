#include <Bluepad32.h>
#include <ESP32Servo.h> // เปลี่ยนเป็น ESP32Servo สำหรับ ESP32

#define SERVO1_PIN 21   // ปรับ Pin ให้ตรงตามวงจรจริง
#define SERVO2_PIN 22

// กำหนด Target MAC Address ที่อนุญาตให้เชื่อมต่อเท่านั้น
const uint8_t ALLOWED_MAC[6] = {0x41, 0x42, 0x0F, 0x8E, 0x60, 0x81};

Servo servo1;
Servo servo2;

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// ฟังก์ชันสำหรับเปรียบเทียบ MAC Address
bool isAllowedMAC(const uint8_t* mac) {
    for (int i = 0; i < 6; i++) {
        if (mac[i] != ALLOWED_MAC[i]) {
            return false; // MAC ไม่ตรง
        }
    }
    return true; // MAC ตรงกัน
}

void onConnectedController(ControllerPtr ctl) {
    ControllerProperties properties = ctl->getProperties();
    
    // ตรวจสอบ MAC Address ของอุปกรณ์ที่พยายามเชื่อมต่อเข้ามา
    if (!isAllowedMAC(properties.btaddr)) {
        Serial.printf("REJECTED: Unauthorized Controller MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                      properties.btaddr[0], properties.btaddr[1], properties.btaddr[2],
                      properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]);
        
        // ตัดการเชื่อมต่อทันทีหาก MAC Address ไม่ตรง
        ctl->disconnect();
        return;
    }

    // หาก MAC Address ตรงกัน จะบันทึกการเชื่อมต่อ
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            Serial.printf("ALLOWED: Controller connected, index=%d\n", i);
            Serial.printf("Controller BT Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                          properties.btaddr[0], properties.btaddr[1], properties.btaddr[2],
                          properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]);
            
            myControllers[i] = ctl;
            foundEmptySlot = true;
            break;
        }
    }
    if (!foundEmptySlot) {
        Serial.println("CALLBACK: Controller connected, but could not find empty slot");
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr;
            break;
        }
    }
}

void processGamepad(ControllerPtr ctl) {
    uint8_t dpad = ctl->dpad();
    
    // --- ควบคุม Servo 2 ผ่าน D-Pad UP (0x01) และ DOWN (0x02) ---
    if (dpad & 0x01) {       // UP: มอเตอร์ 2 หมุนทวนเข็ม
        Serial.println("DPAD UP Pressed -> Servo 2 Counter-Clockwise");
        servo2.write(70);
    } 
    else if (dpad & 0x02) { // DOWN: มอเตอร์ 2 หมุนตามเข็ม
        Serial.println("DPAD DOWN Pressed -> Servo 2 Clockwise");
        servo2.write(110);
    } 
    else {                  // ปล่อยปุ่ม: หยุดหมุน (90)
        servo2.write(90);
    }

    // --- ควบคุม Servo 1 ผ่าน D-Pad LEFT (0x08) และ RIGHT (0x04) ---
    if (dpad & 0x08) {       // LEFT: มอเตอร์ 1 หมุนทวนเข็ม
        Serial.println("DPAD LEFT Pressed -> Servo 1 Counter-Clockwise");
        servo1.write(70);
    } 
    else if (dpad & 0x04) { // RIGHT: มอเตอร์ 1 หมุนตามเข็ม
        Serial.println("DPAD RIGHT Pressed -> Servo 1 Clockwise");
        servo1.write(125);
    } 
    else {                  // ปล่อยปุ่ม: หยุดหมุน (90)
        servo1.write(90);
    }
}

void processControllers() {
    for (auto ctl : myControllers) {
        if (ctl && ctl->isConnected() && ctl->hasData()) {
            if (ctl->isGamepad()) {
                processGamepad(ctl);
            }
        }
    }
}

void setup() {
    Serial.begin(115200);

    // จัดสรร Timer สำหรับ ESP32Servo
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    
    servo1.setPeriodHertz(50);
    servo2.setPeriodHertz(50);
    
    servo1.attach(SERVO1_PIN, 500, 2400); 
    servo2.attach(SERVO2_PIN, 500, 2400); 

    servo1.write(90); // หยุดนิ่ง
    servo2.write(90);

    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.enableVirtualDevice(false);
}

void loop() {
    bool dataAvailable = BP32.update();
    if (dataAvailable) {
        processControllers();
    }
    delay(15);
}