#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <SoftwareSerial.h>

// ใช้ D2 (TX), D3 (RX) เป็น Serial เสมือน
SoftwareSerial espSerial(2, 3);  // TX, RX

bool hasPrintedStart = false; //time
unsigned long now = millis();
unsigned long heater_start_time; //time variable
const unsigned long heater_duration = 240UL * 60UL * 1000UL; // ul=min นาที (ค่า*60*1000 ms)

bool showDetail = true; //lcd
unsigned long lastLcdSwitchTime = 0;
const unsigned long lcdSwitchInterval = 10000; // 2 วินาที
bool hasPrintedEnd = false;

bool isReady = false; //heater ready 

#define DHT1PIN 5
#define DHT2PIN 10
#define DHTTYPE DHT22
DHT dht1(DHT1PIN, DHTTYPE);
DHT dht2(DHT2PIN, DHTTYPE);

int rel1 = A0; //relFan 5 v 
int rel2 = A1; // relFan 12v
int rel3 = A2; // heat

LiquidCrystal_I2C lcd(0x27, 20, 4);

void lcdshow(float temp1, float hum1,float temp2, float hum2,float temp, float hum);
void startTime();
void senddata(float temp1, float hum1,float temp2, float hum2,float temp, float hum);
void reloff();
void relopen();
void iot();


void setup() {
  Serial.begin(9600); 
  Serial.println("START");

  dht1.begin();
  dht2.begin();

  pinMode(rel1, OUTPUT);
  pinMode(rel2, OUTPUT);
  pinMode(rel3, OUTPUT);

  // digitalWrite(rel1, HIGH);//relopen
  // digitalWrite(rel2, HIGH);
  // digitalWrite(rel3, LOW);

  digitalWrite(rel1, LOW);//relay on system off
  digitalWrite(rel2, LOW);
  digitalWrite(rel3, HIGH);

  lcd.begin(); 
  lcd.backlight();
  lcd.setCursor(5, 0); // ไปที่ตัวอักษรที่ 0 แถวที่ 1
  lcd.print("SYSTEM START");

  Serial.println("\ntest capteur DTH22");
  heater_start_time = millis(); 
}
enum DryingState {
  WAITING,      // ยังไม่เริ่ม
  PREHEAT,      // รออุณหภูมิ 20-40
  DRYING,       // อบ 40-50
  PAUSED,       // หยุดเพราะ temp > 50
  FINISHED      // ครบเวลา
};

DryingState state = WAITING;


void loop() {
  unsigned long now = millis();

  float temp1 = dht1.readTemperature();
  float hum1 = dht1.readHumidity();
  float temp2 = dht2.readTemperature();
  float hum2 = dht2.readHumidity();

  int x = 2;
  if (isnan(temp1) || isnan(hum1)) { temp1 = 0; hum1 = 0; x -= 1; }
  if (isnan(temp2) || isnan(hum2)) { temp2 = 0; hum2 = 0; x -= 1; }
  if (x == 0) x = 1;

  float temp = (temp1 + temp2) / x;
  float hum = (hum1 + hum2) / x;

  iot(); 
  senddata(temp1, hum1, temp2, hum2, temp, hum);

  switch (state) {
    case WAITING:
    case PREHEAT:
      if (temp >= 20 && temp < 40) {
        state = PREHEAT;
        digitalWrite(rel1, LOW);  // fan 12V Off correct
        digitalWrite(rel2, HIGH);
        digitalWrite(rel3, LOW);  // heater open
      // else if (temp >= 44 && temp <= 47) {//fanopen
      //   digitalWrite(rel1, HIGH);
      //   digitalWrite(rel2, HIGH);
      //   digitalWrite(rel3, LOW);  // heater ON
      } else if (temp >= 40 && temp <= 50) {
        state = DRYING;
        heater_start_time = millis(); // ⏱️ เริ่มจับเวลา
        digitalWrite(rel3, LOW);  //
        digitalWrite(rel2, HIGH);
        digitalWrite(rel3, LOW);  // heater ON
        Serial.println("Start Drying");
      }

      break;

    case DRYING:
      if ((temp > 40) && (temp < 50)) {
        // state = PAUSED;
        digitalWrite(rel3, LOW);
        digitalWrite(rel2, HIGH);
        digitalWrite(rel3, LOW);  // เปิดทุกอย่าง
        // Serial.println("Paused: Too Hot");
      if (temp >= 48){
          digitalWrite(rel1, HIGH);//fan open
          digitalWrite(rel2, HIGH);
          digitalWrite(rel3, LOW);  
          if (temp >= 42){
              digitalWrite(rel1, LOW);//fan off
              digitalWrite(rel2, HIGH);
              digitalWrite(rel3, LOW);  // เปิดทุกอย่าง
            }
        }
          
      else{
        state = PAUSED;
      }
        
              

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
        digitalWrite(rel1, HIGH);
        digitalWrite(rel2, HIGH);
        digitalWrite(rel3, LOW);  // ทำงานต่อ
      }
      break;

    case PAUSED:
      if (temp >= 50 ) {
        // state = DRYING;
        Serial.println("Resumed");
        digitalWrite(rel1, HIGH);
        digitalWrite(rel2, LOW);
        digitalWrite(rel3, HIGH);
      }
      else if (temp>=42){
        state = DRYING;
        digitalWrite(rel1, HIGH);

      }
      // } else {
      //   digitalWrite(rel1, LOW);
      //   digitalWrite(rel2, LOW);
      //   digitalWrite(rel3, HIGH);
      // }
      break;

    case FINISHED:
      // ไม่มีอะไรให้ทำ
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

void startTime(){
  unsigned long now = millis();

  if (!hasPrintedStart && now >= 5000 && now <= 10000) {
    Serial.println("START");
    hasPrintedStart = true;
  }
}

void senddata(float temp1, float hum1, float temp2, float hum2, float temp, float hum) {
  // ตรวจสอบความถูกต้องก่อนส่ง
  // if (isnan(temp) || isnan(hum) || hum < 0 || hum > 100 || temp < 10 || temp > 80) {
  //   Serial.println("Invalid sensor data, not sending.");
  //   return;
  // }

  // ส่งค่า temp และ hum โดยตรงให้ ESP
  String data = String(temp, 1) + "," + String(hum, 1);

  espSerial.print(data);
  espSerial.print('\n');  // ส่งเฉพาะ \n แบบเดียวกับที่ ESP32 ใช้รับ

  Serial.println("Sent to ESP: " + data); // Debug
}


void lcdshow(float temp1, float hum1, float temp2, float hum2,float temp, float hum) {
  lcd.clear();

  if (showDetail) {
    lcd.setCursor(0, 0);
    lcd.print("Temp1: ");
    if (!isnan(temp1)) {
      lcd.print(temp1, 1);
      lcd.print(" C");
  } else {
    lcd.print("Error");
  }

    lcd.setCursor(0, 1);
    lcd.print("Hum1: ");
    if (!isnan(hum1)) {
     lcd.print(hum1, 1);
     lcd.print(" %");
    } else {
      lcd.print("Error");
    }

    lcd.setCursor(0, 2);
    lcd.print("Temp 2: ");
    if (!isnan(temp2)) {
      lcd.print(temp2, 1);
      lcd.print(" c");
    } else {
      lcd.print("Error");

    }
  
    lcd.setCursor(0, 3);
    lcd.print("Hum 2: ");
    if (!isnan(hum2)) {
      lcd.print(hum2, 1);
      lcd.print(" %");
   } else {
    lcd.print("Error");
   }
  
  }else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp avg: ");
    if (!isnan(temp)) {
    lcd.print(temp, 1);
    lcd.print(" C");
  } else {
    lcd.print("Error");
  }

    lcd.setCursor(0, 1);
    lcd.print("Hum avg: ");
    if (!isnan(hum)) {
      lcd.print(hum, 1);
      lcd.print(" %");
    } else {
      lcd.print("Error");

   }
  }
  
}

void reloff(){

  digitalWrite(rel1, HIGH);
  digitalWrite(rel2, HIGH);
  digitalWrite(rel3, LOW);

}

void relopen(){

  digitalWrite(rel1, LOW);
  digitalWrite(rel2, LOW);
  digitalWrite(rel3, HIGH);

}

void iot() {

  while (Serial.available()) {
    String var = Serial.readStringUntil('\n');
    var.trim();
    Serial.println("Received: " + var);
    // if (cmd == "ON") {
    //   digitalWrite(LED_PIN, HIGH);
    // } else if (cmd == "OFF") {
    //   digitalWrite(LED_PIN, LOW);
    // }
    if (var == "f1_on" )  {
      relopen();
    }
    if (var == "f1_off") {
      reloff();
    }

  }
}




