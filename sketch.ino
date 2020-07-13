/*
  Vin (Voltage In)    ->  3.3V
  Gnd (Ground)        ->  Gnd
  SDA (Serial Data)   ->  A4 on Uno/Pro-Mini
  SCK (Serial Clock)  ->  A5 on Uno/Pro-Mini
*/

#include <BME280I2C.h>
#include <OLED_I2C.h>

#define PLOT_LEN      96
#define STORAGE_TIME  560

OLED  myOLED(SDA, SCL, 8);
BME280I2C bme;

// Temperature Oversampling Rate, Humidity Oversampling Rate, Pressure Oversampling Rate, Mode, Standby Time, Filter, SPI Enable, BME280 Address
// BME280I2C bme(0x1, 0x1, 0x1, 0x1, 0x5, 0x0, false, 0x77); // Version for SparkFun BME280

extern uint8_t BigNumbers[];
extern uint8_t SmallFont[];

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
byte shift_cnt = 0;
bool fastMode = true;

// Prototypes
void drawCol(int x, int y, int yn);


void setup() {
  myOLED.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);

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
  
  bool metric = true;
  uint8_t pressureUnit(0); // unit: B000 = Pa, B001 = hPa, B010 = Hg, B011 = atm, B100 = bar, B101 = torr, B110 = N/m^2, B111 = psi

  bme.read(pres, temp, hum, metric, pressureUnit);
  
  /*
   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_Pa);

   bme.read(pres, temp, hum, tempUnit, presUnit);
  */

  // temp -= 0.3; // correct temp
  pres /= 133.3; // convert to mmHg
  
  myOLED.setBrightness(10);
  myOLED.clrScr();
  myOLED.setFont(BigNumbers);
  myOLED.printNumF(temp, 1, 0 + shift_cnt, 0 + shift_cnt);
  myOLED.printNumI(round(hum), 89 + shift_cnt, 0 + shift_cnt);
  myOLED.printNumF(pres, 1, 42 + shift_cnt, 37 + shift_cnt);

  myOLED.setFont(SmallFont);
  myOLED.print("~C", 56 + shift_cnt, 0 + shift_cnt);
  myOLED.print("%", 119 + shift_cnt, 0 + shift_cnt);
  myOLED.print("MM", 113 + shift_cnt, 54 + shift_cnt);
  myOLED.update();

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
  delay(1000);

  /*
    Graph
  */

  if (wait_cnt > 3) {
    wait_cnt = 0;
    shift_cnt = (shift_cnt == 0) ? 3 : 0;
    
    myOLED.clrScr();

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

    int z = 0;
    int x = 25;
    for (int i = 0; i < PLOT_LEN; i++) {
      if (infoArr[i].temp == 0 && infoArr[i].hum == 0 && infoArr[i].pres == 0) continue;

      /*
        myOLED.drawLine(x, map(infoArr[i].temp, minTemp, maxTemp, 18, 0), x + 1, map(infoArr[i + 1].temp, minTemp, maxTemp, 18, 0));
        myOLED.drawLine(x, map(infoArr[i].hum, minHum, maxHum, 40, 22), x + 1, map(infoArr[i + 1].hum, minHum, maxHum, 40, 22));
        myOLED.drawLine(x, map(infoArr[i].pres, minPres, maxPres, 62, 44), x + 1, map(infoArr[i + 1].pres, minPres, maxPres, 62, 44));
      */

      drawCol(x, map(infoArr[i].temp, minTemp, maxTemp, 17, 0), 17);
      drawCol(x, map(infoArr[i].hum, minHum, maxHum, 40, 23), 40);
      drawCol(x, map(infoArr[i].pres, minPres, maxPres, 63, 46), 63);

      z++;
      if (z > 15) {z = 0; x++;}
      
      x++;
    }

    myOLED.update();

    delay(2000);
  }

  wait_cnt++;

}

void drawCol(int x, int y, int yn) {
  for (int i = y; i <= yn; i++) {
    myOLED.setPixel(x, i);
  }
}
