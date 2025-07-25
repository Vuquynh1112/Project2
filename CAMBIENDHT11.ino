#define BLYNK_TEMPLATE_ID "TMPL6STHPYBb_"
#define BLYNK_TEMPLATE_NAME "Cau6"
#define BLYNK_AUTH_TOKEN "BGZ05E1vkPcezOT7p12pguEW3BWEIwce"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); 

#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define TRIG_PIN 18
#define ECHO_PIN 19

#define LED1 27
#define LED2 26
#define LED3 25
#define LED4 14

#define BUZZER_PIN 5
#define DISTANCE_THRESHOLD 10 // cm

// WiFi & Blynk
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

bool ledState1 = false;
bool ledState2 = false;
bool ledState3 = false;
bool ledState4 = false;

unsigned long lastSensorUpdate = 0;
const unsigned long sensorInterval = 5000; // 5 giây

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1; 
  return duration * 0.034 / 2.0;
}

BLYNK_WRITE(V0) { ledState1 = param.asInt(); digitalWrite(LED1, ledState1); }
BLYNK_WRITE(V1) { ledState2 = param.asInt(); digitalWrite(LED2, ledState2); }
BLYNK_WRITE(V2) { ledState3 = param.asInt(); digitalWrite(LED3, ledState3); }
BLYNK_WRITE(V3) { ledState4 = param.asInt(); digitalWrite(LED4, ledState4); }

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Blynk.begin(auth, ssid, pass);
  dht.begin();
  lcd.init();
  lcd.begin(16,2);
  lcd.backlight();
  lcd.clear();

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
  Blynk.run();

  if (millis() - lastSensorUpdate >= sensorInterval) {
    lastSensorUpdate = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float distance = measureDistance();

    if (!isnan(h) && !isnan(t)) {
      int temp_int = (int)t;
      int hum_int = (int)h;
      int dist_int = (distance != -1) ? (int)distance : -1;

      Blynk.virtualWrite(V4, temp_int);
      Blynk.virtualWrite(V5, hum_int);
      Blynk.virtualWrite(V10, dist_int);

      // Hiển thị trên LCD dòng 1: T:25C H:60% D:15
      lcd.setCursor(0, 0);
      lcd.print("T:");
      lcd.print(temp_int);


      lcd.print(" H:");
      lcd.print(hum_int);
    

      lcd.print(" D:");
      if (dist_int != -1) {
        lcd.print(dist_int);
        // Đảm bảo xóa số cũ còn sót (nếu số cũ nhiều chữ số hơn số mới)
        if (dist_int < 100) lcd.print(" "); // nếu dưới 3 chữ số, in thêm khoảng trắng
        if (dist_int < 10) lcd.print(" ");  // nếu dưới 2 chữ số, in thêm 1 khoảng trắng nữa
      } else {
        lcd.print("-- ");
      }

      lcd.setCursor(0, 1);
      if (dist_int != -1 && dist_int < DISTANCE_THRESHOLD) {
        lcd.print("Nguoi gan    ");  // hiện chữ Người gần
        digitalWrite(BUZZER_PIN, HIGH);
        Blynk.virtualWrite(V11, 1);
      } else {
        lcd.print("                "); // clear dòng 2 khi không gần
        digitalWrite(BUZZER_PIN, LOW);
        Blynk.virtualWrite(V11, 0);
      }

      Serial.print("Nhiet do: "); Serial.print(temp_int);
      Serial.print(" C, Do am: "); Serial.print(hum_int);
      Serial.print(" %, KC: "); Serial.print(dist_int);
      Serial.println(" cm");

    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Loi DHT11!");
      lcd.setCursor(0, 1);
      lcd.print("KT cam bien!");
    }
  }
}