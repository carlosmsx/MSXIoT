//para MEGA2560 definir ARDUINO_MEGA
//#define ARDUINO_MEGA
#undef ARDUINO_MEGA

#include <YetAnotherPcInt.h>
#include <SPI.h>
#include <SD.h>
#include "asm_helper.h"

#ifdef ARDUINO_MEGA
#define MSX_CS_PIN 10
#define MSX_A0_PIN 35
#define MSX_RD_PIN 33
#define MSX_EN_PIN 23
#define CS 53
#else

//PB0, PB1 
//PC0, PC1, PC2, PC3
//PD2, PD3, PD4, PD5, PD6, PD7 

#define MSX_CS_PIN A0 
#define MSX_A0_PIN A1
#define MSX_RD_PIN A2
#define MSX_EN_PIN A3
#define CS 10
#endif

String hexByte(uint8_t b)
{
  String res = "";
  if (b<16) res = "0";
  res = res + String(b, HEX);
  return res;
}

File dsk;
uint8_t state=0;
uint16_t total=0;
uint16_t idx=0;
uint8_t drive_number;
uint8_t n_sectors;
uint8_t media;
uint16_t sector;

void configDataBusAsInput()
{
#ifdef ARDUINO_MEGA
  DDRL = 0x00;
#else
  DDRB = DDRB & 0xfc; //puts bits 0-1 as inputs
  DDRD = DDRD & 0x03; //puts bits 2-7 as inputs
#endif
}

void configDataBusAsOutput()
{
#ifdef ARDUINO_MEGA
  DDRL = 0xff;
#else
  DDRB = DDRB | 0x03; //puts bits 0-1 as outputs
  DDRD = DDRD | 0xfc; //puts bits 2-7 as outputs
#endif
}

byte readDataBusByte()
{
#ifdef ARDUINO_MEGA
  return PINL;
#else
  return (PINB & 0x03) | (PIND & 0xfc);
#endif
}

byte writeDataBusByte(byte x)
{
#ifdef ARDUINO_MEGA
  PORTL = x; 
#else
  PINB = (PINB & 0xfc) | (x & 0x03);
  PIND = (PIND & 0x03) | (x & 0xfc);
#endif
}

void pinChanged_sd(const char* message, bool pinstate) {
  if (pinstate)
  { 
    //caso 2: la interrupción se produjo por la vuelta a HIGH de la entrada MSX_CS_PIN. Se produce al soltar la trampa
    //pongo data bus como entrada por defecto
    configDataBusAsInput();
    digitalWrite(MSX_EN_PIN, HIGH); //habilito
  }
  else
  {
    //caso 1: la interrupción se produjo por el cambio de MSX_CS_PIN a LOW. Se activa la trampa
    uint8_t d;
    delayMicroseconds(50);
    if (digitalRead(MSX_RD_PIN)==HIGH) //WR activo
    {
      d = readDataBusByte();
      Serial.print(hexByte(d));
    }
    
    if (state == 0xF0 || state == 0xF1)
    {
      switch (idx++)
      {
        case 0: drive_number = d; break;
        case 1: n_sectors = d;    break;
        case 2: media = d;        break;
        case 3: sector = d<<8;    break;
        case 4: 
          sector = sector | d; 
          total = n_sectors * 512; 
          Serial.println(" I/O drive="+String(drive_number)+" ns="+String(n_sectors)+" media="+String(media,HEX)+" sector="+String(sector,HEX));
          dsk = SD.open("4K.DSK", FILE_READ | FILE_WRITE);
          dsk.seek(sector*512);
          break;
        default:
          if (state == 0xF0)
          {
            //write byte
            Serial.print(hexByte(d));
            dsk.write(d);
            total--;
            if (total % 32 == 0)
              Serial.println();
            if (total == 0)
            {
              state = 0;
              //dsk.close();
            }
          }
          else if (state == 0xF1)
          {
            //read byte from SD
            
            uint8_t b = dsk.read();
            Serial.print(hexByte(b));
            configDataBusAsOutput();
            writeDataBusByte(b);
            total--;
            if (total % 32 == 0)
              Serial.println();
            if (total == 0)
            {
              state = 0;
              //dsk.close();
            }
          }
      }
    }
    else
    {
      switch (d) {
        case 0xF0:
        case 0xF1:
          Serial.print(hexByte(d));
          state = d;
          idx = 0;
          break;
        default:
          Serial.print(char(d));
      }
    }
    
    digitalWrite(MSX_EN_PIN, LOW); //suelto WAIT
  }
}

void pinChanged(const char* message, bool pinstate) 
{
  static byte col=0;
  if (pinstate)
  { 
    //Serial.println("caso2");
    //caso 2: la interrupción se produjo por la vuelta a HIGH de la entrada MSX_CS_PIN. Se produce al soltar la trampa
    //pongo data bus como entrada por defecto
    configDataBusAsInput();
    digitalWrite(MSX_EN_PIN, HIGH); //habilito
  }
  else
  {
    //Serial.println("caso1");
    //caso 1: la interrupción se produjo por el cambio de MSX_CS_PIN a LOW. Se activa la trampa
    uint8_t d;
    delayMicroseconds(50);
    if (digitalRead(MSX_RD_PIN)==HIGH) //WR activo
    {
      d = readDataBusByte();
      if ((d>'A' && d<'Z') || (d>'a' && d<'z') || (d>'0' && d<'9'))
        Serial.print((char)d);
      else
        Serial.print(hexByte(d));
      if (col++ == 32)
      {
        col = 0;
        Serial.println();
      }
    }
    else
    {
      configDataBusAsOutput();
      writeDataBusByte(123);
    }
    
    digitalWrite(MSX_EN_PIN, LOW); //suelto WAIT
  }
}

void pciSetup(byte pin)
{
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

void setup() {
  pinMode(MSX_CS_PIN, INPUT);
  pinMode(MSX_A0_PIN, INPUT);
  pinMode(MSX_RD_PIN, INPUT);
  pinMode(MSX_EN_PIN, OUTPUT);
  pinMode(CS, OUTPUT);

  configDataBusAsInput(); //DDRL = 0x00;
      
  Serial.begin(230400);  

  /*  
  Serial.print("Init\nSD");
  digitalWrite(CS, HIGH);

  while (!SD.begin(CS))
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("OK\nimagen");
  do
  {
    delay(1000);
    dsk = SD.open("4K.DSK", FILE_READ | FILE_WRITE);
    Serial.print(".");
  } while (!dsk);
  */
  Serial.println("OK");

  digitalWrite(MSX_EN_PIN, LOW); //deshabilito el decoder
  PcInt::attachInterrupt(MSX_CS_PIN, pinChanged, "Pin has changed to ", CHANGE);
  digitalWrite(MSX_EN_PIN, HIGH); //habilito el decoder
}
  
void loop()
{
  //Serial.println(digitalRead(MSX_CS_PIN));
}
