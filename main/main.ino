#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SoftwareSerial.h>

bool hasPrintedStart = false; //time
unsigned long now = millis();
unsigned long heater_start_time; //time variable
const unsigned long heater_duration = 240UL * 60UL * 1000UL; // ul=min นาที (ค่า*60*1000 ms)

bool showDetail = true; //lcd
unsigned long lastLcdSwitchTime = 0;
const unsigned long lcdSwitchInterval = 10000; // 2 วินาที
bool hasPrintedEnd = false;

bool isReady = false; //heater ready 

#define DHT1PIN 3
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

SoftwareSerial espSerial(2, 3);

void setup() {
  Serial.begin(9600);  // ใช้ดู debug บน Serial Monitor
  espSerial.begin(9600);  // ติดต่อกับ ESP32

  dht1.begin();
  dht2.begin();

  pinMode(rel1, OUTPUT);
  pinMode(rel2, OUTPUT);
  pinMode(rel3, OUTPUT);

  // digitalWrite(rel1, HIGH);//relopen
  // digitalWrite(rel2, HIGH);
  // digitalWrite(rel3, LOW);

  digitalWrite(rel1, LOW);//reloff
  digitalWrite(rel2, LOW);
  digitalWrite(rel3, HIGH);

  lcd.begin(); 
  lcd.backlight();
  lcd.setCursor(5, 0); // ไปที่ตัวอักษรที่ 0 แถวที่ 1
  lcd.print("SYSTEM START");

  Serial.println("\ntest capteur DTH22");
  heater_start_time = millis(); 

  
}


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

  float temp = ((temp1 + temp2) / x);
  float hum = ((hum1 + hum2) / x);

  startTime();
  iot(); 
  senddata(temp1, hum1, temp2, hum2, temp, hum);

  // ✅ ตรวจเริ่มทำงาน
  if (!isReady && temp >= 50.0 && temp <= 60.0) {
    isReady = true;
    heater_start_time = millis();
    Serial.println("START");

    lcd.clear();
    lcd.setCursor(3, 1);
    lcd.print("Start Drying Herb");
    delay(2000);
  }

  // ✅ ตรวจสิ้นสุด
  if (!hasPrintedEnd && (now - heater_start_time >= heater_duration)) {
    Serial.println("END");
    hasPrintedEnd = true;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Finishing Drying Herb");
    delay(5000);
  }

  // ✅ ระบบทำงาน
  if (isReady) {
    if (now - heater_start_time <= heater_duration) {
      if ((temp >= 20) && (temp <= 40)) {
        digitalWrite(rel1, LOW);
        digitalWrite(rel2, HIGH);
        digitalWrite(rel3, LOW);
      } else if ((temp >= 50) && (temp < 60)) {
        digitalWrite(rel1, HIGH);
        digitalWrite(rel2, HIGH);
        digitalWrite(rel3, LOW);
      } else if (temp > 60) {
        digitalWrite(rel1, LOW);
        digitalWrite(rel2, LOW);
        digitalWrite(rel3, HIGH);
      }
    } else {
      // ครบเวลา
      if (!hasPrintedEnd) {
        Serial.println("END");
        hasPrintedEnd = true;
      }
      digitalWrite(rel1, LOW);
      digitalWrite(rel2, LOW);
      digitalWrite(rel3, HIGH);
    }
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

void senddata(float temp1, float hum1, float temp2, float hum2,float temp, float hum){

   if (true) {

    String data = String(temp, 1) + "," + String(hum, 1);
    espSerial.println(data);  // ส่งให้ ESP32
    Serial.println("Sent to ESP32: " + data);
  } else {
    Serial.println("No valid sensor data.");
  }
   
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




