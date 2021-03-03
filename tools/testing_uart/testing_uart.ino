//MEGA2560
#include <YetAnotherPcInt.h>
#include <SD.h>
#include "asm_helper.h"

//const uint8_t D[] = {49,48,47,46,45,44,43,42};
#define MSX_CS_PIN 10
#define MSX_A0_PIN 35
#define MSX_RD_PIN 33
#define MSX_EN_PIN 23
#define CS 53

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

void pinChanged(const char* message, bool pinstate) {
  if (pinstate)
  { 
    //interrupción producida por la vuelta a HIGH de la entrada MSX_CS_PIN. Se produce al soltar la trampa
    //pongo data bus como entrada por defecto
    DDRL = 0x00;
    digitalWrite(MSX_EN_PIN, HIGH); //habilito
  }
  else
  {
    //interrupción producida por el cambio de MSX_CS_PIN a LOW. Se activa la trampa
    uint8_t d;
    delayMicroseconds(50);
    if (digitalRead(MSX_RD_PIN)==HIGH) //WR activo
    {
      d = PINL;
      //Serial.print(hexByte(d));
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
          //dsk = SD.open("4K.DSK", FILE_READ | FILE_WRITE);
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
            DDRL = 0xff;
            PORTL = b; //b;
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

  //dataInput();
  DDRL = 0x00;
      
  Serial.begin(230400);  
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
  Serial.println("OK");

  digitalWrite(MSX_EN_PIN, LOW); //deshabilito el decoder
  PcInt::attachInterrupt(MSX_CS_PIN, pinChanged, "Pin has changed to ", CHANGE);
  digitalWrite(MSX_EN_PIN, HIGH); //habilito el decoder
}
  
void loop()
{
}
