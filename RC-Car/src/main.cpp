#include <esp_now.h>
#include <WiFi.h>
#include <Servo.h>

Servo steer;

const int SERVO_PIN = 12;

// Motor A
const int M1_IN1 = 5;
const int M1_IN2 = 18;

float currentAngle = 90;
float targetAngle = 90;

unsigned long lastMove = 0;

char incomingCmd;
int incomingSteer;
int incomingSteeringSpeed;

// Telemetrie
typedef struct {
  int angle;
  int speed;
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
  if (p1 < 1 || p2 < p1) return;

  incomingCmd   = s.charAt(0);
  incomingSteer = s.substring(p1+1, p2).toInt();
  incomingSteeringSpeed = s.substring(p2+1).toInt();

  incomingSteer = constrain(incomingSteer, 50, 130);
  targetAngle = incomingSteer;
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

  steer.attach(SERVO_PIN);
  steer.write(90);

  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
}

void sendTelemetry() {
  Telemetry t;
  t.angle = currentAngle;
  //t.speed = incomingSpeed;
  t.dir   = incomingCmd;

  esp_now_send(controllerMAC, (uint8_t*)&t, sizeof(t));
}

void loop() {
  setMotors(incomingCmd);

  unsigned long now = millis();
  if (now - lastMove > 20) {
    lastMove = now;
    if(incomingSteeringSpeed == 0) currentAngle = targetAngle;
     if (incomingSteeringSpeed == 1){
       if(currentAngle > targetAngle) currentAngle -=0.25;
       if(currentAngle < targetAngle) currentAngle += 0.25;
    }
     if (incomingSteeringSpeed == 2){
       if(currentAngle > targetAngle) currentAngle -=0.5;
       if(currentAngle < targetAngle) currentAngle += 0.5;
    }
  }

    steer.write(currentAngle);
  

  static unsigned long lastT = 0;
  if (millis() - lastT > 500) {
    lastT = millis();
    sendTelemetry();
    
  }
}