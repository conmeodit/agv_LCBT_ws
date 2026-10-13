#include <Arduino.h>
#include <Wire.h>

// ================= KHAI BAO CHAN =================

// Encoder
const byte EN_TRAI_A = 2;
const byte EN_TRAI_B = 3;
const byte EN_PHAI_A = 18;
const byte EN_PHAI_B = 19;

// BTS7960 trai
const byte TRAI_RPWM = 5;
const byte TRAI_LPWM = 6;
const byte TRAI_REN = 22;
const byte TRAI_LEN = 23;

// BTS7960 phai
const byte PHAI_RPWM = 7;
const byte PHAI_LPWM = 8;
const byte PHAI_REN = 24;
const byte PHAI_LEN = 25;

// ================= CAU HINH =================

// Dao chieu dong co trai, giu nguyen dong co phai
const bool DAO_CHIEU_TRAI = true;
const bool DAO_CHIEU_PHAI = false;

// Toc do PWM thu nghiem
int tocdo = 100;

// Thoi gian chay moi lan thu
const unsigned long THOI_GIAN_TEST = 800;

// ================= ENCODER =================

volatile long xungTrai = 0;
volatile long xungPhai = 0;

volatile byte trangThaiTrai = 0;
volatile byte trangThaiPhai = 0;

const int8_t bangDem[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0
};

void capNhatTrai() {
    byte moi = (digitalRead(EN_TRAI_A) << 1) |
               digitalRead(EN_TRAI_B);

    xungTrai += bangDem[(trangThaiTrai << 2) | moi];
    trangThaiTrai = moi;
}

void capNhatPhai() {
    byte moi = (digitalRead(EN_PHAI_A) << 1) |
               digitalRead(EN_PHAI_B);

    xungPhai += bangDem[(trangThaiPhai << 2) | moi];
    trangThaiPhai = moi;
}

// ================= DIEU KHIEN DONG CO =================

bool dangChay = false;
unsigned long thoiDiemDung = 0;

void dungDongCo() {
    analogWrite(TRAI_RPWM, 0);
    analogWrite(TRAI_LPWM, 0);

    analogWrite(PHAI_RPWM, 0);
    analogWrite(PHAI_LPWM, 0);

    digitalWrite(TRAI_REN, LOW);
    digitalWrite(TRAI_LEN, LOW);

    digitalWrite(PHAI_REN, LOW);
    digitalWrite(PHAI_LEN, LOW);

    dangChay = false;
}

// PWM duong: quay tien
// PWM am: quay lui
// PWM = 0: dung
void dongCoTrai(int pwm) {

    if (pwm > 255) pwm = 255;
    if (pwm < -255) pwm = -255;

    if (DAO_CHIEU_TRAI) {
        pwm = -pwm;
    }

    digitalWrite(TRAI_REN, HIGH);
    digitalWrite(TRAI_LEN, HIGH);

    if (pwm > 0) {
        analogWrite(TRAI_RPWM, pwm);
        analogWrite(TRAI_LPWM, 0);
    }
    else if (pwm < 0) {
        analogWrite(TRAI_RPWM, 0);
        analogWrite(TRAI_LPWM, -pwm);
    }
    else {
        analogWrite(TRAI_RPWM, 0);
        analogWrite(TRAI_LPWM, 0);
    }
}

void dongCoPhai(int pwm) {

    if (pwm > 255) pwm = 255;
    if (pwm < -255) pwm = -255;

    if (DAO_CHIEU_PHAI) {
        pwm = -pwm;
    }

    digitalWrite(PHAI_REN, HIGH);
    digitalWrite(PHAI_LEN, HIGH);

    if (pwm > 0) {
        analogWrite(PHAI_RPWM, pwm);
        analogWrite(PHAI_LPWM, 0);
    }
    else if (pwm < 0) {
        analogWrite(PHAI_RPWM, 0);
        analogWrite(PHAI_LPWM, -pwm);
    }
    else {
        analogWrite(PHAI_RPWM, 0);
        analogWrite(PHAI_LPWM, 0);
    }
}

void dieuKhienXe(int pwmTrai, int pwmPhai) {

    // Dung truoc khi doi chieu
    dungDongCo();

    dongCoTrai(pwmTrai);
    dongCoPhai(pwmPhai);

    dangChay = true;
    thoiDiemDung = millis() + THOI_GIAN_TEST;
}

// ================= DOC ENCODER =================

void inEncoder() {

    long trai, phai;

    noInterrupts();
    trai = xungTrai;
    phai = xungPhai;
    interrupts();

    Serial.print("Encoder TRAI: ");
    Serial.print(trai);

    Serial.print(" | Encoder PHAI: ");
    Serial.println(phai);
}

void xoaEncoder() {

    noInterrupts();

    xungTrai = 0;
    xungPhai = 0;

    interrupts();

    Serial.println("Da xoa bo dem encoder");
}

// ================= DOC ADC =================

void docADC() {

    Serial.println("===== GIA TRI ADC =====");

    const char* ten[] = {
        "A0 - Dien ap",
        "A1 - Dong LM358",
        "A2 - Trai R_IS",
        "A3 - Trai L_IS",
        "A4 - Phai R_IS",
        "A5 - Phai L_IS"
    };

    for (int i = 0; i < 6; i++) {

        int raw = analogRead(A0 + i);

        float volt = raw * 5.0 / 1023.0;

        Serial.print(ten[i]);
        Serial.print(" | ADC: ");
        Serial.print(raw);

        Serial.print(" | Dien ap chan: ");
        Serial.println(volt, 3);
    }
}

// ================= KIEM TRA BNO055 =================

void kiemTraBNO055() {

    Serial.println("===== KIEM TRA BNO055 =====");

    bool timThay = false;

    for (byte diaChi = 1; diaChi < 127; diaChi++) {

        Wire.beginTransmission(diaChi);

        if (Wire.endTransmission() == 0) {

            Serial.print("Tim thay I2C: 0x");
            Serial.println(diaChi, HEX);

            if (diaChi == 0x28 || diaChi == 0x29) {

                Wire.beginTransmission(diaChi);
                Wire.write(0x00);

                if (Wire.endTransmission(false) == 0) {

                    if (Wire.requestFrom(diaChi, (byte)1) == 1) {

                        byte id = Wire.read();

                        Serial.print("BNO055 CHIP ID: 0x");
                        Serial.println(id, HEX);

                        if (id == 0xA0) {
                            Serial.println("BNO055 OK");
                            timThay = true;
                        }
                    }
                }
            }
        }
    }

    if (!timThay) {
        Serial.println("Chua xac nhan duoc BNO055");
    }
}

// ================= HC05 =================

void kiemTraHC05() {

    Serial2.println("TEST HC05 TU MEGA");

    Serial.println("Da gui du lieu qua HC05");
}

// ================= HIEN THI HUONG DAN =================

void huongDan() {

    Serial.println();
    Serial.println("===== KIEM TRA ROBOT AGV =====");

    Serial.println("w : Xe tien");
    Serial.println("s : Xe lui");
    Serial.println("a : Quay trai");
    Serial.println("d : Quay phai");

    Serial.println("q : Chi dong co trai tien");
    Serial.println("e : Chi dong co phai tien");

    Serial.println("z : Dung dong co");

    Serial.println("n : Doc encoder");
    Serial.println("m : Bat/tat tu dong hien thi encoder");
    Serial.println("c : Xoa encoder");

    Serial.println("i : Kiem tra BNO055");
    Serial.println("v : Doc ADC");
    Serial.println("b : Kiem tra HC05");

    Serial.println("h : Hien thi huong dan");

    Serial.println("==============================");
}

// ================= SETUP =================

bool tuDongEncoder = false;
unsigned long thoiGianEncoder = 0;

void setup() {

    Serial.begin(115200);
    Serial2.begin(9600);

    // Cau H
    pinMode(TRAI_RPWM, OUTPUT);
    pinMode(TRAI_LPWM, OUTPUT);
    pinMode(PHAI_RPWM, OUTPUT);
    pinMode(PHAI_LPWM, OUTPUT);

    pinMode(TRAI_REN, OUTPUT);
    pinMode(TRAI_LEN, OUTPUT);
    pinMode(PHAI_REN, OUTPUT);
    pinMode(PHAI_LEN, OUTPUT);

    dungDongCo();

    // Encoder
    pinMode(EN_TRAI_A, INPUT_PULLUP);
    pinMode(EN_TRAI_B, INPUT_PULLUP);

    pinMode(EN_PHAI_A, INPUT_PULLUP);
    pinMode(EN_PHAI_B, INPUT_PULLUP);

    trangThaiTrai =
        (digitalRead(EN_TRAI_A) << 1) |
        digitalRead(EN_TRAI_B);

    trangThaiPhai =
        (digitalRead(EN_PHAI_A) << 1) |
        digitalRead(EN_PHAI_B);

    attachInterrupt(
        digitalPinToInterrupt(EN_TRAI_A),
        capNhatTrai,
        CHANGE
    );

    attachInterrupt(
        digitalPinToInterrupt(EN_TRAI_B),
        capNhatTrai,
        CHANGE
    );

    attachInterrupt(
        digitalPinToInterrupt(EN_PHAI_A),
        capNhatPhai,
        CHANGE
    );

    attachInterrupt(
        digitalPinToInterrupt(EN_PHAI_B),
        capNhatPhai,
        CHANGE
    );

    // I2C
    Wire.begin();
    Wire.setClock(100000);

    analogReference(DEFAULT);

    delay(300);

    huongDan();
}

// ================= LOOP =================

void loop() {

    // Tu dong dung dong co
    if (dangChay &&
        (long)(millis() - thoiDiemDung) >= 0) {

        dungDongCo();
        Serial.println("Dong co da dung");
    }

    // Tu dong hien thi encoder
    if (tuDongEncoder &&
        millis() - thoiGianEncoder >= 500) {

        thoiGianEncoder = millis();

        inEncoder();
    }

    // Nhan du lieu HC05
    while (Serial2.available()) {

        char c = Serial2.read();

        Serial.print("HC05 nhan: ");
        Serial.println(c);
    }

    // Nhan lenh USB
    if (Serial.available()) {

        char lenh = Serial.read();

        switch (lenh) {

            case 'w':
                dieuKhienXe(tocdo, tocdo);
                Serial.println("XE TIEN");
                break;

            case 's':
                dieuKhienXe(-tocdo, -tocdo);
                Serial.println("XE LUI");
                break;

            case 'a':
                dieuKhienXe(-tocdo, tocdo);
                Serial.println("QUAY TRAI");
                break;

            case 'd':
                dieuKhienXe(tocdo, -tocdo);
                Serial.println("QUAY PHAI");
                break;

            case 'q':
                dieuKhienXe(tocdo, 0);
                Serial.println("DONG CO TRAI TIEN");
                break;

            case 'e':
                dieuKhienXe(0, tocdo);
                Serial.println("DONG CO PHAI TIEN");
                break;

            case 'z':
                dungDongCo();
                Serial.println("DA DUNG");
                break;

            case 'n':
                inEncoder();
                break;

            case 'm':
                tuDongEncoder = !tuDongEncoder;
                Serial.println(
                    tuDongEncoder ?
                    "BAT AUTO ENCODER" :
                    "TAT AUTO ENCODER"
                );
                break;

            case 'c':
                xoaEncoder();
                break;

            case 'i':
                kiemTraBNO055();
                break;

            case 'v':
                docADC();
                break;

            case 'b':
                kiemTraHC05();
                break;

            case 'h':
                huongDan();
                break;
        }
    }
}