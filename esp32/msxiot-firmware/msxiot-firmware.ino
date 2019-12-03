/*
 * MSX INTERFACE DECO+ESP32
 * Autor: Carlos Escobar
 */
#include "BluetoothSerial.h" 
#include <WiFi.h>
#include <string.h>

#define PIN_WAIT_EN 32
#define PIN_WAIT_ST 13
#define PIN_RD 39
#define PIN_A0 33
#define DATA_PINS {15,16,17,18,19,21,22,23} 

static volatile uint16_t st_idx;
static volatile uint16_t st_size;
static volatile uint8_t st_status;
static volatile uint8_t st_cmd;
//static volatile uint16_t st_counter=0;
//static volatile uint16_t st_counter2=0;
//static volatile uint16_t st_counter3=0;
char *st_buffer; //dynamically better

DRAM_ATTR const byte dmap[] = DATA_PINS;

/* Stores the handle of the task that will be notified when the
transmission is complete. */
static TaskHandle_t xTaskToNotify = NULL;

BluetoothSerial ESP_BT; //Object for Bluetooth

/*const*/ char* ssid = "ssid";
/*const*/ char* password = "password";

void IRAM_ATTR decoInt();
void IRAM_ATTR decoInt_old();

void IRAM_ATTR TaskISR( void * parameter) 
{
  uint32_t ulNotificationValue;

  while (true)
  {
    //waits for ISR notification
    ulNotificationValue = ulTaskNotifyTake( pdTRUE, portMAX_DELAY  );
    //st_counter2++;  
    if( ulNotificationValue == 1 )
    {
        /* The transmission ended as expected. */
        decoInt_old();
    }
  }
}

void setup() {
  st_buffer = new char[8192];
  Serial.begin(115200);

  ESP_BT.begin("MSX-IoT BT"); //Name of your Bluetooth Signal
  
  pinMode(PIN_WAIT_ST, INPUT_PULLUP);
  pinMode(PIN_RD, INPUT_PULLUP);
  pinMode(PIN_WAIT_EN, OUTPUT);
  pinMode(PIN_A0, INPUT_PULLUP);

  for (int i=0; i<8; i++)
    pinMode(dmap[i], INPUT);

  while (digitalRead(PIN_WAIT_ST)==LOW)
    digitalWrite(PIN_WAIT_EN, HIGH); //LOW);
  digitalWrite(PIN_WAIT_EN, LOW); //HIGH);

  String s = "MSX IoT 2019 corriendo en core "+String(xPortGetCoreID())+"\n";
  Serial.print(s);

  xTaskCreate(TaskISR, "decoInt", 1000, NULL, 10, &xTaskToNotify);
  
   /*
  WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Establishing connection to WiFi..");
  }
  Serial.println("conectado!!");
  */
  
  attachInterrupt(digitalPinToInterrupt(PIN_WAIT_ST), decoInt, FALLING);
}

void IRAM_ATTR readData(uint8_t data)
{
  if (st_size<8192) //<sizeof(_buffer))
  {
    st_buffer[st_size++]=data;
  }
  else
    st_status=0x01;
}

void IRAM_ATTR readCommand(uint8_t cmd)
{
  if (st_status & 0x80) 
    return;

  switch(cmd){
    case 0x1:
      st_idx = 0;
      st_size=0;
      st_status = 0x40;
      st_cmd = cmd;
      break;
    case 0x10:
      st_status = 0x80;
      st_cmd = cmd;
      break;
    case 0x11:
    case 0x14:
    case 0x15:
    case 0x16:
      st_status = 0x80;
      st_cmd = cmd;
      break;
    case 0x12: //wnet
    case 0x13: //wpass
    case 0x17: //wload
    case 0x18: //wbload
      st_status = 0x80;
      st_cmd = cmd;
      break;
  }
}

void IRAM_ATTR receiveByte(int A0)
{
  uint8_t data=0;
  uint8_t bit=1;
  for (int i=0; i<8; i++)
  {
    if (digitalRead(dmap[i])==HIGH)
      data|=bit;
    bit=bit<<1; 
  }

  if (A0==0) 
    readData(data);
  else 
    readCommand(data);  

  while (digitalRead(PIN_WAIT_ST)==LOW)
    digitalWrite(PIN_WAIT_EN, HIGH); //LOW);
  digitalWrite(PIN_WAIT_EN, LOW); //HIGH);
}

uint8_t IRAM_ATTR writeData()
{
  if (st_status==0x40 && st_idx<st_size) //<sizeof(_buffer))
  {
    return st_buffer[st_idx++];
  }
  st_status=0;
  return 0;
}

void IRAM_ATTR dispatchByte(int A0)
{    
  uint8_t data=0;
  if (A0==0)
  {
    data=writeData();
  }
  else
  {
    data=st_status;
  }
  
  uint8_t bit=1;
  for (int i=0; i<8; i++)
  {
    pinMode(dmap[i], OUTPUT);
    digitalWrite(dmap[i], data&bit?HIGH:LOW);
    bit=bit<<1; 
  }

  delayMicroseconds(5);

  digitalWrite(PIN_WAIT_EN, HIGH);
  while (digitalRead(PIN_WAIT_ST)==LOW);
    digitalWrite(PIN_WAIT_EN, HIGH); 

  delayMicroseconds(5);

  //for (int i=0; i<8; i++)
  //  pinMode(dmap[i], INPUT);

  digitalWrite(PIN_WAIT_EN, LOW); 
}

void IRAM_ATTR decoInt_old()
{
  //st_counter3++;  
  int A0 = digitalRead(PIN_A0);
  int RD = digitalRead(PIN_RD);
  
  if (RD==HIGH) 
    receiveByte(A0);
  else
    dispatchByte(A0);
}

void IRAM_ATTR decoInt()
{
  //st_counter++;  
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR( xTaskToNotify, &xHigherPriorityTaskWoken );
  if (xHigherPriorityTaskWoken) 
    portYIELD_FROM_ISR( );
}

//uint16_t ant=0, ant2=0, ant3=0;
void loop() {
  /*
  if (ant!=st_counter)
  {
    Serial.println("1:"+String(st_counter));
    ant=st_counter;
  }
  if (ant2!=st_counter2)
  {
    Serial.println(" 2:"+String(st_counter2));
    ant2=st_counter2;
  }
  if (ant3!=st_counter3)
  {
    Serial.println("  3:"+String(st_counter3));
    ant3=st_counter3;
  }
  */
  if (Serial.available()>0)
  {
    String s=Serial.readString();
    st_status=(byte)atoi(s.c_str());
  }
  switch(st_cmd)
  {
    case 0x10:
    case 0x11:
      {
        Serial.println("procesando comando "+String(st_cmd, HEX));
        int numSsid = WiFi.scanNetworks();
        
        if (numSsid == -1)
          st_size = sprintf(st_buffer, "no hay redes\r\n");
        else
        {
          st_size=0;
          for (int thisNet = 0; thisNet < numSsid; thisNet++) {
            if (st_cmd==0x11) 
              st_size += sprintf(&st_buffer[st_size], "_WNET(\"%s\")\r\n", WiFi.SSID(thisNet).c_str());
            else
              st_size += sprintf(&st_buffer[st_size], "%s\r\n", WiFi.SSID(thisNet).c_str());
          }
        }
        Serial.write((uint8_t*)st_buffer, st_size);
        st_cmd = 0;
        st_idx = 0;
        st_status = 0x40;
      }
      break;
    case 0x14:
    case 0x15:
    case 0x16:
      {
        st_size = sprintf(st_buffer, "procesando comando %02x\r\n", st_cmd);
        Serial.write((uint8_t*)st_buffer, st_size);
        st_cmd = 0;
        st_idx = 0;
        st_status = 0x40;
      }
      break;
    case 0x12:
      Serial.print("ssid=");
      for (int i=0; i<st_size; i++)
        Serial.print(String(st_buffer[i]));
      st_cmd = 0;
      st_status=0;
      break;    
    case 0x13:
      Serial.print("pass=");
      for (int i=0; i<st_size; i++)
        Serial.print(String(st_buffer[i]));
      st_cmd = 0;
      st_status=0;
      break;    
    case 0x17:
      Serial.print("load=");
      for (int i=0; i<st_size; i++)
        Serial.print(String(st_buffer[i]));
      st_cmd = 0;
      st_status=0;
      break;    
    case 0x18:
      Serial.print("bload=");
      for (int i=0; i<st_size; i++)
        Serial.print(String(st_buffer[i]));
      st_cmd = 0;
      st_status=0;
      break;    
  }
}
