#include <esp_now.h>
#include <WiFi.h>
#include <Servo.h>

Servo servo; 

#define SERVO_PIN  12
#define M1_IN1  5
#define M1_IN2  18

const int smoothStep = 1;
int currentAngle = 90;
int incomingSteer = 90;

unsigned long lastMove = 0;

char incomingCmd;

typedef struct {
  int angle;
  char dir;
} Telemetry;

uint8_t controllerMAC[6] = {0x78,0x1C,0x3C,0xA7,0xE3,0x94};  


void onReceive(const uint8_t *mac, const uint8_t *data, int len) { 
char msg[32];
memcpy(msg, data, len);
msg[len] = 0;
String s = String(msg);
int p1 = s.indexOf(',');
int p2 = s.lastIndexOf(',');
if (p1 < 1) return;
incomingCmd = s.charAt(0); 
incomingSteer = s.substring(p1+1, p2).toInt(); 
incomingSteer = constrain(incomingSteer, 50, 130); 
}


void setMotors(char cmd) {
  if (cmd == 'F') {
    digitalWrite(M1_IN1, HIGH); digitalWrite(M1_IN2, LOW);
  } 
  else if (cmd == 'B') {
    digitalWrite(M1_IN1, LOW); digitalWrite(M1_IN2, HIGH);
  } 
  else if (cmd == 'S') {
    digitalWrite(M1_IN1, LOW); digitalWrite(M1_IN2, LOW);
  } 
  else {
    digitalWrite(M1_IN1, LOW); digitalWrite(M1_IN2, LOW);
  }
}

void smoothMove(int &current, int target) {
  if (current < target) 
    current += smoothStep;
  else if (current > target)
    current -= smoothStep;
  servo.write(current);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  esp_now_init();
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, controllerMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  servo.attach(SERVO_PIN);
  servo.write(90);

  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
}

void sendTelemetry() {
  Telemetry t;
  t.angle = currentAngle;
  t.dir   = incomingCmd;

  esp_now_send(controllerMAC, (uint8_t*)&t, sizeof(t));
}

void loop() {
  setMotors(incomingCmd);

  unsigned long now = millis();
  if (now - lastMove > 20) {
    lastMove = now;
    smoothMove(currentAngle, incomingSteer);
  }

  static unsigned long lastT = 0;
  if (millis() - lastT > 500) {
    lastT = millis();
    sendTelemetry();
    
  }
}