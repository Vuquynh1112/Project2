#include <Adafruit_Fingerprint.h>

#define FINGER_RX 16  // RX từ cảm biến -> TX của ESP32
#define FINGER_TX 17  // TX từ cảm biến -> RX của ESP32

HardwareSerial mySerial(2);  // UART2
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("ESP32 Fingerprint Control"));

  mySerial.begin(57600, SERIAL_8N1, FINGER_RX, FINGER_TX);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println(F("✅ Cảm biến vân tay đã kết nối!"));
  } else {
    Serial.println(F("❌ Không tìm thấy cảm biến vân tay!"));
    while (1) delay(1);
  }

  Serial.println(F("\nLệnh:"));
  Serial.println(F("A - Đăng ký vân tay"));
  Serial.println(F("B - Kiểm tra vân tay"));
  Serial.println(F("C - Xóa vân tay"));
}

void loop() {
  if (Serial.available()) {
    char cmd = readCommand();
    clearSerialBuffer();  // Xóa buffer sau khi đọc lệnh

    if (cmd == 'A') {
      enrollMode();
    } else if (cmd == 'B') {
      verifyMode();
    } else if (cmd == 'C') {
      deleteMode();
    } else {
      Serial.println(F("❌ Lệnh không hợp lệ!"));
    }

    Serial.println(F("\nLệnh tiếp theo: A-Đăng ký, B-Kiểm tra, C-Xóa"));
  }
}

char readCommand() {
  while (true) {
    if (Serial.available()) {
      char c = toupper(Serial.read());
      if (c == 'A' || c == 'B' || c == 'C') {
        Serial.println(c);
        return c;
      }
    }
  }
}

int readNumber() {
  String input = "";
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (input.length() > 0) {
          Serial.println();
          return input.toInt();
        }
      } else if (isDigit(c)) {
        input += c;
        Serial.print(c);  // Echo ký tự vừa nhập
      }
    }
  }
}

void clearSerialBuffer() {
  while (Serial.available()) {
    Serial.read();
  }
}

void enrollMode() {
  Serial.println(F("Nhập ID (1-127) để đăng ký:"));
  clearSerialBuffer();
  int id = readNumber();

  if (id < 1 || id > 127) {
    Serial.println(F("❌ ID không hợp lệ!"));
    return;
  }

  if (enrollFingerprint(id)) {
    Serial.println(F("✅ Đăng ký thành công!"));
  } else {
    Serial.println(F("❌ Đăng ký thất bại!"));
  }
}

void deleteMode() {
  Serial.println(F("Nhập ID (1-127) cần xóa:"));
  clearSerialBuffer();
  int id = readNumber();

  if (id < 1 || id > 127) {
    Serial.println(F("❌ ID không hợp lệ!"));
    return;
  }

  int p = finger.deleteModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println(F("✅ Xóa thành công!"));
  } else {
    Serial.println(F("❌ Xóa thất bại!"));
  }
}

void verifyMode() {
  Serial.println(F("👉 Đặt ngón tay lên cảm biến để kiểm tra..."));
  int p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    Serial.println(F("❌ Không lấy được ảnh vân tay."));
    return;
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    Serial.println(F("❌ Không chuyển được ảnh sang template."));
    return;
  }

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    Serial.println(F("❌ Không tìm thấy vân tay khớp."));
    return;
  }

  Serial.print(F("✅ Đã tìm thấy ID: "));
  Serial.println(finger.fingerID);
  Serial.print(F("Độ tin cậy: "));
  Serial.println(finger.confidence);
}

bool enrollFingerprint(int id) {
  int p = -1;
  Serial.println(F("👉 Đặt ngón tay lần 1..."));
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) continue;
    if (p == FINGERPRINT_PACKETRECIEVEERR) return false;
    if (p != FINGERPRINT_OK) return false;
  }
  if (finger.image2Tz(1) != FINGERPRINT_OK) return false;

  Serial.println(F("👆 Nhấc ngón tay ra..."));
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  Serial.println(F("👉 Đặt ngón tay lần 2..."));
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) continue;
    if (p == FINGERPRINT_PACKETRECIEVEERR) return false;
    if (p != FINGERPRINT_OK) return false;
  }
  if (finger.image2Tz(2) != FINGERPRINT_OK) return false;

  if (finger.createModel() != FINGERPRINT_OK) return false;
  if (finger.storeModel(id) != FINGERPRINT_OK) return false;

  return true;
}
