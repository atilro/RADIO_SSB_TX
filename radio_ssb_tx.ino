/********************************************************************************************************
  rf generator fot rx e tx radio iu2paq
*********************************************************************************************************/
//v4 rotation
//V2.1 11/1

//Libraries
#include <Wire.h>                 //IDE Standard
#include <Rotary.h>               //Ben Buxton https://github.com/brianlow/Rotary
#include <si5351.h>               //Etherkit https://github.com/etherkit/Si5351Arduino
#include <Adafruit_GFX.h>         //Adafruit GFX https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h>     //Adafruit SSD1306 https://github.com/adafruit/Adafruit_SSD1306
#include <EEPROM.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_MOSI 11 //D1
#define OLED_CLK 13  //D0
#define OLED_DC 10
#define OLED_CS 8
#define OLED_RESET 9


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define XT_CAL_F 120400 //Si5351 calibration factor, adjust to get exatcly 10MHz. Increasing this value will decreases the frequency and vice versa.
#define tunestep 4 //Change the pin used by encoder push button if you want.
#define buttonb 6
#define  A3
#define pttrele 7
#define basefreq 7055000; 
#define PLLB_FREQ    87600000000ULL
Rotary r = Rotary(3, 2);

//Si5351 si5351;
Si5351 si5351(0x60); 
uint32_t freq ;
uint32_t freqold;
unsigned long fstep, cal, calp,vfo,vfop;
byte encoder = 1;
byte stp;
unsigned int period = 100; //millis display active
unsigned long time_now = 0; //millis display active
int band = 80;
int currB;
int mode;
boolean bfon=true;

String msg="I U 2 P A Q   73!";

ISR(PCINT2_vect) {
  char result = r.process();
  if (result == DIR_CW) set_frequency(1);
  else if (result == DIR_CCW) set_frequency(-1);
}

void set_frequency(short dir) {
  if (encoder == 1) { //Up/Down frequency
    if (dir == 1) {
      if (band == 0) {
        cal = cal - fstep;
      } else {
        if(band==1) {
         vfo = vfo - fstep; 
        } else {
        freq = freq - fstep;
        check_freq(-1);
      }
     }
    }
    if (dir == -1) {
      if (band == 0) {
        cal = cal + fstep;
      } else {
        if(band==1) {
         vfo = vfo + fstep; 
         Serial.println(freq);
         Serial.println(vfo);
        } else {
        freq = freq + fstep;
        check_freq(1);
      }
     }
    }
  }
}

void setup() {
  
  Serial.begin(19200);
  Wire.begin();

  //startup_text();
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setRotation(2); 
  display.setTextColor(WHITE);
  display.display();

  pinMode(2, INPUT);
  pinMode(3, INPUT);
  //pinMode(pttext, INPUT);
  pinMode(tunestep, INPUT_PULLUP);
  pinMode(buttonb, INPUT);
  pinMode(pttrele, OUTPUT);
  
  delay(500);

  EEPROM.get(0, band);
  if (band <= 0) {
    band = 80;
    EEPROM.put(0, band);
  }

  EEPROM.get(20, freq);
  if (freq <= 0) {
    freq = 7000000;
    EEPROM.put(20, freq);
    band = 40;
  }

  EEPROM.get(40, vfo);
   if (vfo <= 0) {
    EEPROM.put(40, vfo);
  }
  
 set_band();
 vfo = 4913250;
 mode=2;
  

  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_vcxo(PLLB_FREQ, 40);
  
  si5351.set_correction(XT_CAL_F, SI5351_PLL_INPUT_XO);
  si5351.drive_strength(SI5351_CLK0,SI5351_DRIVE_2MA);
  if (bfon) si5351.drive_strength(SI5351_CLK1,SI5351_DRIVE_2MA);
  //si5351.drive_strength(SI5351_CLK2,SI5351_DRIVE_2MA);
  
  si5351.output_enable(SI5351_CLK0,1);//vfo
   if (bfon) si5351.output_enable(SI5351_CLK1,1);//bfo
   else si5351.output_enable(SI5351_CLK1,0);
   
  si5351.output_enable(SI5351_CLK2,0);//non usata

  tunegen();

  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();

  stp = 4;
  setstep();
  layout();
  displayfreq();
}

void loop() {
 
  currB = digitalRead(buttonb);

  if (currB == HIGH) {

    delay(800);

    if (band == 80) {
      band = 40;
    } else {
      if (band == 40) {
        band = 20;
      } else {
        if (band == 20) {
          band = 15;
        } else {
          if (band == 15) {
            band = 11;
          } else {
            if (band == 11) {
              band = 10;
            } else {
              if (band == 10) {
                band = 00;
              } else {
                if (band == 00)
                  band = 01;
                else {
                  if (band == 01)
                    band = 02;
                  else {
                   if (band == 02)
                    band = 80; 
                  }
                }
              }
            }
          }
        }
      }
    }

    set_freq();
    EEPROM.put(0, band);
    display.clearDisplay();
    displayfreq();
    layout();
    delay(300);

  }

  if (freqold != freq) {
    time_now = millis();
    tunegen();
    freqold = freq;
    EEPROM.put(20, freq);
    
  }

  if (vfo != vfop) {
    time_now = millis();
    tunegen();
    vfop = vfo;
    EEPROM.put(40, vfo);
  }

  if (digitalRead(tunestep) == LOW) {
    time_now = (millis() + 300);
    setstep();
    delay(300);
  }

  if ((time_now + period) > millis()) {
    displayfreq();
    layout();
  }
}

void tunegen() {

  if (band == 80 or band == 40) si5351.set_freq((freq+vfo) * 100, SI5351_CLK0);
  else si5351.set_freq((freq - vfo) * 100, SI5351_CLK0);
   if (bfon) si5351.set_freq(vfo * 100, SI5351_CLK1);
  si5351.update_status();

}

void displayfreq() {

  unsigned int m = freq / 1000000;
  unsigned int k = (freq % 1000000) / 1000;
  unsigned int h = (freq % 1000) / 1;

  display.clearDisplay();
  display.setCursor(100, 1);
  display.print("MHz");

  char buffer[15] = "";
  if (m < 1) {
    display.setCursor(56, 1); //-5
    sprintf(buffer, "%003d.%003d", k, h);
  } else if (m < 100) {
    display.setCursor(20, 1);
    sprintf(buffer, "%2d.%003d.%003d", m, k, h);
  } else if (m >= 100) {
    unsigned int h = (freq % 1000) / 10;
    display.setCursor(20, 1);
    sprintf(buffer, "%2d.%003d.%02d", m, k, h);
  }
  display.print(buffer);
}

void setstep() {
  switch (stp) {
  case 1:
    stp = 2;
    fstep = 1;
    break;
  case 2:
    stp = 3;
    fstep = 10;
    break;
  case 3:
    stp = 4;
    fstep = 100;
    break;
  case 4:
    stp = 5;
    fstep = 1000;
    break;
  case 5:
    stp = 6;
    fstep = 10000;
    break;
  case 6:
    stp = 1;
    fstep = 100000;
    break;
  }
}

void layout() {

  display.setTextColor(WHITE);
  display.drawLine(0, 0, 127, 0, WHITE);
  display.drawLine(0, 9, 127, 9, WHITE);
  display.drawLine(0, 20, 127, 20, WHITE);
  display.drawLine(0, 43, 127, 43, WHITE);
  display.drawLine(105, 24, 105, 39, WHITE);
  display.drawLine(87, 24, 87, 39, WHITE);
  display.drawLine(87, 48, 87, 63, WHITE);
  display.drawLine(15, 55, 82, 55, WHITE);

  display.setCursor(17, 10); //+5
  display.print(">>>");
  if (stp == 2) display.print("       |");
  if (stp == 3) display.print("      | ");
  if (stp == 4) display.print("     |  ");
  if (stp == 5) display.print("   |  ");
  if (stp == 6) display.print("  |   ");
  if (stp == 1) display.print("|    ");
  if (band >= 10) {
    display.setCursor(0, 24);
    display.print("BAND -       ");
    display.setCursor(40, 24);
    display.print(band);
    display.setCursor(55, 24);
    display.print("m");
  } else {
    display.setCursor(00, 24);
    if (band == 0)
      display.print("Calibr. Freq.");
    else {
      if (band==1) {
        freq=basefreq;
        tunegen();
        Serial.println(freq);
        Serial.println(vfo
        );
        display.print("Calibraz. IF");      
      }
    }
  }
  display.setCursor(92, 24);
 
  display.print("TX");
  display.setCursor(88, 10);
  display.print("        ");
  if (band == 1) {
    display.setTextSize(1);
    display.setCursor(84, 10);
    display.print(vfo);
  }
  
  display.display();
}

void set_freq() {
  if (band == 80) freq = 3500000;
  if (band == 40) freq = 7000000;
  if (band == 20) freq = 14000000;
  if (band == 15) freq = 21000000;
  if (band == 11) freq = 26500000;
  if (band == 10) freq = 28000000;
}

void set_band() {
  if (freq >= 3500000 and freq <= 3880000) band = 80;
  if (freq >= 7000000 and freq <= 7300000) band = 40;
  if (freq >= 14000000 and freq <= 14350000) band = 20;
  if (freq >= 21000000 and freq <= 21450000) band = 15;
  if (freq >= 26500000 and freq <= 27500000) band = 11;
  if (freq >= 28000000 and freq <= 28700000) band = 10;
}

void check_freq(int dir) {

  unsigned long fmi_ = 0;
  unsigned long fma_ = 0;

  if (band == 80) {
    fmi_ = 3500000;
    fma_ = 3580000;
  }
  if (band == 40) {
    fmi_ = 7000000;
    fma_ = 7350000;
  }
  if (band == 20) {
    fmi_ = 14000000;
    fma_ = 14380000;
  }
  if (band == 15) {
    fmi_ = 21000000;
    fma_ = 21650000;
  }
  if (band == 11) {
    fmi_ = 26500000;
    fma_ = 27500000;
  }
  if (band == 10) {
    fmi_ = 28000000;
    fma_ = 28800000;
  }
  if (dir == 1)
    if (freq > fma_) freq = freq - fstep;

  if (dir == -1)
    if (freq < fmi_) freq = freq + fstep;
}

/*
void startup_text() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.drawLine(0, 0, 127, 0, WHITE);
  display.setCursor(10, 1);
  display.print("TX Firmware V2.00");
}
*/

void setRotation(uint8_t rotation);
