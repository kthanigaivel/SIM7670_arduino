#include <HardwareSerial.h>
#include <Keypad.h>
#include "TFT_22_ILI9225.h"


#define TX 16  //esp32 serial pin
#define RX 17 //esp32 serial pin
HardwareSerial *SimSerial = &Serial1;  //init esp32 serial
//debug define
#define DebugStream Serial
#define DEBUG_PRINT(...) DebugStream.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) DebugStream.println(__VA_ARGS__)
typedef const __FlashStringHelper *FStringPtr;

//keypad init
const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 27, 14, 12, 13 };
byte colPins[COLS] = { 32, 33, 25, 26 };
static byte kpadState;
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);




#define TFT_LED 0           // 0 if wired to +5V directly
#define TFT_CS 2            // HSPI-SS0
#define TFT_RST 21          // IO 26
#define TFT_RS 19           // IO 25
#define TFT_SDI 23          // HSPI-MOSI
#define TFT_CLK 18          // HSPI-SCK
#define TFT_BRIGHTNESS 200  // Initial brightness of TFT backlight (optional)
TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_SDI, TFT_CLK, TFT_LED, TFT_BRIGHTNESS);
int x = 0, y = 90;

int16_t timeout = 7000;
char replybuffer[254];
char number[30] = { 0 };
char calltime[1];
char DATE[8];
char TIME[8];
String data[10] = { "0", "9", "9", "0", "0", "00/00/00", "00:00:00", "0000000000" };
char calltype[10] = { 0 };

char mobile[30] = { 0 };
int i = 0;

void setup() {
  delay(10000);
  Serial.begin(115200);
  SimSerial->begin(115200, SERIAL_8N1, TX, RX);
  SimSerial->onReceive(onreceive);
  tft.begin();
  tft.setFont(Terminal11x16);
  tft.setOrientation(0);
  tft.clear();
  if (!config()) {
    Serial.println(F("Couldn't find modem"));
    while (1)
      ;
  }
  Serial.println("Modem is READY");

  keypad.begin(makeKeymap(hexaKeys));
  keypad.addEventListener(keypadEvent);
}
void loop() {
  while (Serial.available()) {
    SimSerial->write(Serial.read());
  }
  char customKey = keypad.getKey();
  if (customKey) {
  }
}

boolean config() {
  sendData(F("AT"));
  sendData(F("AT+CRC=1"));
  sendData(F("AT+CLIP=1"));
  sendData(F("AT+CTZU=0"));
  sendData(F("AT+CTZU=1"));
  sendData(F("AT+CCLK?"));
  sendData(F("AT+CMGF=1"));
  sendData(F("ATE0"));
  return true;
}

void updatescreen() {
  // sendData(F("AT+CCLK?"));
  // sendData(F("AT+CLCC"));
  if (data[1] == "0" && data[2] == "2") { calltype[0] = 'a'; }
  if (data[1] == "0" && data[2] == "0") { calltype[0] = 'a'; }
  if (data[1] == "0" && data[2] == "6") { calltype[0] = 'b'; }
  if (data[1] == "1" && data[2] == "6") {
    calltype[0] = 'b';
    data[7] = data[3];
  }
  if (data[1] == "1" && data[2] == "6" && (!(data[4] == "0"))) {
    calltype[0] = 'd';
  }
  if (data[1] == "0" && data[2] == "6" && (!(data[4] == "0"))) {
    calltype[0] = 'd';
  }
  if (data[1] == "1" && data[2] == "4") { calltype[0] = 'c'; }
  if (data[1] == "1" && data[2] == "0") {
    calltype[0] = 'a';
    data[7] = data[3];
  }
  tft.setFont(Terminal11x16);
  tft.setOrientation(0);
  tft.clear();
  switch (calltype[0]) {
    case 'a':
      tft.drawText(5, 50, "Current call...", COLOR_CYAN);
      tft.drawText(5, 70, data[7], COLOR_GREEN);
      break;
    case 'b':
      tft.drawText(5, 50, "Disconnected...", COLOR_RED);
      tft.drawText(5, 70, data[7], COLOR_GREEN);
      break;
    case 'c':
      tft.drawText(5, 50, "Incoming call...", COLOR_BLUE);
      tft.drawText(5, 70, data[3], COLOR_GREEN);
      break;
    case 'd':
      tft.drawText(5, 50, "Disconnected", COLOR_YELLOW);
      tft.drawText(5, 70, data[7], COLOR_GREEN);
      tft.drawText(5, 90, "Call Duration");
      tft.drawText(5, 110, data[4]);
      break;
  }


  tft.drawText(tft.maxX() / 2, 0, data[6]);
  tft.drawText(5, 170, "Dial...");
  tft.drawText(5, 190, mobile);
}

void onreceive() {
  while (SimSerial->available() > 0) {
    while (readline(false) > 0) {
      debug();

      char *n = strstr(replybuffer, "+CCLK:");
      TIME[8] = { 0 };
      if (n) {
        strncpy(TIME, n + 17, 8);
        data[6] = strtok(TIME, ",");
      }

      char *o = strstr(replybuffer, "+CCLK:");
      DATE[8] = { 0 };
      if (o) {
        strncpy(DATE, o + 8, 17);
        data[5] = strtok(DATE, ",");
      }

      char *p = strstr(replybuffer, "+CLIP:");
      if (p) {
        strncpy(number, p + 8, 13);
        // DEBUG_PRINTLN(number);
        data[3] = number;
      }

      char *r = strstr(replybuffer, "+CLCC:");
      if (r) {
        for (uint8_t i = 0; i < 1; i++) {
          r = strchr(r, ',');
          r++;
        }
        DEBUG_PRINTLN(r[0]);
        data[1] = r[0];
      }

      char *q = strstr(replybuffer, "+CLCC:");
      if (q) {
        for (uint8_t i = 0; i < 2; i++) {
          q = strchr(q, ',');
          q++;
        }
        DEBUG_PRINTLN(q[0]);
        data[2] = q[0];
      }

      char *t = strstr(replybuffer, "+CLCC:");
      if (t) {
        for (uint8_t i = 0; i < 5; i++) {
          t = strchr(t, ',');
          t++;
        }
       // DEBUG_PRINTLN(t);
        data[7] = strtok(t,",");

        updatescreen();
      }
      char *s = strstr(replybuffer, "VOICE CALL: END: ");
      if (s) {
        strncpy(calltime, s + 17, 17);
        DEBUG_PRINTLN(calltime);
        data[4] = calltime;
        delay(5000);
        updatescreen();
      }

      if (strcmp(replybuffer, "+CRING: VOICE") == 0) {
        //  DEBUG_PRINTLN("INCOMing CAll");
      }
      if (strcmp(replybuffer, "VOICE CALL: END") == 0) {
        data[4] = "0";
        delay(5000);
        updatescreen();
      }

      if (strcmp(replybuffer, "VOICE CALL: BEGIN") == 0) {
        data[1] = "0";
        //  DEBUG_PRINTLN("CAll begin");
      }

      if (strcmp(replybuffer, "+SMS FULL") == 0) {
        sendData(F("AT+CMGD=1,4"));
        //  DEBUG_PRINTLN("INCOMing CAll");
      }
    }
  }
}

void debug() {
  DEBUG_PRINTLN(replybuffer);
}
void sendData(char *send) {
  SimSerial->println(send);
}
void sendData(FStringPtr send) {
  SimSerial->println(send);
}
void sendData(FStringPtr send, char suffix) {
  SimSerial->print(send);
  SimSerial->println(suffix);
}

void sendData(FStringPtr send, char suffix, int suffix1) {
  SimSerial->print(send);
  SimSerial->print(suffix);
  SimSerial->print(",");
  SimSerial->println(suffix1);
}
uint8_t readline(boolean multiline) {
  uint16_t replyidx = 0;
  while (timeout--) {
    if (replyidx >= 254) {
      DEBUG_PRINTLN(F("SPACE"));
      break;
    }
    while (SimSerial->available()) {
      char c = SimSerial->read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (replyidx == 0)  // the first 0x0A is ignored
          continue;
        if (!multiline) {
          timeout = 0;  // the second 0x0A is the end of the line
          break;
        }
      }
      replybuffer[replyidx] = c;
      // DEBUG_PRINT(c,HEX); DEBUG_PRINT("#"); DEBUG_PRINTLN(c);
      if (++replyidx >= 254)
        break;
    }
    if (timeout == 0) {
      //  DEBUG_PRINTLN(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  replybuffer[replyidx] = 0;  // null term
  return replyidx;
}

void keypadEvent(KeypadEvent key) {
  kpadState = keypad.getState();
  char keyVal = key;
  switch (kpadState) {
    case PRESSED:
      break;
    case HOLD:
      reset(key);
      break;
    case RELEASED:
      if (data[2] == "0") {
        dtmf(key);
        break;
      }
      call(key);
      break;
  }
}

void reset(char n) {
  switch (n) {
    case NO_KEY:
      break;
    case '*':
      tft.begin();
      updatescreen();
      break;
    case '#':
      abort();
      break;
    case 'B':
      break;
  }
}
void call(char n) {
  switch (n) {
    case NO_KEY:
      break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '0':
      mobile[i] = n + '\0';
      i++;
      if (mobile[0] == '0' && mobile[1] == '0') {
        i = 0;
        memset(mobile, 0, sizeof(mobile));
        data[7] = "0";
        updatescreen();
      }
      tft.drawText(5, 170, "Dial...");
      tft.drawText(5, 190, mobile);
      break;
    case '*':
      i = 0;
      memset(mobile, 0, sizeof(mobile));
      updatescreen();
      break;
    case '#':
      updatescreen();
      break;
    case 'A':
      if (data[2] == "4") {
        sendData(F("ATA"));
      }
      break;
    case 'B':
      updatescreen();
      break;
    case 'C':
      sendData(F("AT+CHUP"));
      i = 0;
      memset(mobile, 0, sizeof(mobile));
      delay(5000);
      updatescreen();
      break;
    case 'D':
      if (mobile[0] >= '0' && mobile[1] > '0') {
        char sendbuff[35] = "ATD";
        strncpy(sendbuff + 3, mobile, min(30, (int)strlen(mobile)));
        uint8_t x = strlen(sendbuff);
        sendbuff[x] = ';';
        sendData(sendbuff);
      }
  }
}

void dtmf(char n) {
  switch (n) {
    case NO_KEY:
      break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '0':
    case '*':
    case '#':
      sendData(F("AT+VTS="), n, 300);
      Serial.println(n);
      tft.drawChar(0, 190, n);
      break;
    case 'A':
      updatescreen();
      break;
    case 'B':
      updatescreen();
      break;
    case 'C':
      sendData(F("AT+CHUP"));
      i = 0;
      memset(mobile, 0, sizeof(mobile));
      delay(5000);
      updatescreen();
      break;
    case 'D':
      updatescreen();
      break;
  }
}
