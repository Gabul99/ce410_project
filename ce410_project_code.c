#include <SPI.h>
#include <MFRC522.h>

int flamesensor = A5;      // Read the sensor[0] value
int gassensor = A0;
int tempsensor = A4;
int led = 4;
int echo = 7;
int trig = 8;
long pole_sonic;
long duration, distance;
float pole_flame;
float pole_gas;
float pole_temp;
float pole_ultra;
float sensor_min[3] = {1024, 1024, 1024};
float sensor_max[3] = {0, 0, 0};
long ultra_max;
long ultra_min;

bool people = false;
bool u_people = false;

char* rfidlist[16][16];
int SS_PIN = 10;
int RST_PIN = 9;
byte nuidPICC[4];

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;


void setup(){
  Serial.begin(9600);
  
  SPI.begin();
  rfid.PCD_Init();

  for (byte i = 0; i < 0; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  pinMode(led, OUTPUT);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  for (int i = 0; i < 50; i++) {
    float sensorValue[3];
    sensorValue[0] = analogRead(flamesensor);
    sensorValue[1] = analogRead(gassensor);
    sensorValue[2] = analogRead(tempsensor);
    Serial.print(i);
    Serial.print("flame :");
    Serial.println(sensorValue[0]);
    Serial.print("gas :");
    Serial.println(sensorValue[1]);
    Serial.print("temp :");
    Serial.println(sensorValue[2]);
    for (int j = 0; j < 3; j++) {
      normalization(sensorValue[j], j);
    }
    delay(100);
  }
  pole_flame = (sensor_min[0] + sensor_max[0]) / 2;
  pole_gas = (sensor_min[1] + sensor_max[1]) / 2;
  pole_temp = (sensor_min[2] + sensor_max[2]) / 2;
  for (int i = 0; i < 50; i++) {
    long Value = ultraValueMake();
    if (Value > ultra_max) {
      ultra_max = Value;
    } else if (Value < ultra_min) {
      ultra_min = Value;
    }
  }
  pole_ultra = (ultra_max + ultra_min) / 2;
  Serial.println("End calibration");
}

long ultraValueMake() {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig,HIGH);
  delayMicroseconds(10);
  digitalWrite(trig,LOW);
  duration = pulseIn(echo, HIGH);
  distance = duration/58.2;
  if (distance < 0) {
    Serial.println("ultrasonic error");
  } else {
    return distance;
  }
}

void normalization(int sensorval, int i) {
  if(sensorval > sensor_max[i]) {
    sensor_max[i] = sensorval;
  }
  if(sensorval < sensor_min[i]) {
    sensor_min[i] = sensorval;
  }
}
int serious_flame(int val) {
  float diff = pole_flame - val;
  float pole = sensor_max[0] - sensor_min[0];
  if (diff > pole*1.5) {
    return 3;
  } else if (diff > pole*1.25) {
    return 2;
  } else if (diff > pole) {
    return 1;
  } else {
    return 0;
  }
}
int serious_gas(int val) {
  float diff = val - pole_gas;
  float pole = sensor_max[1] - sensor_min[1];
  if (diff > 100) {
    return 3;
  } else if (diff > 50) {
    return 2;
  } else if (diff > pole) {
    return 1;
  } else {
    return 0;
  }
}
int serious_temp(int val) {
  float diff = val - pole_temp;
  if (diff > 50) {
    return 3;
  } else if (diff > 30) {
    return 2;
  } else if (diff > 10) {
    return 1;
  } else {
    return 0;
  }
}

bool ispeople() {
  if (u_people == true && people == true) {
    return true;
  } else {
    return false;
  }
}

void emergency() {
  if (ispeople) {
    digitalWrite(led, HIGH);
    Serial.println("Emergency Fire in the Room");
  }
}



void loop(){
//  int val = digitalRead(sensor);  // Read the sensor value
  int flameval = analogRead(flamesensor);  // Read the sensor value
  int gasval = analogRead(gassensor);
  int tempval = analogRead(tempsensor);
  int ultraval = (int)ultraValueMake();
  digitalWrite(led, LOW);

  if (rfid.PICC_IsNewCardPresent() || rfid.PICC_ReadCardSerial()) {
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    if (people) {
      people = false;
      Serial.println("people out");
    } else {
      people = true;
      Serial.println("people in");
    }
  }

  if (pole_ultra > ultraval) {
    if (people) {
      u_people = true;
    } else {
      u_people = false;
    }
  }
  
  int serious = serious_flame(flameval) + serious_gas(gasval) + serious_temp(tempval);
  if (serious >= 4) {
    emergency();
  } else if (serious >= 3) {
    Serial.println("It is possible to fire in the Room");
  } // call ThingSpeak
  Serial.print(",");
  Serial.print(serious);
  Serial.print(",");
  Serial.print(flameval);
  Serial.print(",");
  Serial.print(gasval);
  Serial.print(",");
  Serial.println(tempval);
  delay(1000);
}