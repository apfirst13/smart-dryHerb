#define BLYNK_TEMPLATE_ID "TMPL6Ylwz0DOT"
#define BLYNK_TEMPLATE_NAME "ledtest1"
#define BLYNK_AUTH_TOKEN "JnrP-KNZTRcW2sesLZl2dgoWIhB4-Ykm"

#define BLYNK_PRINT Serial

#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>

char ssid[] = "Fi2";
char pass[] = "U1234567";
const char* googleScriptId = "AKfycbwTbSzuC4ugmWQKQQ6D895QupD-Nu3abnyLR2tN-ZNIsv3TpR9bw8EF8ZxH5Xh7Bake";

HardwareSerial espSerial(2);  // UART2 (RX=16, TX=17)

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000;  // ส่งข้อมูลทุก 10 วินาที

void setup() {
  WiFi.begin(ssid, pass);

  Serial.print("Connecting to WiFi");
  int count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 30) { // 30 ครั้ง * 500ms = 15 วินาที
    delay(500);
    Serial.print(".");
    count++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
 
}

void loop() {
  Blynk.run();

  if (espSerial.available()) {
  char ch = espSerial.read();
  Serial.print(ch);  // ดูว่าได้รับอะไรจาก Uno แบบดิบๆ
  }

  if (espSerial.available()) {
    String line = espSerial.readStringUntil('\n');
    line.trim();

    if (line.length() > 0 && millis() - lastSendTime > sendInterval) {
      lastSendTime = millis();
      Serial.println("\n===============================");
      Serial.println("📥 Received: " + line);
      sendToGoogleSheet(line);
    }
  }
}

bool isFloat(String str) {
  if (str.length() == 0) return false;
  bool seenDot = false;
  for (int i = 0; i < str.length(); i++) {
    if (!isDigit(str[i])) {
      if (str[i] == '.' && !seenDot) seenDot = true;
      else return false;
    }
  }
  return true;
}

void sendToGoogleSheet(String line) {
  int comma = line.indexOf(',');
  if (comma < 1) {
    Serial.println("❌ Comma not found in line: " + line);
    return;
  }

  String t = line.substring(0, comma);
  String h = line.substring(comma + 1);

  Serial.println("🟡 Parsed → Temp: " + t + "°C, Hum: " + h + "%");

  if (!isFloat(t) || !isFloat(h)) {
    Serial.println("❌ Invalid number format.");
    return;
  }

  float temp = t.toFloat();
  float hum = h.toFloat();

  if (temp < 10 || temp > 80 || hum < 10 || hum > 100) {
    Serial.println("❌ Out of range → Temp: " + String(temp) + ", Hum: " + String(hum));
    return;
  }

  Serial.println("✅ Valid data. Sending to Blynk and Google Sheet...");
  Serial.println("   → Temp: " + String(temp, 1));
  Serial.println("   → Hum : " + String(hum, 1));

  // Update Blynk
  Blynk.virtualWrite(V2, temp);
  Blynk.virtualWrite(V3, hum);

  // Build Google Sheets URL
  String url = "https://script.google.com/macros/s/" + String(googleScriptId) +
               "/exec?Temp=" + t + "&Hum=" + h;
  Serial.println("🌐 Sending URL: " + url);

  WiFiClientSecure client;
  client.setInsecure();  // ไม่เช็ก SSL certificate
  HTTPClient https;

  if (https.begin(client, url)) {
    int httpCode = https.GET();
    Serial.println("📡 Google HTTP Response Code: " + String(httpCode));
    https.end();
  } else {
    Serial.println("❌ HTTPS request failed.");
  }

}
