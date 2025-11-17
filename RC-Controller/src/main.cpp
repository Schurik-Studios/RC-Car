#include <esp_now.h>
#include <WiFi.h>

const int JOY_X = 34;
const int JOY_Y = 35;

const int DEADZONE = 500;
const int STEER_CENTER = 1750;

const int STEER_MIN_ANGLE = 50;
const int STEER_MAX_ANGLE = 130;

unsigned long lastSend = 0;

typedef struct {
  int angle;
  int speed;
  char dir;
} Telemetry;

uint8_t carMAC[6] = {0x00, 0x4B, 0x12, 0x30, 0xC3, 0xB8};

//Telemetrie
void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(Telemetry)) return;
  Telemetry t;
  memcpy(&t, data, len);

  Serial.printf("Tele: Angle=%d  Speed=%d  Dir=%c\n", t.angle, t.speed, t.dir);
}

int mapSteering(int rawX) {
  int dx = rawX - STEER_CENTER;

  if (abs(dx) < DEADZONE) return 90;

  long angle = map(rawX, 0, 4095, STEER_MAX_ANGLE, STEER_MIN_ANGLE);
  return constrain(angle, STEER_MIN_ANGLE, STEER_MAX_ANGLE);
}

int mapSpeed(int rawY) {
  int dy = rawY - STEER_CENTER;
  int dY = abs(dy);
  if (dY < DEADZONE) return 0;
  if (dY < 1000) return 1;
  return 2;
}


char mapSteering2(int rawY) {
  int dy = rawY - STEER_CENTER;

  if (abs(dy) < DEADZONE) return 'S';
  if (dy > 550) return 'F';
  if (dy < -550) return 'B';
  return 'S';
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  esp_now_init();
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, carMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void loop() {
  if (millis() - lastSend < 100) return;
  lastSend = millis();

  int x = analogRead(JOY_X);
  int y = analogRead(JOY_Y);

  char cmd = mapSteering2(y);
  int steering = mapSteering(x);
  int steeringspeed = mapSpeed(x);
  if (cmd == 'S');

  char msg[32];
  sprintf(msg, "%c,%d,%d", cmd, steering, steeringspeed); // was macht sprintf ? 

  esp_now_send(carMAC, (uint8_t*)msg, strlen(msg)+1);
  Serial.printf("%d %d", x, y);
  Serial.printf("TX: %s\n", msg);
}
