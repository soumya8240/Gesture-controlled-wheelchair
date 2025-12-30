#include <SoftwareSerial.h>

/* ================= CONFIG ================= */
#define BT_RX 10
#define BT_TX 11

#define ENA 5
#define IN1 6
#define IN2 7
#define ENB 9
#define IN3 8
#define IN4 12

#define BUZZER 4
#define ENABLE_BTN 2

#define PACKET_SIZE 9
#define CONTROL_HZ 50
#define CONTROL_DT_MS 20
#define PACKET_TIMEOUT 200

#define MAX_PWM 255
#define ACC_STEP 5
/* ========================================== */

SoftwareSerial bt(BT_RX, BT_TX);

/* ---------- Packet ---------- */
uint8_t buf[PACKET_SIZE];
uint8_t bufIdx = 0;

uint16_t seq = 0, lastSeq = 0;
bool seqSynced = false;   // ✅ FIX 1: sequence sync flag

int16_t gyroX, gyroY, gyroZ;
uint8_t rxCRC;

/* ---------- State ---------- */
bool enabled = false;
bool fault = false;

unsigned long lastPacketTime = 0;
unsigned long lastControlTime = 0;

/* ---------- Velocity ---------- */
int targetLeft = 0, targetRight = 0;
int currentLeft = 0, currentRight = 0;

/* ---------- CRC ---------- */
uint8_t crc8(uint8_t *d, uint8_t n) {
  uint8_t c = 0;
  for (uint8_t i = 0; i < n; i++) c ^= d[i];
  return c;
}

/* ---------- Motor ---------- */
void writeMotors(int l, int r) {
  l = constrain(l, -MAX_PWM, MAX_PWM);
  r = constrain(r, -MAX_PWM, MAX_PWM);

  digitalWrite(IN1, l >= 0);
  digitalWrite(IN2, l < 0);
  analogWrite(ENA, abs(l));

  digitalWrite(IN3, r >= 0);
  digitalWrite(IN4, r < 0);
  analogWrite(ENB, abs(r));
}

void hardStop() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

/* ---------- Buzzer ---------- */
void buzzer(bool on) {
  digitalWrite(BUZZER, on);
}

/* ---------- Enable Button ---------- */
void handleEnableButton() {
  static bool last = HIGH;
  bool now = digitalRead(ENABLE_BTN);

  if (last == HIGH && now == LOW) {
    enabled = !enabled;
    buzzer(true);
    delay(80);
    buzzer(false);
  }
  last = now;
}

/* ---------- Packet Processing ---------- */
void processPacket() {
  seq   = buf[0] | (buf[1] << 8);
  gyroX = buf[2] | (buf[3] << 8);
  gyroY = buf[4] | (buf[5] << 8);
  gyroZ = buf[6] | (buf[7] << 8);
  rxCRC = buf[8];

  // CRC check
  if (crc8(buf, 8) != rxCRC) {
    fault = true;
    return;
  }

  // ✅ FIX 1: Accept first packet unconditionally
  if (!seqSynced) {
    lastSeq = seq;
    seqSynced = true;
    lastPacketTime = millis();
  } else {
    if (seq != lastSeq + 1) {
      fault = true;
      lastSeq = seq; // resync
      return;
    }
    lastSeq = seq;
    lastPacketTime = millis();
  }

  if (!enabled || fault) return;

  /* ---------- Motion Mapping ---------- */
  int pitch = constrain(gyroY / 150, -MAX_PWM, MAX_PWM);
  int roll  = constrain(gyroX / 150, -MAX_PWM, MAX_PWM);

  targetLeft  = pitch - roll;
  targetRight = pitch + roll;

  /* ---------- FALL DETECTION (DISABLED) ----------
     gyroZ is angular velocity, not accel Z.
     This logic is intentionally commented out.

  if (gyroZ < SOME_THRESHOLD) {
    fault = true;
  }
  ----------------------------------------------- */
}

/* ---------- Smooth Control Loop ---------- */
void updateVelocity() {
  if (fault || !enabled) {
    currentLeft = 0;
    currentRight = 0;
    hardStop();
    buzzer(true);
    return;
  }

  buzzer(false);

  if (currentLeft < targetLeft)
    currentLeft = min(currentLeft + ACC_STEP, targetLeft);
  else
    currentLeft = max(currentLeft - ACC_STEP, targetLeft);

  if (currentRight < targetRight)
    currentRight = min(currentRight + ACC_STEP, targetRight);
  else
    currentRight = max(currentRight - ACC_STEP, targetRight);

  writeMotors(currentLeft, currentRight);
}

/* ---------- SETUP ---------- */
void setup() {
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(ENABLE_BTN, INPUT_PULLUP);

  hardStop();
  bt.begin(9600);
}

/* ---------- LOOP ---------- */
void loop() {
  handleEnableButton();

  while (bt.available()) {
    buf[bufIdx++] = bt.read();
    if (bufIdx == PACKET_SIZE) {
      bufIdx = 0;
      processPacket();
    }
  }

  if (millis() - lastPacketTime > PACKET_TIMEOUT) {
    fault = true;
  }

  if (millis() - lastControlTime >= CONTROL_DT_MS) {
    lastControlTime = millis();
    updateVelocity();
  }
}
