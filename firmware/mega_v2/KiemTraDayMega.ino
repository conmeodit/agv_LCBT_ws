#include <Arduino.h>
#include <Wire.h>

// ========== KHAI BAO CHAN ==========
const byte EN_TRAI_A = 2, EN_TRAI_B = 3;
const byte EN_PHAI_A = 18, EN_PHAI_B = 19;
const byte TRAI_RPWM = 5, TRAI_LPWM = 6;
const byte PHAI_RPWM = 7, PHAI_LPWM = 8;
const byte TRAI_REN = 22, TRAI_LEN = 23;
const byte PHAI_REN = 24, PHAI_LEN = 25;

volatile long xungTrai = 0, xungPhai = 0;
volatile byte trangThaiTrai = 0, trangThaiPhai = 0;
const int8_t bangDem[16] = {0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0};

bool dangThuDongCo = false;
unsigned long thoiDiemDung = 0;

void capNhatTrai() {
  byte moi = (digitalRead(EN_TRAI_A) << 1) | digitalRead(EN_TRAI_B);
  xungTrai += bangDem[(trangThaiTrai << 2) | moi];
  trangThaiTrai = moi;
}
void capNhatPhai() {
  byte moi = (digitalRead(EN_PHAI_A) << 1) | digitalRead(EN_PHAI_B);
  xungPhai += bangDem[(trangThaiPhai << 2) | moi];
  trangThaiPhai = moi;
}

void dungDongCo() {
  analogWrite(TRAI_RPWM, 0);
  analogWrite(TRAI_LPWM, 0);
  analogWrite(PHAI_RPWM, 0);
  analogWrite(PHAI_LPWM, 0);
  digitalWrite(TRAI_REN, LOW);
  digitalWrite(TRAI_LEN, LOW);
  digitalWrite(PHAI_REN, LOW);
  digitalWrite(PHAI_LEN, LOW);
  dangThuDongCo = false;
}

void thuDongCo(bool trai, bool tien) {
  dungDongCo();
  const byte pwm = 75;                 // Muc thu thap (0..255)
  if (trai) {
    digitalWrite(TRAI_REN, HIGH);
    digitalWrite(TRAI_LEN, HIGH);
    analogWrite(tien ? TRAI_RPWM : TRAI_LPWM, pwm);
  } else {
    digitalWrite(PHAI_REN, HIGH);
    digitalWrite(PHAI_LEN, HIGH);
    analogWrite(tien ? PHAI_RPWM : PHAI_LPWM, pwm);
  }
  thoiDiemDung = millis() + 300;     // Tu dung sau 300 ms
  dangThuDongCo = true;
  Serial.print(F("THU MOTOR "));
  Serial.print(trai ? F("TRAI") : F("PHAI"));
  Serial.println(tien ? F(" - CHIEU 1") : F(" - CHIEU 2"));
}

void inEncoder() {
  long trai, phai;
  noInterrupts();
  trai = xungTrai;
  phai = xungPhai;
  interrupts();
  Serial.print(F("Encoder TRAI (D2,D3) = ")); Serial.println(trai);
  Serial.print(F("Encoder PHAI (D18,D19) = ")); Serial.println(phai);
  Serial.println(F("Quay tung banh bang tay va go e de kiem tra so xung thay doi."));
}

void docADC() {
  const char* ten[] = {"A0 - Chia ap", "A1 - Shunt LM358",
                        "A2 - Trai R_IS", "A3 - Trai L_IS",
                        "A4 - Phai R_IS", "A5 - Phai L_IS"};
  Serial.println(F("ADC (gia su AREF = AVcc ~ 5V; dien ap chi la uoc tinh):"));
  for (byte i = 0; i < 6; i++) {
    int raw = analogRead(A0 + i);
    float volt = raw * (5.0 / 1023.0);
    Serial.print(ten[i]);
    Serial.print(F(" | raw = ")); Serial.print(raw);
    Serial.print(F(" | V_ADC xap xi = ")); Serial.println(volt, 3);
  }
  Serial.println(F("Khong suy ra dien ap nguon/dong dien khi chua biet he so chia ap va gain."));
}

int docThanhGhi(byte diaChi, byte thanhGhi) {
  Wire.beginTransmission(diaChi);
  Wire.write(thanhGhi);
  if (Wire.endTransmission(false) != 0) return -1;
  if (Wire.requestFrom(diaChi, (byte)1) != 1) return -1;
  return Wire.read();
}

void kiemTraI2C() {
  Serial.println(F("Quet I2C tren D20 (SDA), D21 (SCL):"));
  int soThietBi = 0;
  bool thayBNO = false;
  for (byte diaChi = 1; diaChi < 127; diaChi++) {
    Wire.beginTransmission(diaChi);
    if (Wire.endTransmission() == 0) {
      soThietBi++;
      Serial.print(F("  Dia chi 0x"));
      if (diaChi < 16) Serial.print('0');
      Serial.print(diaChi, HEX);
      if (diaChi == 0x28 || diaChi == 0x29) {
        int id = docThanhGhi(diaChi, 0x00);
        Serial.print(F(" | BNO055 CHIP_ID: 0x"));
        if (id >= 0) Serial.print(id, HEX); else Serial.print(F("DOC LOI"));
        if (id == 0xA0) { Serial.print(F(" -> DUNG ID")); thayBNO = true; }
      }
      Serial.println();
    }
  }
  if (soThietBi == 0) Serial.println(F("KHONG THAY THIET BI I2C."));
  if (!thayBNO) Serial.println(F("CHUA XAC NHAN DUOC CHIP BNO055 (ID = 0xA0)."));
}

void inHuongDan() {
  Serial.println(F("\n=== KIEM TRA DAY ARDUINO MEGA ==="));
  Serial.println(F("h: Huong dan"));
  Serial.println(F("e: Doc encoder | c: Dat lai bo dem encoder"));
  Serial.println(F("a: Doc ADC A0..A5"));
  Serial.println(F("i: Quet I2C + kiem tra BNO055"));
  Serial.println(F("t: Gui chuoi kiem tra den HC-05 qua Serial2"));
  Serial.println(F("l/L: Motor trai chieu 1/2 (300 ms)"));
  Serial.println(F("r/R: Motor phai chieu 1/2 (300 ms)"));
  Serial.println(F("x: Dung tat ca dong co"));
  Serial.println(F("!!! KE CA HAI BANH LEN TRUOC KHI TEST MOTOR !!!"));
  Serial.println(F("Serial2 HC-05: Mega TX2=D16 -> RXD qua chia ap, RX2=D17 <- TXD"));
}

void setup() {
  // Uu tien dua cau H ve trang thai tat ngay khi khoi dong
  pinMode(TRAI_REN, OUTPUT); pinMode(TRAI_LEN, OUTPUT);
  pinMode(PHAI_REN, OUTPUT); pinMode(PHAI_LEN, OUTPUT);
  pinMode(TRAI_RPWM, OUTPUT); pinMode(TRAI_LPWM, OUTPUT);
  pinMode(PHAI_RPWM, OUTPUT); pinMode(PHAI_LPWM, OUTPUT);
  dungDongCo();

  pinMode(EN_TRAI_A, INPUT_PULLUP);
  pinMode(EN_TRAI_B, INPUT_PULLUP);
  pinMode(EN_PHAI_A, INPUT_PULLUP);
  pinMode(EN_PHAI_B, INPUT_PULLUP);
  trangThaiTrai = (digitalRead(EN_TRAI_A) << 1) | digitalRead(EN_TRAI_B);
  trangThaiPhai = (digitalRead(EN_PHAI_A) << 1) | digitalRead(EN_PHAI_B);
  attachInterrupt(digitalPinToInterrupt(EN_TRAI_A), capNhatTrai, CHANGE);
  attachInterrupt(digitalPinToInterrupt(EN_TRAI_B), capNhatTrai, CHANGE);
  attachInterrupt(digitalPinToInterrupt(EN_PHAI_A), capNhatPhai, CHANGE);
  attachInterrupt(digitalPinToInterrupt(EN_PHAI_B), capNhatPhai, CHANGE);

  analogReference(DEFAULT);
  Wire.begin();
  Wire.setClock(100000);
  Serial.begin(115200);              // USB voi Raspberry Pi
  Serial2.begin(9600);               // Baud HC-05 can khop voi module
  delay(300);
  inHuongDan();
}

void loop() {
  // Dat dau vong lap de dung motor dung han, khong can cho lenh moi
  if (dangThuDongCo && (long)(millis() - thoiDiemDung) >= 0) {
    dungDongCo();
    Serial.println(F("MOTOR DA DUNG"));
  }
  while (Serial2.available()) {
    char c = Serial2.read();
    Serial.print(F("HC05 nhan: "));
    Serial.println(c);
  }
  if (!Serial.available()) return;
  char lenh = Serial.read();
  if (lenh == '\n' || lenh == '\r') return;
  switch (lenh) {
    case 'h': inHuongDan(); break;
    case 'e': inEncoder(); break;
    case 'c':
      noInterrupts(); xungTrai = 0; xungPhai = 0; interrupts();
      Serial.println(F("Da reset dem encoder")); break;
    case 'a': docADC(); break;
    case 'i': kiemTraI2C(); break;
    case 't': Serial2.println(F("TEST HC05 TU MEGA"));
              Serial.println(F("Da gui TEST HC05 TU MEGA")); break;
    case 'l': thuDongCo(true, true); break;
    case 'L': thuDongCo(true, false); break;
    case 'r': thuDongCo(false, true); break;
    case 'R': thuDongCo(false, false); break;
    case 'x': dungDongCo(); Serial.println(F("DA DUNG MOTOR")); break;
    default: Serial.println(F("Lenh khong hop le, go h")); break;
  }
}
