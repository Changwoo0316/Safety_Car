#include <Arduino.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// ===== 핀 설정 =====
const uint8_t PIN_SERVO =                           5;   // 서보 SIGNAL
const uint8_t PIN_TRIG  = 9;   // 초음파 TRIG
const uint8_t PIN_ECHO  = 8;   // 초음파 ECHO (다르면 여기만 수정)

// ===== SoftwareSerial 핀 =====
const uint8_t PIN_SW_RX = 2;
const uint8_t PIN_SW_TX = 3;

// ===== 서보 각도 (안전 범위) =====
const int ANG_CENTER = 90;
const int ANG_LEFT   = 120;
const int ANG_RIGHT  = 60;

// ===== 파라미터 =====
const uint16_t MAX_CM = 200;          // 측정 최대거리(타임아웃)
const uint16_t SERVO_SETTLE_MS = 250; // 서보 이동 후 안정화
const uint16_t GUARD_MS = 15;         // 초음파 잔향 방지 딜레이
uint8_t frontViewFlag = 0xFF;

Servo scanServo;SoftwareSerial mySerial(PIN_SW_RX, PIN_SW_TX);

// ---- 초음파 1회 측정(타임아웃 포함) ----
static uint16_t ultrasonicOnceCm() {
  // Trig pulse
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // timeout(us) = MAX_CM * 58us
  unsigned long timeoutUs = (unsigned long)MAX_CM * 58UL;
  unsigned long dur = pulseIn(PIN_ECHO, HIGH, timeoutUs);

  if (dur == 0) return MAX_CM + 1;    // Echo 못 받으면 멀다고 처리
  return (uint16_t)(dur / 58UL);      // cm 변환
}

// ---- 중앙값 필터(3회)로 튐 제거 ----
static uint16_t median3(uint16_t a, uint16_t b, uint16_t c) {
  if (a > b) { uint16_t t=a; a=b; b=t; }
  if (b > c) { uint16_t t=b; b=c; c=t; }
  if (a > b) { uint16_t t=a; a=b; b=t; }
  return b;
}

static uint16_t ultrasonicFilteredCm() {
  uint16_t v1 = ultrasonicOnceCm(); delay(GUARD_MS);
  uint16_t v2 = ultrasonicOnceCm(); delay(GUARD_MS);
  uint16_t v3 = ultrasonicOnceCm(); delay(GUARD_MS);
  return median3(v1, v2, v3);
}

// ---- 특정 각도로 보고 거리 측정 ----
static uint16_t scanAt(int angle) {
  scanServo.write(angle);
  delay(SERVO_SETTLE_MS);
  return ultrasonicFilteredCm();
}

// ---- 방향 문자열 ----
static const char* dirNameMin(uint16_t dL, uint16_t dC, uint16_t dR) {
  // 가장 가까운(위험) 방향
  if (dL <= dC && dL <= dR) return "LEFT (closest)";
  if (dC <= dL && dC <= dR) return "CENTER (closest)";
  return "RIGHT (closest)";
}

static const char* dirNameMax(uint16_t dL, uint16_t dC, uint16_t dR) {
  // 가장 먼(통과 가능) 방향
  if (dL >= dC && dL >= dR) return "LEFT (farthest)";
  if (dC >= dL && dC >= dR) return "CENTER (farthest)";
  return "RIGHT (farthest)";
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  scanServo.attach(PIN_SERVO);
  scanServo.write(ANG_CENTER);
  delay(500);

  //Serial.println("=== Servo + Ultrasonic Scan (noL/C/R) ===");
  //Serial.println("Open Serial Monitor @115200");
}

void loop() {

  if(mySerial.available() > 0){
    frontViewFlag = mySerial.read();
    Serial.print("frontViewFlag: ");
    Serial.println(frontViewFlag);
  }

  if(frontViewFlag == 0xFF){
    uint16_t dC = scanAt(ANG_CENTER);

    mySerial.write((uint8_t)dC);
    Serial.print("dC: ");
    Serial.println(dC);
    //Serial.println("cm");

  }

  else{


    // 1) CENTER
    uint8_t dC = scanAt(ANG_CENTER);

    // 2) LEFT
    uint8_t dL = scanAt(ANG_LEFT);

    // 3) RIGHT
    uint8_t dR = scanAt(ANG_RIGHT);

    // 4) CENTER 복귀
    scanServo.write(ANG_CENTER);

    // 1. 보낼 배열 생성 (총 4바이트)
    uint8_t packet[4];

    // 2. 패킷 구성
    packet[0] = 0xFF;           // 헤더 (Start Byte)   
    packet[1] = (uint8_t)dL;    // 왼쪽 거리
    packet[2] = (uint8_t)dC;    // 중앙 거리
    packet[3] = (uint8_t)dR;    // 오른쪽 거리

    // 3. 바이너리 전송 (Serial.print 아님!)
    mySerial.write(packet, 4);
    // 출력/판단
    Serial.print("L="); Serial.print(dL);
    Serial.print("cm  C="); Serial.print(dC);
    Serial.print("cm  R="); Serial.print(dR);
    Serial.println("cm");

    //Serial.print("Danger dir : ");
    //Serial.println(dirNameMin(dL, dC, dR));

    //Serial.print("Safe dir   : ");
    //Serial.println(dirNameMax(dL, dC, dR));
  }


  //Serial.println("-----------------------");
}

