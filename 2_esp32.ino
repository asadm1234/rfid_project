#include <esp_now.h>
#include <WiFi.h>
#include <LiquidCrystal.h>

// lcd wiring (rs, en, d4, d5, d6, d7)
LiquidCrystal lcd(14, 32, 27, 26, 25, 33);

// contrast control
int contrast = 120;
const int contrastPin = 13;

// incoming esp-now message
struct Message {
  char line1[16];
  char line2[16];
};
Message incoming;

// rotary encoder
const int CLK = 34;
const int DT  = 35;
int lastCLK = 0;

void setup() {
  Serial.begin(115200);

  // lcd start
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("booting...");
  delay(400);

  // pwm for contrast
  ledcAttachPin(contrastPin, 1);
  ledcSetup(1, 5000, 8);
  ledcWrite(1, contrast);

  pinMode(CLK, INPUT);
  pinMode(DT,  INPUT);
  lastCLK = digitalRead(CLK);

  // esp-now setup
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    lcd.clear();
    lcd.print("espnow fail");
    return;
  }

  esp_now_register_recv_cb(receiveData);

  Serial.print("mac: ");
  Serial.println(WiFi.macAddress());
}

void receiveData(const uint8_t *mac, const uint8_t *data, int len) {
  memcpy(&incoming, data, sizeof(incoming));

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(incoming.line1);

  lcd.setCursor(0, 1);
  lcd.print(incoming.line2);

  Serial.print("recv: ");
  Serial.print(incoming.line1);
  Serial.print(" | ");
  Serial.println(incoming.line2);
}

void loop() {

  // rotary adjusts contrast
  int clk = digitalRead(CLK);
  int dt  = digitalRead(DT);

  if (clk != lastCLK) {
    if (dt != clk) contrast++;
    else contrast--;

    contrast = constrain(contrast, 0, 255);
    ledcWrite(1, contrast);
  }

  lastCLK = clk;
}
