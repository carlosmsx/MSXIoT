#define NOTEST

#include <YetAnotherPcInt.h>
#include <SPI.h>
#include <SD.h>
//#include "asm_helper.h"

//DATA BUS: PB0, PB1, PD2, PD3, PD4, PD5, PD6, PD7 
//CONTROL: PC0, PC1, PC2, PC3

#define MSX_CS_PIN A0 
#define MSX_A0_PIN A1
#define MSX_RD_PIN A2
#define MSX_EN_PIN A3
#define CS 10

#define CMD_SENDSTR   0xE0
#define CMD_FSAVE     0xE1
#define CMD_WRITE     0xF0
#define CMD_READ      0xF1
#define CMD_INIHRD    0xF2
#define CMD_INIENV    0xF3
#define CMD_DRIVES    0xF4
#define CMD_DSKCHG    0xF5
#define CMD_CHOICE    0xF6
#define CMD_DSKFMT    0xF7
#define CMD_OEMSTAT   0xF8
#define CMD_MTOFF     0xF9
#define CMD_GETDPB    0xFA

#define CMD_PARAM__DRIVE_NUMBER   0
#define CMD_PARAM__N_SECTORS      1
#define CMD_PARAM__MEDIA          2
#define CMD_PARAM__SECTOR_H       3
#define CMD_PARAM__SECTOR_L       4
#define CMD_PARAM__ADDR_H         5
#define CMD_PARAM__ADDR_L         6
#define CMD_ST__READING_SEC       10
#define CMD_ST__READ_CRC          11
#define CMD_ST__WRITING_SEC       20

String hexByte(uint8_t b)
{
  String res = "";
  if (b<16) res = "0";
  res = res + String(b, HEX);
  return res;
}

//File dsk[2];
File dsk;
volatile uint8_t _stat=0;
//volatile uint16_t _idx=0;
volatile uint8_t _cmd;
volatile uint8_t _cmd_st;

volatile uint16_t _total=0;
volatile uint8_t _drive_number, _last_drv=0;
volatile uint8_t _n_sectors;
volatile uint8_t _media;
volatile uint8_t _sec_H;
volatile uint8_t _sec_L;
volatile uint8_t _addr_H;
volatile uint8_t _addr_L;
volatile uint16_t _sector;
volatile uint16_t _address;
volatile uint32_t _sector_pos;
volatile uint16_t _idx_sec=0;
//uint8_t buf_sec[512];

String diskFile(uint8_t drive)
{
  if (drive == 0)
    return "MSXDOS.DSK";
  else
    return "TPASCAL.DSK";
}

void configDataBusAsInput()
{
  DDRB = DDRB & 0xfc; //puts bits 0-1 as inputs
  DDRD = DDRD & 0x03; //puts bits 2-7 as inputs
}

void configDataBusAsOutput()
{
  DDRB = DDRB | 0x03; //puts bits 0-1 as outputs
  DDRD = DDRD | 0xfc; //puts bits 2-7 as outputs
}

byte readDataBusByte()
{
  return (PINB & 0x03) | (PIND & 0xfc);
}

byte writeDataBusByte(byte x)
{
  PORTB = (PORTB & 0xfc) | (x & 0x03);
  PORTD = (PORTD & 0x03) | (x & 0xfc);
}

void processCommand(uint8_t command)
{
  _cmd = command;

  //resincronizo datos a recibir
  //_idx = 0;  

  switch (_cmd)
  {
    case CMD_SENDSTR:
      break;
    case CMD_FSAVE:
      Serial.println("CMD_FSAVE");
      break;
    case CMD_WRITE:
      _cmd_st = CMD_PARAM__DRIVE_NUMBER;
      Serial.println("CMD_WRITE");
      break;
    case CMD_READ:
      _cmd_st = CMD_PARAM__DRIVE_NUMBER;
      Serial.println("CMD_READ");
      break;
    case CMD_INIHRD:
      Serial.println("CMD_INIHRD");
      break;
    case CMD_INIENV:
      Serial.println("CMD_INIENV");
      break;
    case CMD_DRIVES:
      Serial.println("CMD_DRIVES");
      break;
    case CMD_DSKCHG:
      Serial.println("CMD_DSKCHG");
      break;
    case CMD_CHOICE:
      Serial.println("CMD_CHOICE");
      break;
    case CMD_DSKFMT:
      Serial.println("CMD_DSKFMT");
      break;
    case CMD_OEMSTAT:
      Serial.println("CMD_OEMSTAT");
      break;
    case CMD_MTOFF:
      Serial.println("CMD_MTOFF");
      break;
    case CMD_GETDPB:
      Serial.println("CMD_GETDPB");
      break;
    default:
      Serial.println("UNKNOWN COMMAND "+String(hexByte(_cmd)));
  }
}

void processData(uint8_t data)
{
  switch (_cmd)
  {
    case CMD_SENDSTR:
      Serial.print(char(data));
      break;
    case CMD_FSAVE:
      break;
    case CMD_WRITE:
    case CMD_READ:
      //switch (_idx++)
      switch (_cmd_st)
      {
        case CMD_PARAM__DRIVE_NUMBER: 
          _drive_number = data; 
          _cmd_st = CMD_PARAM__N_SECTORS; 
          break;
        case CMD_PARAM__N_SECTORS: 
          _n_sectors = data; 
          _cmd_st = CMD_PARAM__MEDIA; 
          break;
        case CMD_PARAM__MEDIA: 
          _media = data; 
          _cmd_st = CMD_PARAM__ADDR_H;
          break;
        case CMD_PARAM__ADDR_H: 
          _addr_H = data;    
          _cmd_st = CMD_PARAM__ADDR_L;
          break;
        case CMD_PARAM__ADDR_L: 
          _addr_L = data;    
          _cmd_st = CMD_PARAM__SECTOR_H;
          break;
        case CMD_PARAM__SECTOR_H: 
          _sec_H = data;    
          _cmd_st = CMD_PARAM__SECTOR_L;
          break;
        case CMD_PARAM__SECTOR_L: 
          _sec_L = data;
          _sector = (uint16_t)_sec_H<<8 | _sec_L;
          _address = (uint16_t)_addr_H<<8 | _addr_L;
          _sector_pos = _sector * 512; 
          _total = _n_sectors * 512; 
          _idx_sec = 0;
          Serial.println(" I/O drive="+String(_drive_number)+" ns="+String(_n_sectors)+" media="+String(_media,HEX));
          Serial.println("     sector="+String(_sector)+ "["+ hexByte(_sec_H)+ hexByte(_sec_L) +"] total="+ String(_total)+ " cmd="+hexByte(_cmd));
          Serial.println("     address="+String(_address)+ "["+ hexByte(_addr_H)+ hexByte(_addr_L) +"] sector pos="+String(_sector_pos));

          if (_drive_number != _last_drv)
          {
            _last_drv = _drive_number;
            dsk.close();
            dsk = SD.open(diskFile(_drive_number), O_RDWR);
          }

          Serial.println(dsk.name());
          dsk.seek(_sector_pos);
          _cmd_st = (_cmd == CMD_READ ? CMD_ST__READING_SEC : CMD_ST__WRITING_SEC);
          break;
      }
      break;
  }
}

uint8_t dataToSend()
{
  switch(_cmd)
  {
    case CMD_READ:
      if ( _cmd_st == CMD_ST__READING_SEC )
      {
        //read byte from SD
        uint8_t b = dsk.read();
        Serial.print(hexByte(b));
        _total--;
        if (_total % 32 == 0)
          Serial.println();
        
        if (_total == 0)
        {
          _cmd = 0;
          //dsk.close();
        }
        
        _idx_sec++;
        /*
        if ( _idx_sec == 512 )
        {
          _idx_sec = 0;
          _cmd_st = CMD_ST__READ_CRC;
        }
        */
        return b;
      }
      /*
      else if ( _cmd_st == CMD_ST__READ_CRC )
      {
        if ( _total == 0)
        {
          _cmd = 0;
          _cmd_st = 0;
        }
        else
        {
          _cmd_st = CMD_ST__READING_SEC;
        }
        return _crc;
      }
      */
  }
  return 0;
}

#ifdef TEST
void pinChanged_sd(const char* message, bool pinstate)  //rutina de testeo
{
  if (pinstate)
  { 
    configDataBusAsInput();
    digitalWrite(MSX_EN_PIN, HIGH); //habilito
  }
  else
  {
    delayMicroseconds(10);
    Serial.print(digitalRead(MSX_A0_PIN));
    Serial.print(digitalRead(MSX_RD_PIN));
    
    uint8_t d = readDataBusByte();
    Serial.print(":");
    Serial.println(hexByte(d));
    digitalWrite(MSX_EN_PIN, LOW); //suelto WAIT
  }
}
#else
void pinChanged_sd(const char* message, bool pinstate)
{
  if (pinstate)
  { 
    configDataBusAsInput();
    digitalWrite(MSX_EN_PIN, HIGH); //habilito
  }
  else
  {
    delayMicroseconds(10);
    if (digitalRead(MSX_A0_PIN) == LOW)
    {
      //DATA REGISTER
      if (digitalRead(MSX_RD_PIN) == LOW)
      {
        //MSX lee un byte
        //delayMicroseconds(10);
        configDataBusAsOutput();
        writeDataBusByte(dataToSend());
      }
      else
      {
        //MSX envía un byte
        processData(readDataBusByte());
      }
    }
    else
    {
      //COMMAND/STATUS REGISTER
      if (digitalRead(MSX_RD_PIN) == LOW)
      {
        //MSX lee registro de estado
        configDataBusAsOutput();
        writeDataBusByte(_stat);
      }
      else
      {
        //MSX envía un comando
        processCommand(readDataBusByte());
      }
    }
    digitalWrite(MSX_EN_PIN, LOW); //suelto WAIT
  }
}
#endif

/*
void pinChanged_sd(const char* message, bool pinstate) {
  if (pinstate)
  { 
    //caso 2: la interrupción se produjo por la vuelta a HIGH de la entrada MSX_CS_PIN. Se produce al soltar la trampa
    //pongo data bus como entrada (alta imedancia) por defecto
    configDataBusAsInput();
    digitalWrite(MSX_EN_PIN, HIGH); //habilito
  }
  else
  {
    //caso 1: la interrupción se produjo por el cambio de MSX_CS_PIN a LOW. Se activa la trampa
    uint8_t d;
    delayMicroseconds(10);

    if (digitalRead(MSX_A0_PIN)==HIGH) //Command
    {
      if (digitalRead(MSX_RD_PIN)==HIGH) //WR activo
      {
        Serial.println("CMD");
        //recibe un comando
        state = readDataBusByte();
        idx = 0;
        switch(state)
        {
          case CMD_SENDSTR:
            Serial.println("CMD_SENDSTR");
            break;
          case CMD_FSAVE:
            Serial.println("CMD_FSAVE");
            break;
          case CMD_WRITE:
            break;
          case CMD_READ:
            break;
          case CMD_INIHRD:
            Serial.println("CMD_INIHRD");
            break;
          case CMD_INIENV:
            Serial.println("CMD_INIENV");
            break;
          case CMD_DRIVES:
            Serial.println("CMD_DRIVES");
            break;
          case CMD_DSKCHG:
            Serial.println("CMD_DSKCHG");
            break;
          case CMD_CHOICE:
            Serial.println("CMD_CHOICE");
            break;
          case CMD_DSKFMT:
            Serial.println("CMD_DSKFMT");
            break;
          case CMD_OEMSTAT:
            Serial.println("CMD_OEMSTAT");
            break;
          case CMD_MTOFF:
            Serial.println("CMD_MTOFF");
            break;
          case CMD_GETDPB:
            Serial.println("CMD_GETDPB");
            break;
          default:
            Serial.println("UNKNOWN COMMAND "+String(state));
        }
      }
      else
      {
        Serial.println("STAT");
        //envía el estado
        configDataBusAsOutput();
        writeDataBusByte(0); //TODO: mandar el estado correcto
      }
    }
    else //DATA I/O
    {
      if (digitalRead(MSX_RD_PIN)==HIGH) //WR activo
      {
        d = readDataBusByte();
        //Serial.print(hexByte(d));
      }

      if (state == CMD_SENDSTR)
      {
        Serial.print(char(d));
      }
      else if (state == CMD_WRITE || state == CMD_READ)
      {
        switch (idx++)
        {
          case 0: drive_number = d; break;
          case 1: n_sectors = d;    break;
          case 2: media = d;        break;
          case 3: sector = d<<8;    break;
          case 4: 
            sector = sector | d;
            sector = sector * 512; 
            total = n_sectors * 512; 
            idx_sec = 0;
            Serial.println(" I/O drive="+String(drive_number)+" ns="+String(n_sectors)+" media="+String(media,HEX));
            Serial.println("     sector="+String(sector,HEX)+ " total="+ String(total)+ " state="+hexByte(state));
  
            if (drive_number != last_drv)
            {
              last_drv = drive_number;
              dsk.close();
              dsk = SD.open(diskFile(drive_number), O_RDWR);
            }
  
            Serial.println(dsk.name());
            dsk.seek(sector);
            break;
          default:
            if (state == CMD_WRITE)
            {
              if (idx_sec % 32==0) Serial.println();
              //write byte
              dsk.write(d); //buf_sec[idx_sec++] = d;
              total--;
              idx_sec++;
  
              Serial.print(hexByte(d));
  
              if (idx_sec == 512)
              {
                //dsk.write(buf_sec, 512);
                dsk.flush();
                Serial.println("sector saved");
                idx_sec=0;
              }
  
              if (total==0)
              {
                dsk.flush();
                state = 0;
                Serial.println("all done");
              }
            }
            else if (state == CMD_READ)
            {
              //read byte from SD
              
              uint8_t b = dsk.read();
              //Serial.print(hexByte(b));
              configDataBusAsOutput();
              writeDataBusByte(b);
              total--;
              //if (total % 32 == 0)
              //  Serial.println();
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
        Serial.print(char(d));
      }
    }
    digitalWrite(MSX_EN_PIN, LOW); //suelto WAIT
  }
}
*/

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

  configDataBusAsInput();
      
  Serial.begin(250000);  

  Serial.print("Init\nSD");
  while (!SD.begin(CS))
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("OK\nimagen0:");
  Serial.print(diskFile(0));
  do
  {
    delay(1000);
    dsk = SD.open(diskFile(0), O_RDWR);
    Serial.print(".");
  } while (!dsk);

  Serial.println("OK");

  digitalWrite(MSX_EN_PIN, LOW); //deshabilito el decoder
  PcInt::attachInterrupt(MSX_CS_PIN, pinChanged_sd, "Pin has changed to ", CHANGE);
  digitalWrite(MSX_EN_PIN, HIGH); //habilito el decoder
}
  
void loop()
{
  /*
  String diskName = "";
  if (Serial.available())
  {
    char c = Serial.read();
    if (c == 13)
    {
      drive_B = diskName;
      diskName = "";
    }
    else  
      diskName += c;
  
  }
  */
}
