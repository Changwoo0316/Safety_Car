#include <Wire.h>
#include <Servo.h>

/*
  MPU6050 Impact Detection (Arduino MEGA 2560 Version)
  - Hardware Serial 사용으로 통신 충돌 해결
  
  [배선 정보]
  - STM32   : Serial1 (RX=19, TX=18)
  - Bluetooth: Serial2 (RX=17, TX=16)
  - MPU6050 : SDA=20, SCL=21 (메가는 SDA/SCL 핀이 따로 있습니다!)
*/

char recvId[10] = "LCW_SQL"; // SQL 저장 클라이언트 ID
char sendBuf[60]; // sprintf로 보낼 버퍼

// ===== 서보 모터 핀 설정 =====
// const uint8_t PIN_SERVO = 5;  // [삭제] LED와 충돌나므로 아래 attach(4)를 따름
const uint8_t PIN_TRIG  = 9;   // 초음파 TRIG (사용 안함)
const uint8_t PIN_ECHO  = 8;   // 초음파 ECHO (사용 안함)

// ==== RGB 핀 ====
int redpin = 5;   // Red LED
int bluepin = 6;  // Blue LED
int greenpin = 9; // Green LED

Servo myServo;

const uint8_t MPU_ADDR = 0x68;   // 대부분 0x68 (AD0가 HIGH면 0x69)
int16_t AcX, AcY, AcZ;
int16_t prevAcX = 0, prevAcY = 0, prevAcZ = 0;

// ===== 충돌 임계값(ΔRAW 기준) =====
const int16_t IMPACT_DACX = 15000;
const int16_t IMPACT_DACY = 15000;

// ===== 연속 초과 판정 =====
const uint8_t NEED_CONSECUTIVE = 3;
static uint8_t overCntX = 0;
static uint8_t overCntY = 0;

// ===== 쿨다운 =====
const uint16_t COOLDOWN_MS = 1000;
uint32_t lastImpactMs = 0;

// LED 제어용 함수
void setRGB(int r, int g, int b) {
  analogWrite(redpin, r);
  analogWrite(greenpin, g);
  analogWrite(bluepin, b);
}

void mpuWake() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);
}

bool mpuReadAccel(int16_t &x, int16_t &y, int16_t &z) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;

  uint8_t n = Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6);
  if (n != 6) return false;

  int16_t rx = (Wire.read() << 8) | Wire.read();
  int16_t ry = (Wire.read() << 8) | Wire.read();
  int16_t rz = (Wire.read() << 8) | Wire.read();

  x = rx; y = ry; z = rz;
  return true;
}

void setup() {
  // 1. PC 디버깅용
  Serial.begin(115200);
  
  // 2. STM32 통신용 (RX:19, TX:18)
  Serial1.begin(9600);
  
  // 3. 블루투스 통신용 (RX:17, TX:16)
  Serial2.begin(9600);

  Wire.begin(); // 메가는 SDA(20), SCL(21)

  pinMode(redpin, OUTPUT);
  pinMode(bluepin, OUTPUT);
  pinMode(greenpin, OUTPUT);

  // [수정] 핀 충돌 방지를 위해 4번 핀 사용
  myServo.attach(4); 
  myServo.write(0); // 초기화

  mpuWake();

  // MPU 확인
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x75);
  if (Wire.endTransmission(false) == 0) {
    Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)1);
    if (Wire.available()) {
      uint8_t who = Wire.read();
      Serial.print("MPU6050 WHO_AM_I = 0x");
      Serial.println(who, HEX);
    }
  }

  Serial.println("Arduino Mega Ready. (Serial1: STM32, Serial2: BT)");
}

void loop() {

  // ==========================================================
  // 1. STM32에서 데이터 수신 (Serial1 사용)
  // ==========================================================
  if(Serial1.available() > 0){
    uint8_t driveDirection = Serial1.read();
    
    // 디버깅용 (PC로 확인)
    Serial.print("STM32 Received: ");
    Serial.println(driveDirection, HEX);

    // 오른쪽 주행 (왼쪽 장애물)
    if(driveDirection == 0x0F){
      myServo.write(180); // 안전벨트

      // 블루투스 전송 (Serial2 사용)
      sprintf(sendBuf,"[%s]2@0@0@R\r\n",recvId); 
      Serial2.write(sendBuf);

      setRGB(255, 255, 0); // Yellow
    }
    // 왼쪽 주행 (오른쪽 장애물)
    else if(driveDirection == 0xF0) {
      myServo.write(180); // 안전벨트

      // 블루투스 전송 (Serial2 사용)
      sprintf(sendBuf,"[%s]2@0@0@L\r\n",recvId); 
      Serial2.write(sendBuf);

      setRGB(255, 255, 0); // Yellow
    }
    // 정상 주행 
    else{
      myServo.write(0);
      setRGB(0, 255, 0); // Green
    }
  }

  // ==========================================================
  // 2. MPU6050 충격 감지
  // ==========================================================
  const uint32_t now = millis();
  delay(10); // 100Hz

  if (!mpuReadAccel(AcX, AcY, AcZ)) {
    return;
  }

  // 변화량(|Δ|) 계산
  const int16_t dAcX = abs(AcX - prevAcX);
  const int16_t dAcY = abs(AcY - prevAcY);
  const int16_t dAcZ = abs(AcZ - prevAcZ);

  prevAcX = AcX;
  prevAcY = AcY;
  prevAcZ = AcZ;

  if (now - lastImpactMs < COOLDOWN_MS) {
    overCntX = 0;
    overCntY = 0;
    return;
  }

  // 연속 초과 카운트
  if (dAcX > IMPACT_DACX) { if (overCntX < 10) overCntX++; } else { overCntX = 0; }
  if (dAcY > IMPACT_DACY) { if (overCntY < 10) overCntY++; } else { overCntY = 0; }

  // 충돌 확정
  const bool impact = (overCntX >= NEED_CONSECUTIVE) || (overCntY >= NEED_CONSECUTIVE);

  if (impact) {
    // 충돌 발생!
    
    myServo.write(180); // 안전벨트
    setRGB(255, 0, 0);  // Red

    // STM32에게 충돌 알림 (Serial1 사용)
    uint8_t collision = 0xFF;
    Serial1.write(collision);
    
    lastImpactMs = now;

    // 비교용 (가장 큰 축 찾기)
    char axis = 'X';
    int16_t maxDelta = dAcX;
    if (dAcY > maxDelta) { maxDelta = dAcY; axis = 'Y'; }
    if (dAcZ > maxDelta) { maxDelta = dAcZ; axis = 'Z'; }

    // 블루투스 전송 (Serial2 사용)
    sprintf(sendBuf,"[%s]3@%c@%d@0\r\n",recvId,axis,maxDelta); 
    Serial2.write(sendBuf);
      
    // PC 디버깅 출력
    Serial.print("IMPACT DETECTED! Max Delta: ");
    Serial.println(maxDelta);

    overCntX = 0;
    overCntY = 0;
  }
}