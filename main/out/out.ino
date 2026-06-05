#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SoftwareSerial.h>

SoftwareSerial espSerial(2, 3);  // TX, RX

bool hasPrintedStart = false;
unsigned long heater_start_time;
const unsigned long heater_duration = 150UL * 60UL * 1000UL;

bool showDetail = true;
unsigned long lastLcdSwitchTime = 0;
const unsigned long lcdSwitchInterval = 10000;

#define DHT1PIN 5
#define DHT2PIN 10
#define DHTTYPE DHT22
DHT dht1(DHT1PIN, DHTTYPE);
DHT dht2(DHT2PIN, DHTTYPE);

int rel1 = A0; // พัดลม 12V
int rel2 = A1; // พัดลม 5V
int rel3 = A2; // ฮีตเตอร์

LiquidCrystal_I2C lcd(0x27, 20, 4);

void lcdshow(float temp1, float hum1, float temp2, float hum2, float temp, float hum);
void senddata(float temp1, float hum1, float temp2, float hum2, float temp, float hum);
void reloff();
void relopen();
void iot();

enum DryingState {
  WAITING, PREHEAT, DRYING, PAUSED, FINISHED
};
DryingState state = WAITING;

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);

  dht1.begin();
  dht2.begin();

  pinMode(rel1, OUTPUT);
  pinMode(rel2, OUTPUT);
  pinMode(rel3, OUTPUT);

  digitalWrite(rel1, LOW); // relay OFF fan off
  digitalWrite(rel2, LOW);//
  digitalWrite(rel3, HIGH);

  lcd.begin();
  lcd.backlight();
  lcd.setCursor(5, 0);
  lcd.print("SYSTEM START");

  heater_start_time = millis();
}

void loop() {
  unsigned long now = millis();

  float temp1 = dht1.readTemperature();
  float hum1 = dht1.readHumidity();
  float temp2 = dht2.readTemperature();
  float hum2 = dht2.readHumidity();

  int valid = 2;
  if (isnan(temp1) || isnan(hum1)) { temp1 = 0; hum1 = 0; valid--; }
  if (isnan(temp2) || isnan(hum2)) { temp2 = 0; hum2 = 0; valid--; }
  if (valid == 0) valid = 1;

  float temp = (temp1 + temp2) / valid;
  float hum = (hum1 + hum2) / valid;

  iot();
  senddata(temp1, hum1, temp2, hum2, temp, hum);

  switch (state) {
    case WAITING:
    case PREHEAT:
      if (temp >= 20 && temp < 50) {
        state = PREHEAT;
        digitalWrite(rel1, LOW);  // พัดลม 12V ปิด
        digitalWrite(rel2, HIGH); // พัดลม 5V เปิด
        digitalWrite(rel3, LOW);  // ฮีตเตอร์ เปิด
      } else if (temp >= 40 && temp <= 50) {
        state = DRYING;
        heater_start_time = millis();
        digitalWrite(rel2, HIGH);
        digitalWrite(rel3, LOW);
        Serial.println("Start Drying");
      }
      break;

        case DRYING:
      if (temp >= 50) {
        state = PAUSED;
        digitalWrite(rel1, HIGH);  // เปิดพัดลม 12V
        digitalWrite(rel2, LOW);   // ปิดพัดลม 5V
        digitalWrite(rel3, HIGH);  // ปิดฮีตเตอร์
        Serial.println("Too hot, pause drying");
      } else if (now - heater_start_time >= heater_duration) {
        state = FINISHED;
        digitalWrite(rel1, LOW);
        digitalWrite(rel2, LOW);
        digitalWrite(rel3, HIGH);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Finishing Drying");
        delay(5000);
        Serial.println("END");
      } else {
        digitalWrite(rel1, LOW);   // พัดลม 12V ปิด
        digitalWrite(rel2, HIGH);  // พัดลม 5V เปิด
        digitalWrite(rel3, LOW);   // heater เปิด
      }
      break;

    case PAUSED:
      if (temp >= 57) {
        digitalWrite(rel1, HIGH);  // พัดลม 12V เปิด
        digitalWrite(rel2, LOW);   // ปิดพัดลม 5V
        digitalWrite(rel3, HIGH);  // ปิด heater
      } else if (temp <= 53) {
        state = DRYING;
        digitalWrite(rel1, LOW);   // ปิดพัดลม 12V
        digitalWrite(rel2, HIGH);  // เปิดพัดลม 5V
        digitalWrite(rel3, LOW);   // เปิด heater
        Serial.println("Resume Drying");
      }
      break;

    case FINISHED:
      digitalWrite(rel1, LOW);
      digitalWrite(rel2, LOW);
      digitalWrite(rel3, HIGH);
      break;
  }

  if (millis() - lastLcdSwitchTime >= lcdSwitchInterval) {
    lcdshow(temp1, hum1, temp2, hum2, temp, hum);
    showDetail = !showDetail;
    lastLcdSwitchTime = millis();
  }
}

void senddata(float temp1, float hum1, float temp2, float hum2, float temp, float hum) {
  
  String data = String(temp, 1) + "," + String(hum, 1);
  espSerial.print(data);
  espSerial.print('\n');
  Serial.println("Sent to ESP: " + data);
  delay(1000);

}

void lcdshow(float temp1, float hum1, float temp2, float hum2, float temp, float hum) {
  lcd.clear();

  if (showDetail) {
    lcd.setCursor(0, 0);
    lcd.print("T1: ");
    lcd.print(isnan(temp1) ? "Err" : String(temp1, 1) + "C");

    lcd.setCursor(10, 0);
    lcd.print("H1: ");
    lcd.print(isnan(hum1) ? "Err" : String(hum1, 1) + "%");

    lcd.setCursor(0, 1);
    lcd.print("T2: ");
    lcd.print(isnan(temp2) ? "Err" : String(temp2, 1) + "C");

    lcd.setCursor(10, 1);
    lcd.print("H2: ");
    lcd.print(isnan(hum2) ? "Err" : String(hum2, 1) + "%");

    lcd.setCursor(0, 3);  // แสดงสถานะที่บรรทัดล่างสุด
    lcd.print("State: ");
    switch (state) {
      case WAITING: lcd.print("WAIT"); break;
      case PREHEAT: lcd.print("PREHEAT"); break;
      case DRYING: lcd.print("DRYING"); break;
      case PAUSED: lcd.print("PAUSED"); break;
      case FINISHED: lcd.print("DONE"); break;
    }

  } else {
    lcd.setCursor(0, 0);
    lcd.print("AvgTemp: ");
    lcd.print(isnan(temp) ? "Err" : String(temp, 1) + "C");

    lcd.setCursor(0, 1);
    lcd.print("AvgHum: ");
    lcd.print(isnan(hum) ? "Err" : String(hum, 1) + "%");

    // 🔸 เพิ่มแสดงเวลาอบ
    unsigned long elapsedMillis = 0;
    if (state == DRYING || state == PAUSED || state == FINISHED) {
      elapsedMillis = millis() - heater_start_time;
    }

    unsigned int minutes = (elapsedMillis / 60000) % 60;
    unsigned int hours = elapsedMillis / 3600000;

    lcd.setCursor(0, 2);
    lcd.print("Time: ");
    lcd.print(hours);
    lcd.print("h ");
    lcd.print(minutes);
    lcd.print("m");

    lcd.setCursor(0, 3);
    lcd.print("State: ");
    switch (state) {
      case WAITING: lcd.print("WAIT"); break;
      case PREHEAT: lcd.print("PREHEAT"); break;
      case DRYING: lcd.print("DRYING"); break;
      case PAUSED: lcd.print("PAUSED"); break;
      case FINISHED: lcd.print("DONE"); break;
    }
  }
}

void reloff() {
  digitalWrite(rel1, HIGH);
  digitalWrite(rel2, HIGH);
  digitalWrite(rel3, LOW);
}

void relopen() {
  digitalWrite(rel1, LOW);
  digitalWrite(rel2, LOW);
  digitalWrite(rel3, HIGH);
}

void iot() {
  while (Serial.available()) {
    String var = Serial.readStringUntil('\n');
    var.trim();
    Serial.println("Received: " + var);
    if (var == "f1_on") relopen();
    if (var == "f1_off") reloff();
  }
}
