/*
  TFT Touch Shield 2.0 examples - tftbmp

  loovee
  2013-1-21

  this demo can show specified bmp file in root Directory of SD card
  please ensure that your image file is 320x240 size.

  change __Gnfile_num and __Gsbmp_files to display your image
*/


#include <SD.h>
#include <SPI.h>
#include <Streaming.h>

#include "TFTv2.h"

#include <Servo.h>

#if 1
#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <NfcAdapter.h>

PN532_SPI pn532spi(SPI, 9);
NfcAdapter nfc = NfcAdapter(pn532spi);
#else

#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
#endif


#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            3

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      12

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


#define MAX_BMP         12                      // bmp file num
#define FILENAME_LEN    20                      // max file name length

#define ARIES   0x3031
#define TAURUS  0x3032
#define GEMINI  0x3033
#define CANCER  0x3034
#define LEO     0x3035
#define VIRGO   0x3036
#define LIBRA   0x3037
#define SCORPIO 0x3038
#define SAGITTARIUS 0x3039
#define CAPRICORN   0x3130
#define AQUARIUS    0x3131
#define PISCES      0x3132

#define STATE_INIT 0x01
#define STATE_CHECK 0x02
#define STATE_OPEN 0x03

int state = STATE_INIT;
int constellation[12];

const int PIN_SD_CS = 4;                        // pin of sd card

const int __Gnbmp_height = 320;                 // bmp hight
const int __Gnbmp_width  = 240;                 // bmp width

unsigned char __Gnbmp_image_offset  = 0;        // offset


int __Gnfile_num = MAX_BMP;                           // num of file

char __Gsbmp_files[MAX_BMP][FILENAME_LEN] =           // add file name here
{
"1.bmp",
"2.bmp",
"3.bmp",
"4.bmp",
"5.bmp",
"6.bmp",
"7.bmp",
"8.bmp",
"9.bmp",
"10.bmp",
"11.bmp",
"12.bmp",
};


File bmpFile;

Servo myservo;

void setup()
{

    Serial.begin(115200);

    pinMode(PIN_SD_CS,OUTPUT);
    digitalWrite(PIN_SD_CS,HIGH);

    Tft.TFTinit();

    Sd2Card card;
    card.init(SPI_FULL_SPEED, PIN_SD_CS);

    if(!SD.begin(PIN_SD_CS))
    {
        Serial.println("failed!");
        while(1);                               // init fail, die here
    }

    Serial.println("SD OK!");

    TFT_BL_ON;

    Serial.println("NDEF Reader");
    nfc.begin();

    pixels.begin();

    myservo.attach(21);
}

void loop()
{
  Serial.println("\nScan a NFC tag\n");
  nfc.begin();
  if (nfc.tagPresent())
  {
      NfcTag tag = nfc.read();
      tag.print();
      if(tag.hasNdefMessage()) {
          NdefRecord record = tag.getNdefMessage().getRecord(0);
          int len = record.getPayloadLength();
          byte payload[30];
          record.getPayload(payload);
          Serial.println(payload[len-2], HEX);
          Serial.println(payload[len-1], HEX);

          int code = (payload[len-2] << 8) + payload[len-1];
          Serial.println(code, HEX);
          check_constellation(code);
      }
  }

  state_switch(state);

  delay(1000);
}


// TFT display-----
void display_constellation(int index) {
  bmpFile = SD.open(__Gsbmp_files[index]);
  if (! bmpFile)
  {
      Serial.println("didnt find image");
      while (1);
  }

  if(! bmpReadHeader(bmpFile))
  {
      Serial.println("bad bmp");
      return;
  }

  bmpdraw(bmpFile, 0, 0);
  bmpFile.close();
}
/*********************************************/
// This procedure reads a bitmap and draws it to the screen
// its sped up by reading many pixels worth of data at a time
// instead of just one pixel at a time. increading the buffer takes
// more RAM but makes the drawing a little faster. 20 pixels' worth
// is probably a good place

#define BUFFPIXEL       60                      // must be a divisor of 240
#define BUFFPIXEL_X3    180                     // BUFFPIXELx3

void bmpdraw(File f, int x, int y)
{
    bmpFile.seek(__Gnbmp_image_offset);

    uint32_t time = millis();

    uint8_t sdbuffer[BUFFPIXEL_X3];                 // 3 * pixels to buffer

    for (int i=0; i< __Gnbmp_height; i++)
    {

        for(int j=0; j<(240/BUFFPIXEL); j++)
        {
            bmpFile.read(sdbuffer, BUFFPIXEL_X3);
            uint8_t buffidx = 0;
            int offset_x = j*BUFFPIXEL;

            unsigned int __color[BUFFPIXEL];

            for(int k=0; k<BUFFPIXEL; k++)
            {
                __color[k] = sdbuffer[buffidx+2]>>3;                        // read
                __color[k] = __color[k]<<6 | (sdbuffer[buffidx+1]>>2);      // green
                __color[k] = __color[k]<<5 | (sdbuffer[buffidx+0]>>3);      // blue

                buffidx += 3;
            }

            Tft.setCol(offset_x, offset_x+BUFFPIXEL);
            Tft.setPage(i, i);
            Tft.sendCMD(0x2c);

            TFT_DC_HIGH;
            TFT_CS_LOW;

            for(int m=0; m < BUFFPIXEL; m++)
            {
                SPI.transfer(__color[m]>>8);
                SPI.transfer(__color[m]);
            }

            TFT_CS_HIGH;
        }

    }

    Serial.print(millis() - time, DEC);
    Serial.println(" ms");
}

boolean bmpReadHeader(File f)
{
    // read header
    uint32_t tmp;
    uint8_t bmpDepth;

    if (read16(f) != 0x4D42) {
        // magic bytes missing
        return false;
    }

    // read file size
    tmp = read32(f);
    Serial.print("size 0x");
    Serial.println(tmp, HEX);

    // read and ignore creator bytes
    read32(f);

    __Gnbmp_image_offset = read32(f);
    Serial.print("offset ");
    Serial.println(__Gnbmp_image_offset, DEC);

    // read DIB header
    tmp = read32(f);
    Serial.print("header size ");
    Serial.println(tmp, DEC);


    int bmp_width = read32(f);
    int bmp_height = read32(f);

    if(bmp_width != __Gnbmp_width || bmp_height != __Gnbmp_height)      // if image is not 320x240, return false
    {
        return false;
    }

    if (read16(f) != 1)
    return false;

    bmpDepth = read16(f);
    Serial.print("bitdepth ");
    Serial.println(bmpDepth, DEC);

    if (read32(f) != 0) {
        // compression not supported!
        return false;
    }

    Serial.print("compression ");
    Serial.println(tmp, DEC);

    return true;
}

/*********************************************/
// These read data from the SD card file and convert them to big endian
// (the data is stored in little endian format!)

// LITTLE ENDIAN!
uint16_t read16(File f)
{
    uint16_t d;
    uint8_t b;
    b = f.read();
    d = f.read();
    d <<= 8;
    d |= b;
    return d;
}

// LITTLE ENDIAN!
uint32_t read32(File f)
{
    uint32_t d;
    uint16_t b;

    b = read16(f);
    d = read16(f);
    d <<= 16;
    d |= b;
    return d;
}

// NFC  Tag check
void check_constellation(int code) {
    switch (code) {
      case ARIES:
        blank_led(0);
        set_constellation(0);
        display_constellation(0);
      break;
      case TAURUS:
        blank_led(1);
        set_constellation(1);
        display_constellation(1);
      break;
      case GEMINI:
        blank_led(2);
        set_constellation(2);
        display_constellation(2);
      break;
      case CANCER:
        blank_led(3);
        set_constellation(3);
        display_constellation(3);
      break;
      case LEO:
        blank_led(4);
        set_constellation(4);
        display_constellation(4);
      break;
      case VIRGO:
        blank_led(5);
        set_constellation(5);
        display_constellation(5);
      break;
      case LIBRA:
        blank_led(6);
        set_constellation(6);
        display_constellation(6);
      break;
      case SCORPIO:
        blank_led(7);
        set_constellation(7);
        display_constellation(7);
      break;
      case SAGITTARIUS:
        blank_led(8);
        set_constellation(8);
        display_constellation(8);
      break;
      case CAPRICORN:
        blank_led(9);
        set_constellation(9);
        display_constellation(9);
      break;
      case AQUARIUS:
        blank_led(10);
        set_constellation(10);
        display_constellation(10);
      break;
      case PISCES:
        blank_led(11);
        set_constellation(11);
        display_constellation(11);
      break;
    }
}

// LED Strip ------
void set_led(int led_num)
{
  if (led_num > NUMPIXELS-1) return ;

  // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
  pixels.setPixelColor(led_num, pixels.Color(10,20,50)); // Moderately bright green color.
  pixels.show(); // This sends the updated pixel color to the hardware.

}

void clear_led(int led_num)
{
  if (led_num > NUMPIXELS-1) return ;
  // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
  pixels.setPixelColor(led_num, pixels.Color(0,0,0)); // Moderately bright green color.
  pixels.show(); // This sends the updated pixel color to the hardware.
}

void clear_all() {
  for (int i=0; i<12; i++) {
    pixels.setPixelColor(i, pixels.Color(0,0,0)); // Moderately bright green color.
  }
  pixels.show();
}

void blank_led(int led_num) {
  set_led(led_num);
  delay(500);
  clear_led(led_num);
  delay(500);
  set_led(led_num);
}

void happy_end(){
  theaterChase(pixels.Color(  0,   0, 127), 50);
}

void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, c);    //turn every third pixel on
      }
      pixels.show();

      delay(wait);

      for (int i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}


// lock ----------
void lock() {
  myservo.write(20);
}

void unlock() {
  myservo.write(90);
}


// constellation state
void constellation_init() {
  for (int i=0; i<12; i++)
    constellation[i] = 0;
}

bool get_constellation_all_pass() {
  for (int i=0; i<12; i++) {
    Serial.print("pin: ");
    Serial.print(i);
    Serial.print(" ");
    Serial.println(constellation[i]);
    if (constellation[i] == 0)
      return false;
  }
  return true;
}

void set_constellation(int index) {
  constellation[index] = 1;
}


// state swithc
void state_switch(int _state) {
  Serial.print("State: 0x0");Serial.println(state);
  switch (_state) {
    case STATE_INIT:
    {
      constellation_init();
      clear_all();
      lock();
      state = STATE_CHECK;
      break;
    }
    case STATE_CHECK:
    {
      if(get_constellation_all_pass()) {
        state = STATE_OPEN;
      }
      break;
    }
    case STATE_OPEN:
    {
      happy_end();
      unlock();
      break;
    }
  }
}
