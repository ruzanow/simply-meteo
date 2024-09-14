/*
  Vin (Voltage In)    ->  3.3V
  Gnd (Ground)        ->  Gnd
  SDA (Serial Data)   ->  A4 on Uno/Pro-Mini
  SCK (Serial Clock)  ->  A5 on Uno/Pro-Mini
*/
#include "LowPower.h"
#include <Wire.h>
#include <BME280I2C.h>
#include <U8g2lib.h>

#define PLOT_LEN      96
#define STORAGE_TIME  480

static const unsigned char BigNumbers[] PROGMEM =
  "\22\1\4\4\5\5\1\1\6\24\30\1\377\26\0\15\0\0\0\0\0\2F(\22\16\347>\374\333\371\335"
  "\242d\7\213\16\16\17\12\0)\23\16\347>\374\343\300C\13\36K\264n\354|\17\10\0*\32\16\347"
  "\316\30\62\42D\30\21!\211\24\322\15\21,\23\201&\311\303\377\23\0+\64\25W\77\374\377WR\210"
  "\22\62f\304\230!$h\222\42\211\210\21R\214\20\42$\210\10!A\244\22!J\210(\21\242\204\210"
  "\22!J\210(\21\242\304\3\3,\13\6g>\374\241 \61b\0-\16\16\347>\374CT\252\320\303"
  "\177\7\0.\13\6g>\374\211\30\62\2\1/$\16\347\316\250@\42\304\10\22!D\224\10\351F\210"
  "\25\61N\204TBD\10\22#BP\250\361\360\377\6\0\60<\16\347\212\232\20(\202\210\60!d\330"
  "\220aC\206\15\31\66d\330\220aC\4\12\11\32\36D\320 \2\205\14\33\62l\310\260!\303\206\14"
  "\33\62l\210\10\23BB\240\10\243\36\4\0\61\26\16\347>\330\300b\347c\321\341\1\7\26;\37\213"
  "\16\17\34\0\62\37\16\347\212*\24\241L\210\235\217\5\241\10\243&\4\42\301c\347\255\10S!P\251"
  "\7\1\0\63\36\16\347\212*\24\241L\210\235\217\5\241\10\243\12E`\261\363\225\11A(\302\250\7\1"
  "\0\64(\16\347>\200\240A\4\12\31\66d\330\220aC\206\15\31\66d\330\20\201BB\240\10\243\12"
  "E`\261\363\261\350\360\300\1\65\37\16\347\212\232\20\210D\230\32;o\5\207@\245\12E`\261\363\225"
  "\11A(\302\250\7\1\0\66/\16\347\212\232\20\210D\230\32;o\5\207@\245&\4\212 \2\205\14"
  "\33\62l\310\260!\303\206\14\33\62l\210\10\23BB\240\10\243\36\4\0\67\30\16\347\212*\24\241L"
  "\210\235\217E\207\7\34X\354|,:<p\0\70>\16\347\212\232\20(\202\210\60!d\330\220aC"
  "\206\15\31\66d\330\220aC\4\12\11\201\42\214\232\20(\202\10\24\62l\310\260!\303\206\14\33\62l"
  "\310\260!\42L\10\11\201\42\214z\20\0\71.\16\347\212\232\20(\202\210\60!d\330\220aC\206\15"
  "\31\66d\330\220aC\4\12\11\201\42\214*\24\201\305\316W&\4\241\10\243\36\4\0\0\0\0\4\377"
  "\377\0";


U8G2_SH1106_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//U8G2_HX1230_96X68_F_3W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 12, /* data=*/ 11, /* cs=*/ 10, /* reset=*/ 8);

BME280I2C bme;
// Temperature Oversampling Rate, Humidity Oversampling Rate, Pressure Oversampling Rate, Mode, Standby Time, Filter, SPI Enable, BME280 Address
// BME280I2C bme(0x1, 0x1, 0x1, 0x1, 0x5, 0x0, false, 0x77); // Version for SparkFun BME280


struct {
  byte temp = 0;
  byte hum = 0;
  byte pres = 0;
} infoArr[PLOT_LEN];

struct {
  float temp = 0;
  float hum = 0;
  float pres = 0;
  int counter = 0;
} avrg;

byte wait_cnt = 0;
byte move_cnt = 0;
bool fastMode = true;

// Prototypes
void drawCol(int x, int y, int yn);


void setup() {
  u8g2.begin();
  u8g2.setContrast(32);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);

  Wire.begin();
  while (!bme.begin()) {
    // If bme280 not found - blink LED
    // Maybe you use BME280 from SparkFun with another address?
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
  }
  delay(500);
}

void loop() {

  float temp(NAN), hum(NAN), pres(NAN);
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_torr);

  bme.read(pres, temp, hum, tempUnit, presUnit);

  //temp -= 0.5; // correct temp

  /*
    bool metric = true;
    uint8_t pressureUnit(0); // unit: B000 = Pa, B001 = hPa, B010 = Hg, B011 = atm, B100 = bar, B101 = torr, B110 = N/m^2, B111 = psi

    bme.read(pres, temp, hum, metric, pressureUnit);

    pres /= 133.3; // convert to mmHg

     myOLED.printNumF(temp, 1, 0 + move_cnt, 0 + move_cnt);
     myOLED.printNumI(round(hum), 89 + move_cnt, 0 + move_cnt);
     myOLED.printNumF(pres, 1, 39 + move_cnt, 37 + move_cnt);
  */

  // arrows
  int8_t presChanged = 0;
  int normPres = round(pres * 2) - 1380;
  for (int i = PLOT_LEN - 1; i >= 0 ; i--) {
    if (infoArr[i].temp == 0 && infoArr[i].hum == 0 && infoArr[i].pres == 0) continue;
    if (normPres > infoArr[i].pres + 8) {
      presChanged = 1;
      break;
    }
    if (normPres < infoArr[i].pres - 8) {
      presChanged = -1;
      break;
    }
  }

  u8g2.clearBuffer();
  u8g2.setFont(BigNumbers);
  u8g2.firstPage();
  do {
    u8g2.setCursor(0 + move_cnt, 24 + move_cnt);
    u8g2.print(temp, 1);
    u8g2.print("*");

    u8g2.setCursor(84 + move_cnt, 24 + move_cnt);
    u8g2.print(round(hum));
    u8g2.print("/");

    u8g2.setCursor(42 + move_cnt, 61 + move_cnt);
    u8g2.print(pres, 1);
    u8g2.print("+");

    if (presChanged) {
      u8g2.setCursor(28 + move_cnt, 61 + move_cnt);
      u8g2.print(presChanged == 1 ? ")" : "(");
    }
    
  } while ( u8g2.nextPage() );

  avrg.temp += temp;
  avrg.hum += hum;
  avrg.pres += pres;
  avrg.counter++;

  if (fastMode && avrg.counter >= STORAGE_TIME) {
    fastMode = false;
    for (int i = 0; i < PLOT_LEN - 1; i++) {
      infoArr[i].temp = 0;
      infoArr[i].hum = 0;
      infoArr[i].pres = 0;
    }
  }

  if (fastMode || avrg.counter >= STORAGE_TIME) {
    if (avrg.counter >= STORAGE_TIME) {
      temp = avrg.temp / avrg.counter;
      hum = avrg.hum / avrg.counter;
      pres = avrg.pres / avrg.counter;
      avrg.temp = 0;
      avrg.hum = 0;
      avrg.pres = 0;
      avrg.counter = 0;
    }
    for (int i = 1; i < PLOT_LEN; i++) {
      infoArr[i - 1] = infoArr[i];
    }
    infoArr[PLOT_LEN - 1].temp = round(temp * 2) + 80;
    infoArr[PLOT_LEN - 1].pres = round(pres * 2) - 1380;
    infoArr[PLOT_LEN - 1].hum = round(hum);
  }
  //delay(1000);
  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);

  /*
    Graph
  */

  if (wait_cnt > 3) {
    wait_cnt = 0;
    move_cnt = (move_cnt == 0) ? 3 : 0;

    byte minTemp = 255;
    byte minHum = 255;
    byte minPres = 255;
    byte maxTemp = 0;
    byte maxHum = 0;
    byte maxPres = 0;

    for (int i = PLOT_LEN - 1; i >= 0 ; i--) {
      if (infoArr[i].temp == 0 && infoArr[i].hum == 0 && infoArr[i].pres == 0) break;

      if (infoArr[i].temp < minTemp) minTemp = infoArr[i].temp;
      if (infoArr[i].hum < minHum) minHum = infoArr[i].hum;
      if (infoArr[i].pres < minPres) minPres = infoArr[i].pres;

      if (infoArr[i].temp > maxTemp) maxTemp = infoArr[i].temp;
      if (infoArr[i].hum > maxHum) maxHum = infoArr[i].hum;
      if (infoArr[i].pres > maxPres) maxPres = infoArr[i].pres;
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_profont12_mn);
    u8g2.firstPage();
    do {
      u8g2.setCursor(0, 18);
      u8g2.print(round((minTemp - 80) / 2.0));
      u8g2.setCursor(0, 8);
      u8g2.print(round((maxTemp - 80) / 2.0));

      u8g2.setCursor(0, 41);
      u8g2.print(minHum);
      u8g2.setCursor(0, 31);
      u8g2.print(maxHum);

      u8g2.setCursor(0, 64);
      u8g2.print(round((minPres + 1380) / 2.0));
      u8g2.setCursor(0, 54);
      u8g2.print(round((maxPres + 1380) / 2.0));

      /*
          if (maxTemp - minTemp < 10) maxTemp = minTemp + 10;
          if (maxHum - minHum < 10) maxHum = minHum + 10;
          if (maxPres - minPres < 10) maxPres = minPres + 10;

          myOLED.setFont(SmallFont);
          myOLED.printNumI(round((minTemp - 80) / 2.0), 0, 11);
          myOLED.printNumI(round((maxTemp - 80) / 2.0), 0, 1);

          myOLED.printNumI(minHum, 0, 34);
          myOLED.printNumI(maxHum, 0, 24);

          myOLED.printNumI(round((minPres + 1380) / 2.0), 0, 57);
          myOLED.printNumI(round((maxPres + 1380) / 2.0), 0, 47);
      */

      int z = 0;
      int x = 25;
      for (int i = 0; i < PLOT_LEN; i++) {
        if (infoArr[i].temp == 0 && infoArr[i].hum == 0 && infoArr[i].pres == 0) continue;

        drawCol(x, map(infoArr[i].temp, minTemp, (maxTemp - minTemp < 10) ? minTemp + 10 : maxTemp, 17, 0), 17);
        drawCol(x, map(infoArr[i].hum, minHum, (maxHum - minHum < 10) ? minHum + 10 : maxHum, 40, 23), 40);
        drawCol(x, map(infoArr[i].pres, minPres, (maxPres - minPres < 10) ? minPres + 10 : maxPres, 63, 46), 63);

        z++;
        if (z > 15) {
          z = 0;
          x++;
        }

        x++;
      }
    } while ( u8g2.nextPage() );

    //delay(2000);
    LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
  }

  wait_cnt++;

}

void drawCol(int x, int y, int yn) {
  u8g2.drawVLine(x, y, yn-y+1);
/*
  for (int i = y; i <= yn; i++) {
    u8g2.drawPixel(x, i);
  }
*/
}
