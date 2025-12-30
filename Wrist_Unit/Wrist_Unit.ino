#include <Wire.h>
#include <SoftwareSerial.h>

// ---------------- CONFIG ----------------
#define MPU_ADDR 0x68
#define TX_RATE_MS 20      // 50 Hz
#define BT_RX 10
#define BT_TX 11
// ---------------------------------------

SoftwareSerial bt(BT_RX, BT_TX);

// Sequence counter
uint16_t seq = 0;
unsigned long lastTxTime = 0;

// Raw gyro values
int16_t gyroX, gyroY, gyroZ;

// -------- CRC (simple XOR) --------
uint8_t computeCRC(uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
  }
  return crc;
}

// -------- MPU6050 Init --------
void initMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);     // Power management
  Wire.write(0x00);     // Wake up MPU6050
  Wire.endTransmission(true);
}

// -------- Read Raw Gyro --------
void readGyro() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x43); // Gyro start register
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  gyroX = (Wire.read() << 8) | Wire.read();
  gyroY = (Wire.read() << 8) | Wire.read();
  gyroZ = (Wire.read() << 8) | Wire.read();
}

// -------- Send Packet --------
void sendPacket() {
  uint8_t packet[9];

  packet[0] = seq & 0xFF;
  packet[1] = (seq >> 8) & 0xFF;

  packet[2] = gyroX & 0xFF;
  packet[3] = (gyroX >> 8) & 0xFF;

  packet[4] = gyroY & 0xFF;
  packet[5] = (gyroY >> 8) & 0xFF;

  packet[6] = gyroZ & 0xFF;
  packet[7] = (gyroZ >> 8) & 0xFF;

  packet[8] = computeCRC(packet, 8);

  bt.write(packet, 9);
  seq++;
}

// -------- SETUP --------
void setup() {
  Wire.begin();
  initMPU();

  bt.begin(9600);     // HC-05 default
  Serial.begin(9600); // Debug (optional)

  Serial.println("TX READY");
}

// -------- LOOP --------
void loop() {
  unsigned long now = millis();

  if (now - lastTxTime >= TX_RATE_MS) {
    lastTxTime = now;

    readGyro();
    sendPacket();

    // Optional debug
    /*
    Serial.print(seq);
    Serial.print(" | ");
    Serial.print(gyroX);
    Serial.print(" ");
    Serial.print(gyroY);
    Serial.print(" ");
    Serial.println(gyroZ);
    */
  }
}
