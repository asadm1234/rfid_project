#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>

uint8_t lcdMAC[] = {0x88, 0x57, 0x21, 0xB1, 0x8B, 0xC0};

#define SS_PIN 5
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);

// allowed tag uid
byte allowedUID[4] = {0x83, 0x95, 0x36, 0x2D};

// keypad stuff
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {32, 33, 25, 26};
byte colPins[COLS] = {27, 14, 12, 13};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// buzzer + leds
const int buzzerPin = 4;
const int redLED = 2;
const int greenLED = 15;

// esp-now message format
typedef struct Message {
  char line1[16];
  char line2[16];
} Message;

Message msg;

// quick function to update lcd esp32
void sendLCD(String l1, String l2) {
  memset(&msg, 0, sizeof(msg));
  l1.toCharArray(msg.line1, 16);
  l2.toCharArray(msg.line2, 16);
  esp_now_send(lcdMAC, (uint8_t*)&msg, sizeof(msg));
}

// checks if the scanned card matches the allowed one
bool checkUID(byte *uid) {
  for (int i = 0; i < 4; i++) {
    if (uid[i] != allowedUID[i]) return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);

  pinMode(buzzerPin, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);

  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, LOW);

  // tiny boot beep
  tone(buzzerPin, 900, 120);

  // esp-now setup
  WiFi.mode(WIFI_STA);
  esp_now_init();

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, lcdMAC, 6);
  esp_now_add_peer(&peerInfo);

  SPI.begin();
  rfid.PCD_Init();

  sendLCD("RFID Ready", "Scan Card");
}

void loop() {

  // check for rfid tag
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {

    sendLCD("Card Found", "");
    delay(400);

    // card not allowed
    if (!checkUID(rfid.uid.uidByte)) {
      sendLCD("BAD CARD", "Access Denied");

      // red flashes + low beep
      for (int i = 0; i < 3; i++) {
        digitalWrite(redLED, HIGH);
        tone(buzzerPin, 300, 120);
        delay(150);
        digitalWrite(redLED, LOW);
        delay(120);
      }

      delay(1200);
      sendLCD("RFID Ready", "Scan Card");
      return;
    }

    // card accepted, now ask for pin
    sendLCD("UID OK", "Enter PIN");
    delay(700);

    String typedPIN = "";
    String correctPIN = "1234";
    int attempts = 0;

    sendLCD("Enter PIN", "");

    // pin input loop
    while (true) {
      char key = keypad.getKey();

      if (key) {

        // number buttons
        if (key >= '0' && key <= '9') {
          typedPIN += key;

          String stars = "";
          for (int i = 0; i < typedPIN.length(); i++) stars += "*";
          sendLCD("Enter PIN", stars);
        }

        // '#' = enter
        if (key == '#') {

          // correct pin
          if (typedPIN == correctPIN) {

            // make sure red is off
            digitalWrite(redLED, LOW);

            sendLCD("ACCESS", "GRANTED");

            digitalWrite(greenLED, HIGH);
            tone(buzzerPin, 1500, 200);
            delay(250);
            tone(buzzerPin, 1800, 200);

            delay(1500);

            digitalWrite(greenLED, LOW);
            sendLCD("RFID Ready", "Scan Card");
            return;
          }

          // wrong pin
          attempts++;

          sendLCD("WRONG PIN", "Try Again");

          // red flashes + short beeps
          for (int i = 0; i < 3; i++) {
            digitalWrite(redLED, HIGH);
            tone(buzzerPin, 500, 120);
            delay(150);
            digitalWrite(redLED, LOW);
            delay(120);
          }

          // locked out after 3 tries
          if (attempts >= 3) {
            sendLCD("LOCKED OUT", "WAIT 10s");
            digitalWrite(redLED, HIGH);
            delay(10000);
            digitalWrite(redLED, LOW);

            sendLCD("RFID Ready", "Scan Card");
            return;
          }

          typedPIN = "";
          sendLCD("Enter PIN", "");
        }
      }
    }
  }
}
