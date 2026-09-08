// H12 — Drone Controller (ESP32)
// MAVLink handling, RC receiver emulation, drone detection

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <math.h>

// ── Configuration ──────────────────────────────────────────────
#define SERIAL_BAUD        115200
#define MAVLINK_BAUD       57600
#define MAVLINK_TX_PIN     17
#define MAVLINK_RX_PIN     16
#define RC_CHANNEL_COUNT   8
#define RC_UPDATE_MS       20
#define DETECT_SCAN_MS     500

// ── MAVLink v2 minimal header ─────────────────────────────────
struct MavlinkMsg {
  uint8_t  startFlag;   // 0xFE (v1) or 0xFD (v2)
  uint8_t  length;
  uint8_t  seq;
  uint8_t  sysid;
  uint8_t  compid;
  uint8_t  msgid;
  uint8_t  payload[255];
  uint8_t  checksum[2];
};

// ── RC channel values (1000–2000 µs PWM) ─────────────────────
uint16_t rcChannels[RC_CHANNEL_COUNT] = {
  1500,  // Roll
  1500,  // Pitch
  1500,  // Throttle
  1500,  // Yaw
  1500,  // Aux1
  1500,  // Aux2
  1000,  // Aux3
  1000   // Aux4
};

// ── Detected drones ───────────────────────────────────────────
struct DetectedDrone {
  uint8_t  mac[6];
  uint32_t lastSeen;
  int8_t   rssi;
  uint16_t protocol;   // 0=unknown, 1=MAVLink, 2=ESP-NOW, 3=Bayang
};
DetectedDrone drones[16];
int droneCount = 0;

// ── Forward declarations ──────────────────────────────────────
void     initMavlink();
void     initEspNow();
void     sendMavlinkHeartbeat();
void     sendMavlinkRcChannels();
void     handleMavlinkIncoming();
void     emulateRcReceiver();
void     detectDrones();
void     parseEspNowPacket(const uint8_t* mac, const uint8_t* data, int len);
uint16_t crcAccum(uint16_t crc, uint8_t data);
String   macToString(const uint8_t* mac);

// ── ESP-NOW receive callback ──────────────────────────────────
void onDataRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  parseEspNowPacket(info->src_addr, data, len);
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);
  Serial.println(F("\n=== H12 — Drone Controller ==="));

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  initEspNow();
  initMavlink();

  Serial.println(F("MAVLink + ESP-NOW + RC emulation active"));
  Serial.println(F("Commands: t=arm  s=disarm  w=throttle_up  d=throttle_down\n"));
}

// ── Main loop ─────────────────────────────────────────────────
void loop() {
  static uint32_t lastRc     = 0;
  static uint32_t lastDetect = 0;
  static uint32_t lastHb     = 0;
  static bool     armed      = false;

  // Handle serial commands
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 't': case 'T':
        armed = true;
        rcChannels[2] = 1100;  // Low throttle for arming
        Serial.println(F("[ARMED] Throttle low — ready to arm"));
        break;
      case 's': case 'S':
        armed = false;
        rcChannels[2] = 1000;  // Throttle cut
        Serial.println(F("[DISARMED]"));
        break;
      case 'w': case 'W':
        rcChannels[2] = min(2000, (int)rcChannels[2] + 50);
        Serial.printf("[THROTTLE] %d µs\n", rcChannels[2]);
        break;
      case 'd': case 'D':
        rcChannels[2] = max(1000, (int)rcChannels[2] - 50);
        Serial.printf("[THROTTLE] %d µs\n", rcChannels[2]);
        break;
    }
  }

  // Send RC channels at 50 Hz
  if (millis() - lastRc >= RC_UPDATE_MS) {
    lastRc = millis();
    emulateRcReceiver();
    sendMavlinkRcChannels();
  }

  // Send heartbeat at 1 Hz
  if (millis() - lastHb >= 1000) {
    lastHb = millis();
    sendMavlinkHeartbeat();
  }

  // Read incoming MAVLink
  handleMavlinkIncoming();

  // Detect drones
  if (millis() - lastDetect >= DETECT_SCAN_MS) {
    lastDetect = millis();
    detectDrones();
  }
}

// ── Init MAVLink serial ──────────────────────────────────────
void initMavlink() {
  Serial2.begin(MAVLINK_BAUD, SERIAL_8N1, MAVLINK_RX_PIN, MAVLINK_TX_PIN);
  Serial.printf("MAVLink on GPIO%d(TX)/GPIO%d(RX) @ %d baud\n",
    MAVLINK_TX_PIN, MAVLINK_RX_PIN, MAVLINK_BAUD);
}

// ── Init ESP-NOW ─────────────────────────────────────────────
void initEspNow() {
  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(onDataRecv);
    Serial.println("ESP-NOW initialized for drone detection");
  } else {
    Serial.println("ESP-NOW init failed");
  }
}

// ── MAVLink Heartbeat (Msg ID 0) ────────────────────────────
void sendMavlinkHeartbeat() {
  uint8_t buf[26];
  buf[0] = 0xFE;        // MAVLink v1 start
  buf[1] = 9;           // payload length
  buf[2] = 0;           // sequence
  buf[3] = 255;         // system ID (any)
  buf[4] = 0;           // component ID
  buf[5] = 0;           // message ID: HEARTBEAT

  // type=0(Generic), autopilot=0(Unknown), base_mode=0, custom_mode=0, system_status=0
  memset(&buf[6], 0, 9);

  uint16_t crc = 0xFFFF;
  for (int i = 1; i < 16; i++) crc = crcAccum(crc, buf[i]);
  buf[15] = crc & 0xFF;
  buf[16] = crc >> 8;

  Serial2.write(buf, 17);
}

// ── MAVLink RC Channels (Msg ID 35) ─────────────────────────
void sendMavlinkRcChannels() {
  uint8_t buf[36];
  buf[0] = 0xFE;
  buf[1] = 20;          // payload
  buf[2] = 1;           // sequence
  buf[3] = 255;
  buf[4] = 0;
  buf[5] = 35;          // RC_CHANNELS

  uint32_t timeMs = millis();
  buf[6] = timeMs & 0xFF;
  buf[7] = (timeMs >> 8) & 0xFF;
  buf[8] = (timeMs >> 16) & 0xFF;
  buf[9] = (timeMs >> 24) & 0xFF;

  buf[10] = RC_CHANNEL_COUNT;
  buf[11] = 0;  // port

  for (int i = 0; i < RC_CHANNEL_COUNT; i++) {
    int off = 12 + i * 2;
    buf[off]     = rcChannels[i] & 0xFF;
    buf[off + 1] = (rcChannels[i] >> 8) & 0xFF;
  }

  uint16_t crc = 0xFFFF;
  for (int i = 1; i < 32; i++) crc = crcAccum(crc, buf[i]);
  buf[32] = crc & 0xFF;
  buf[33] = crc >> 8;

  Serial2.write(buf, 34);
}

// ── Read incoming MAVLink ────────────────────────────────────
void handleMavlinkIncoming() {
  while (Serial2.available()) {
    uint8_t b = Serial2.read();
    if (b == 0xFE || b == 0xFD) {
      // Potential MAVLink frame header — log it
      Serial.printf("[MAV RX] Start=0x%02X\n", b);
    }
  }
}

// ── Emulate RC receiver ──────────────────────────────────────
void emulateRcReceiver() {
  // Simulate gentle stick movements when armed
  static uint32_t t = 0;
  t++;

  if (t % 100 == 0) {
    // Subtle jitter to simulate real RC
    rcChannels[0] = constrain(rcChannels[0] + random(-2, 3), 1000, 2000);
    rcChannels[1] = constrain(rcChannels[1] + random(-2, 3), 1000, 2000);
  }
}

// ── Detect drones via ESP-NOW ────────────────────────────────
void detectDrones() {
  // Purge stale entries
  uint32_t now = millis();
  for (int i = droneCount - 1; i >= 0; i--) {
    if (now - drones[i].lastSeen > 5000) {
      // Shift remaining
      for (int j = i; j < droneCount - 1; j++) drones[j] = drones[j + 1];
      droneCount--;
    }
  }
}

// ── Parse ESP-NOW packet for drone signatures ────────────────
void parseEspNowPacket(const uint8_t* mac, const uint8_t* data, int len) {
  // Check for known drone protocol signatures
  uint16_t proto = 0;
  if (len >= 2 && data[0] == 0xAA && data[1] == 0x11) proto = 3;  // Bayang
  if (len >= 4 && data[0] == 0x4C && data[1] == 0x43) proto = 2;  // ESP-NOW drone

  // Check if already tracked
  for (int i = 0; i < droneCount; i++) {
    if (memcmp(drones[i].mac, mac, 6) == 0) {
      drones[i].lastSeen = millis();
      drones[i].rssi = 0;
      return;
    }
  }

  // New drone detected
  if (droneCount < 16) {
    memcpy(drones[droneCount].mac, mac, 6);
    drones[droneCount].lastSeen = millis();
    drones[droneCount].protocol = proto;
    drones[droneCount].rssi = 0;
    droneCount++;

    Serial.printf("[DRONE] New: %s (proto=%d)\n", macToString(mac).c_str(), proto);
  }
}

// ── CRC-16/MCRF4XX (MAVLink standard) ───────────────────────
uint16_t crcAccum(uint16_t crc, uint8_t data) {
  uint8_t tmp = data ^ (uint8_t)(crc & 0xFF);
  tmp ^= (tmp << 4);
  crc = (crc >> 8) ^ ((uint16_t)tmp << 8) ^ ((uint16_t)tmp << 3) ^ (tmp >> 4);
  return crc;
}

// ── MAC to string ────────────────────────────────────────────
String macToString(const uint8_t* mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}
