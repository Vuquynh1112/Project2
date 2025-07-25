#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h>
#include <EEPROM.h>
#include <Adafruit_Fingerprint.h>

#define PIN_SG90 5
#define FINGER_RX 16
#define FINGER_TX 17

// PASSWORD
char password[6] = "11111";
char pass_def[6] = "11111";
char data_input[6];
unsigned char in_num = 0, index_t = 0, error_pass = 0;

char mode_changePass[6] = "*#01#";
char mode_resetPass[6] = "*#02#";
char mode_enroll[6] = "*#03#";
char mode_delete[6] = "*#04#";

const byte ROWS = 4, COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {14, 27, 26, 25};
byte colPins[COLS] = {33, 32, 18, 19};
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo sg90;
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// EEPROM
void writeEpprom(char data[]) {
  for (int i = 0; i < 5; i++) EEPROM.write(i, data[i]);
  EEPROM.commit();
}
void readEpprom() {
  for (int i = 0; i < 5; i++) password[i] = EEPROM.read(i);
}

// UTILITY
void clear_data_input() {
  for (int i = 0; i < 6; i++) data_input[i] = '\0';
  in_num = 0;
}
bool compareData(char a[], char b[]) {
  for (int i = 0; i < 5; i++) if (a[i] != b[i]) return false;
  return true;
}
bool isBufferFull(char data[]) {
  for (int i = 0; i < 5; i++) if (data[i] == '\0') return false;
  return true;
}
void getData() {
  char key = keypad.getKey();
  if (key) {
    data_input[in_num] = key;
    lcd.setCursor(5 + in_num, 1);
    lcd.print('*');
    in_num++;
    if (in_num >= 5) in_num = 0;
  }
}

// MAIN LOGIC
void checkPass() {
  getData();
  if (isBufferFull(data_input)) {
    if (compareData(data_input, password)) {
      lcd.clear(); clear_data_input(); index_t = 3;
    } else if (compareData(data_input, mode_changePass)) {
      lcd.clear(); clear_data_input(); index_t = 1;
    } else if (compareData(data_input, mode_resetPass)) {
      lcd.clear(); clear_data_input(); index_t = 2;
    } else if (compareData(data_input, mode_enroll)) {
      lcd.clear(); clear_data_input(); index_t = 5;
    } else if (compareData(data_input, mode_delete)) {
      lcd.clear(); clear_data_input(); index_t = 6;
    } else {
      error_pass++;
      lcd.clear(); lcd.print("SAI MK");
      delay(1000); lcd.clear(); clear_data_input();
      if (error_pass >= 3) index_t = 4;
    }
  }
}

bool checkFingerprint() {
  int p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    lcd.clear(); lcd.print("VTAY SAI");
    delay(1000); lcd.clear(); error_pass++;
    if (error_pass >= 3) index_t = 4;
    return false;
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    lcd.clear(); lcd.print("VTAY SAI");
    delay(1000); lcd.clear(); error_pass++;
    if (error_pass >= 3) index_t = 4;
    return false;
  }

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    lcd.clear(); lcd.print("VTAY SAI");
    delay(1000); lcd.clear(); error_pass++;
    if (error_pass >= 3) index_t = 4;
    return false;
  }

  lcd.clear(); lcd.print("VANTAY OK");
  delay(1000);
  return true;
}

void enrollFingerprint() {
  lcd.clear(); lcd.print("ID (1-127):");
  int id = 0;
  clear_data_input();
  lcd.setCursor(0, 1); lcd.print(id);

  while (true) {
    char k = keypad.getKey();
    if (k) {
      if (k >= '0' && k <= '9') {
        if (id < 100) {
          id = id * 10 + (k - '0');
          lcd.setCursor(0, 1);
          lcd.print("                ");
          lcd.setCursor(0, 1); lcd.print(id);
        }
      } else if (k == '*') {
        id /= 10;
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1); lcd.print(id);
      } else if (k == '#') break;
    }
  }

  if (id < 1 || id > 127) {
    lcd.clear(); lcd.print("ID INVALID");
    delay(2000); lcd.clear(); return;
  }

  if (finger.loadModel(id) == FINGERPRINT_OK) {
    lcd.clear(); lcd.print("ID DA DANG KY");
    delay(2000); lcd.clear(); return;
  }

  // Đặt tay lần 1
  lcd.clear(); lcd.print("Dat tay lan 1");
  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz(1) != FINGERPRINT_OK) {
    lcd.clear(); lcd.print("LOI LAN 1");
    delay(2000); lcd.clear(); return;
  }

  lcd.clear(); lcd.print("Nhat tay");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  // Đặt tay lần 2
  lcd.clear(); lcd.print("Dat tay lan 2");
  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz(2) != FINGERPRINT_OK) {
    lcd.clear(); lcd.print("LOI LAN 2");
    delay(2000); lcd.clear(); return;
  }

  // So sánh 2 lần đặt tay
  if (finger.createModel() != FINGERPRINT_OK) {
    lcd.clear(); lcd.print("KO GIONG NHAU");
    delay(2000); lcd.clear(); return;
  }

  // Lưu mẫu vân tay nếu đúng
  if (finger.storeModel(id) == FINGERPRINT_OK) {
    lcd.clear(); lcd.print("DK OK ID:"); lcd.print(id);
  } else {
    lcd.clear(); lcd.print("DK FAIL");
  }

  delay(2000); lcd.clear();
}


void deleteFingerprint() {
  lcd.clear(); lcd.print("DEL ID(1-127):");
  int id = 0;
  clear_data_input();
  while (true) {
    char k = keypad.getKey();
    if (k) {
      if (k >= '0' && k <= '9') {
        lcd.print(k);
        id = id * 10 + (k - '0');
      }
      if (k == '#') break;
    }
  }

  if (id < 1 || id > 127) {
    lcd.clear(); lcd.print("ID INVALID");
    delay(2000); lcd.clear(); return;
  }
  // KIỂM TRA ID CÓ TỒN TẠI KHÔNG
  if (finger.loadModel(id) != FINGERPRINT_OK) {
    lcd.clear(); lcd.print("ID CHUA DANG KY");
    delay(2000); lcd.clear(); return;
  }

  if (finger.deleteModel(id) == FINGERPRINT_OK) {
    lcd.clear(); lcd.print("XOA OK ID:"); lcd.print(id);
  } else {
    lcd.clear(); lcd.print("XOA FAIL");
  }
  delay(2000); lcd.clear();
}

void openDoor() {
  lcd.clear(); lcd.print("CUA MO");
  sg90.write(180); delay(5000); sg90.write(0);
  lcd.clear(); index_t = 0; error_pass = 0;
}

void error() {
  lcd.clear(); lcd.print("LOCK 1MIN");
  delay(60000); lcd.clear(); index_t = 0; error_pass = 0;
}

void changePassword() {
  lcd.clear(); lcd.print("NEW PASS:");
  clear_data_input();
  while (!isBufferFull(data_input)) getData();
  char new_pass[6];
  for (int i = 0; i < 5; i++) new_pass[i] = data_input[i];
  writeEpprom(new_pass);
  for (int i = 0; i < 5; i++) password[i] = new_pass[i];
  lcd.clear(); lcd.print("DOI MK OK");
  delay(2000); lcd.clear(); index_t = 0;
}

void resetPassword() {
  for (int i = 0; i < 5; i++) password[i] = pass_def[i];
  writeEpprom(password);
  lcd.clear(); lcd.print("RESET OK");
  delay(2000); lcd.clear(); index_t = 0;
}

void setup() {
  Serial.begin(115200);
  sg90.setPeriodHertz(50); sg90.attach(PIN_SG90, 500, 2400);
  lcd.init(); lcd.backlight(); lcd.print("SYSTEM INIT"); delay(2000); lcd.clear();
  EEPROM.begin(10); readEpprom();
  mySerial.begin(57600, SERIAL_8N1, FINGER_RX, FINGER_TX);
  finger.begin(57600);
  if (finger.verifyPassword()) Serial.println("Cam bien OK");
  else Serial.println("Cam bien loi");
}

void loop() {
  lcd.setCursor(0, 0); lcd.print("MK/VTAY: ");
  checkPass();

  // Chỉ gọi checkFingerprint() nếu có ngón tay đặt vào
  if (finger.getImage() == FINGERPRINT_OK) {
    if (checkFingerprint()) index_t = 3;
  }

  if (index_t == 1) changePassword();
  else if (index_t == 2) resetPassword();
  else if (index_t == 3) openDoor();
  else if (index_t == 4) error();
  else if (index_t == 5) { enrollFingerprint(); index_t = 0; }
  else if (index_t == 6) { deleteFingerprint(); index_t = 0; }
}

