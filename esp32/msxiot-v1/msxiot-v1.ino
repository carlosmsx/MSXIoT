/*
 * MSX INTERFACE DECO+ESP32
 * Autor: Carlos Escobar
 * 2020
 * 
 * Placa: ESP32 Dev Module
 * Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
 */
 
#include "BluetoothSerial.h" 
#include <WiFi.h>
#include <HTTPClient.h>
#include <string.h>
#include "SPIFFS.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#define PIN_WAIT_EN 32
#define PIN_WAIT_ST 13
#define PIN_RD 39
#define PIN_A0 33
#define D0 15
#define D1 16
#define D2 17
#define D3 18
#define D4 19
#define D5 21
#define D6 22
#define D7 23

#define DATA_MODE(x) pinMode(D0, x); pinMode(D1, x); pinMode(D2, x); pinMode(D3, x); pinMode(D4, x); pinMode(D5, x); pinMode(D6, x); pinMode(D7, x)
#define ARM_WAIT digitalWrite(PIN_WAIT_EN, LOW)
#define RELEASE_WAIT digitalWrite(PIN_WAIT_EN, HIGH)

#define READ_FROM_MSX(data) \
  if (digitalRead(D0)==HIGH) data =  0x01;\
  if (digitalRead(D1)==HIGH) data |= 0x02;\
  if (digitalRead(D2)==HIGH) data |= 0x04;\
  if (digitalRead(D3)==HIGH) data |= 0x08;\
  if (digitalRead(D4)==HIGH) data |= 0x10;\
  if (digitalRead(D5)==HIGH) data |= 0x20;\
  if (digitalRead(D6)==HIGH) data |= 0x40;\
  if (digitalRead(D7)==HIGH) data |= 0x80
  
#define WRITE_TO_MSX(data) \
  digitalWrite(D0, data&0x01?HIGH:LOW);\
  digitalWrite(D1, data&0x02?HIGH:LOW);\
  digitalWrite(D2, data&0x04?HIGH:LOW);\
  digitalWrite(D3, data&0x08?HIGH:LOW);\
  digitalWrite(D4, data&0x10?HIGH:LOW);\
  digitalWrite(D5, data&0x20?HIGH:LOW);\
  digitalWrite(D6, data&0x40?HIGH:LOW);\
  digitalWrite(D7, data&0x80?HIGH:LOW)

static uint16_t waitting_for_data = 0;
static uint8_t status_register = 0;
static uint8_t buf[512];

QueueHandle_t r_queue;
File dsk;

/* Stores the handle of the task that will be notified when pin ST change */
static TaskHandle_t xTaskToNotify = NULL;

BluetoothSerial ESP_BT; //Object for Bluetooth

/*
  st_status: variable que indica el estado de la última operación
    7 6 5 4 3 2 1 0
    | | | | | | | \__ 
    | | | | | | \____
    | | | | | \______
    | | | | \________
    | | | \__________
    | | \____________
    | \______________ en alto indica que hay datos disponibles para leer desde el BUFFER (DATA_READY)
    \________________ en alto indica que la interfaz está ocupada (BUSY)
 */

#define BUSY 0x80
#define DATA_READY 0x40


String hexByte(uint8_t b)
{
  return b>=16 ? String(b, HEX) : "0"+String(b, HEX);
}

void IRAM_ATTR decoISR(void)
{
  if (digitalRead(PIN_WAIT_ST)==HIGH) //released
  {
    ARM_WAIT;
  }
  else
  {
    int A0 = digitalRead(PIN_A0);
    if (digitalRead(PIN_RD)==HIGH) 
    {
      uint8_t data=0;
      READ_FROM_MSX(data);
      if (A0==0) 
        xQueueSend(r_queue, &data, portMAX_DELAY);
      //else 
      //  readCommand(data);  
      //RELEASE_WAIT;
    }
    else
    {
      if (A0==1)
      {
        status_register++;
        DATA_MODE(OUTPUT);
        WRITE_TO_MSX(status_register);    
        RELEASE_WAIT;
      }
      else
        waitting_for_data++;
    }
  }
}

void setup() {
  Serial.begin(115200);

  //ESP_BT.begin("MSX-IoT BT"); //Name of your Bluetooth Signal
  
  pinMode(PIN_WAIT_ST, INPUT_PULLUP);
  pinMode(PIN_RD, INPUT_PULLUP);
  pinMode(PIN_WAIT_EN, OUTPUT);
  pinMode(PIN_A0, INPUT_PULLUP);

  r_queue = xQueueCreate(512, sizeof(uint8_t));

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  dsk = SPIFFS.open("/4K.DSK", "r+");

  Serial.print("MSX IoT 2020 corriendo en core "+String(xPortGetCoreID())+"\n");

  attachInterrupt(digitalPinToInterrupt(PIN_WAIT_ST), decoISR, CHANGE);
  DATA_MODE(INPUT);
  RELEASE_WAIT;
  ARM_WAIT;
}


uint8_t state=0;
uint8_t drive_number;
uint8_t n_sectors;
uint8_t media;
uint16_t address;
uint16_t idx;

void loop() {
  uint8_t data;

  switch(state)
  {
    case 0:
      xQueueReceive(r_queue, &data, portMAX_DELAY);
      if (data==0xF0)
        state = 10;
      else if (data==0xF1)
        state = 20;
      else
        Serial.print(char(data));
      RELEASE_WAIT;
      break;
    case 10:
      xQueueReceive(r_queue, &data, portMAX_DELAY);
      drive_number = data;
      state = 11;
      RELEASE_WAIT;
      break;
    case 11:
      xQueueReceive(r_queue, &data, portMAX_DELAY);
      n_sectors = data;
      state = 12;
      RELEASE_WAIT;
      break;
    case 12:
      xQueueReceive(r_queue, &data, portMAX_DELAY);
      media = data;
      state = 13;
      RELEASE_WAIT;
      break;
    case 13: 
      xQueueReceive(r_queue, &data, portMAX_DELAY);
      address = data<<8;
      state = 14;
      RELEASE_WAIT;
      break;
    case 14:
      xQueueReceive(r_queue, &data, portMAX_DELAY);
      address |= data;
      idx = 0;
      Serial.println("WR drive="+String(drive_number)+" n_sec="+String(n_sectors)+" med="+String(media,HEX)+" addr="+String(address));
      state = 15;
      RELEASE_WAIT;
      break;
    case 15:
      xQueueReceive(r_queue, &buf[idx], portMAX_DELAY);
      idx++;
      if (idx == 512)
      {
        dsk.seek(address * 512, SeekSet);
        dsk.write(buf, 512);
        idx = 0;
        address++;
        n_sectors--;  
        if (n_sectors == 0)
        {
          state = 0;
        }
      }
      RELEASE_WAIT;
      break;
    case 20:
      xQueueReceive(r_queue, &data, portMAX_DELAY);
      drive_number = data;
      state = 21;
      RELEASE_WAIT;
      break;
    case 21:
      xQueueReceive(r_queue, &data, portMAX_DELAY);
      n_sectors = data;
      state = 22;
      RELEASE_WAIT;
      break;
    case 22:
      xQueueReceive(r_queue, &data, portMAX_DELAY);
      media = data;
      state = 23;
      RELEASE_WAIT;
      break;
    case 23: 
      xQueueReceive(r_queue, &data, portMAX_DELAY);
      address = data<<8;
      state = 24;
      RELEASE_WAIT;
      break;
    case 24:
      xQueueReceive(r_queue, &data, portMAX_DELAY);
      address |= data;
      idx = 0;
      Serial.println("RD drive="+String(drive_number)+" n_sec="+String(n_sectors)+" med="+String(media,HEX)+" addr="+String(address));
      dsk.seek(address * 512, SeekSet);
      dsk.read(buf, 512);
      state = 25;
      RELEASE_WAIT;
      break;
    case 25:
      if (waitting_for_data > 0)
      {
        DATA_MODE(OUTPUT);
        WRITE_TO_MSX(buf[idx]);    

        waitting_for_data--;
        idx++;
        if (idx == 512)
        {
          address++;
          n_sectors--;  
          if (n_sectors == 0)
            state = 0;
          else
          { 
            dsk.seek(address * 512, SeekSet);
            dsk.read(buf, 512);
            idx = 0;
          }
        }
        RELEASE_WAIT;
      }
      break;
  }
}
