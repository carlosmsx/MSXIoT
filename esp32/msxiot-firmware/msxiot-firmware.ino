/*
 * MSX INTERFACE DECO+ESP32
 * Autor: Carlos Escobar
 * 2019
 * 
 * Placa: ESP32 Dev Module
 * Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
 */
 
#include "BluetoothSerial.h" 
#include <WiFi.h>
#include <HTTPClient.h>
#include <string.h>
#include "SPIFFS.h"
#include "GDrive.h"

#define PIN_WAIT_EN 32
#define PIN_WAIT_ST 13
#define PIN_RD 39
#define PIN_A0 33
#define DATA_PINS {15,16,17,18,19,21,22,23} 

//32768+2
#define BUFSIZE 0x8002

#define CMD_NULL 0x00
#define CMD_SENDSTR 0x01
#define CMD_WLIST 0x10
#define CMD_WCLIST 0x11
#define CMD_WNET 0x12
#define CMD_WPASS 0x13
#define CMD_WCONN 0x14
#define CMD_WDISCON 0x15
#define CMD_WSTAT 0x16
#define CMD_WLOAD 0x17
#define CMD_WBLOAD 0x18
#define CMD_TCPSVR 0x19
#define CMD_TCPSEND 0x1A

#define CMD_FNAME 0x60
#define CMD_FSAVE 0x61
#define CMD_FLOAD 0x62
#define CMD_FFILES 0x63
#define CMD_LOADROM 0x64

#define CMD_CLOUDROM 0x70

#define CMD_GD_LST  0x80
#define CMD_GD_UPL  0x81
#define CMD_GD_DWL  0x82

static volatile uint16_t st_idx;
static volatile uint16_t st_size; //mantiene el tamaño del dato en el buffer
static volatile uint8_t st_cmd;   //indica el comando ejecutado

static volatile uint8_t st_status;
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

char *st_buffer; //dynamically better
char *st_ssid = NULL;
char *st_password = NULL;
char st_server[64];
char st_file_name[64];
static volatile uint16_t st_server_port;

DRAM_ATTR const byte dmap[] = DATA_PINS;

/* Stores the handle of the task that will be notified when the
transmission is complete. */
static TaskHandle_t xTaskToNotify = NULL;

BluetoothSerial ESP_BT; //Object for Bluetooth

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
  Serial.begin(115200);
  
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  /*
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file){
      Serial.print("FILE: ");
      Serial.println(file.name());
      file = root.openNextFile();
  }
  file.close();
  */
  
  st_buffer = new char[BUFSIZE+2];

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
    
  attachInterrupt(digitalPinToInterrupt(PIN_WAIT_ST), decoInt, FALLING);
}

void IRAM_ATTR readData(uint8_t data)
{
  if (st_size<BUFSIZE) //<sizeof(_buffer))
  {
    st_buffer[st_size++]=data;
  }
  else
    st_status=0x01;
}

void IRAM_ATTR readCommand(uint8_t cmd)
{
  if (st_status & BUSY) 
    return;

  switch(cmd){
    case CMD_SENDSTR:
      st_idx = 0;
      st_size=0;
      st_status = DATA_READY;
      st_cmd = cmd;
      break;
    case CMD_WLIST:
    case CMD_WCLIST:
    case CMD_WCONN:
    case CMD_WDISCON:
    case CMD_WSTAT:
    case CMD_WNET: 
    case CMD_WPASS:
    case CMD_WLOAD:
    case CMD_WBLOAD:
    case CMD_TCPSVR:
    case CMD_TCPSEND:
    case CMD_FSAVE:
    case CMD_FLOAD:
    case CMD_FFILES:
    case CMD_FNAME:
    case CMD_LOADROM:
    case CMD_CLOUDROM:
    case CMD_GD_UPL:
      st_status = BUSY;
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
    digitalWrite(PIN_WAIT_EN, HIGH); 
  digitalWrite(PIN_WAIT_EN, LOW);
}

uint8_t IRAM_ATTR writeData()
{
  if (st_status==DATA_READY && st_idx<st_size)
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

void getStrFromBuffer(char **s)
{
  char *p = *s;
  
  if (p != NULL)
    delete p;

  p = new char[st_size+1];
    
  for (st_idx=0; st_idx<st_size; st_idx++)
    p[st_idx] = st_buffer[st_idx];
  p[st_idx]='\0';

  Serial.println(p);
  *s=p;
}

void loop() {
  if (Serial.available()>0)
  {
    String s=Serial.readString();
    if (s=="dump")
    {
      for (uint16_t i=0; i<st_size; i++)
      {
        String s_hexa="";
        String s="";
        for(int j=0; j<32; j++, i++)
        {
          s_hexa=s_hexa+String(st_buffer[i],HEX)+" ";
          s=s+(st_buffer[i]>=32 && st_buffer[i]<128?String(st_buffer[i]):".");
        }
        Serial.println(s_hexa+s);    
      }
    }
    else
      st_status=(byte)atoi(s.c_str());
  }

  switch(st_cmd)
  {
    case CMD_WLIST:
    case CMD_WCLIST:
      {
        Serial.println("procesando comando "+String(st_cmd, HEX));
        int numSsid = WiFi.scanNetworks();
        
        if (numSsid == -1)
          st_size = sprintf(st_buffer, "WiFi net not found\r\n");
        else
        {
          st_size=0;
          for (int thisNet = 0; thisNet < numSsid; thisNet++) {
            if (st_cmd == CMD_WCLIST) 
              st_size += sprintf(&st_buffer[st_size], "_WNET(\"%s\")\r\n", WiFi.SSID(thisNet).c_str());
            else
              st_size += sprintf(&st_buffer[st_size], "%s\r\n", WiFi.SSID(thisNet).c_str());
          }
        }
        Serial.write((uint8_t*)st_buffer, st_size);
        st_cmd = 0;
        st_idx = 0;
        st_status = DATA_READY;
      }
      break;
    case CMD_WCONN:
      {
        if (st_ssid == NULL && st_password == NULL) 
        {
          st_size = sprintf(st_buffer, "first run _wnet(...) and _wpass(...)\r\n");
        }
        else if (st_ssid == NULL) 
        {
          st_size = sprintf(st_buffer, "first run _wnet(...)\r\n");
        }
        else if (st_password == NULL) 
        {
          st_size = sprintf(st_buffer, "first run _wpass(...)\r\n");
        }
        else
        {
          Serial.println("procesando comando "+String(st_cmd, HEX));
          WiFi.mode(WIFI_STA);
          WiFi.begin(st_ssid, st_password);
         
          //TODO: manejar máximo de reintentos para que no quede infinitamente
          uint8_t i = 0;
          while (WiFi.status() != WL_CONNECTED)
          {
            Serial.print('.');
            delay(500);
         
            if ((++i % 16) == 0)
            {
              Serial.println(F(" still trying to connect"));
            }
          }
          st_size = sprintf(st_buffer, "Connected to \"%s\"\r\nIP: %s\r\n", st_ssid, WiFi.localIP().toString().c_str());
          Serial.write((uint8_t*)st_buffer, st_size);
        }
        st_cmd = 0;
        st_idx = 0;
        st_status = DATA_READY;
      }
      break;
    case CMD_WDISCON:
      {
        WiFi.disconnect();
        st_size = sprintf(st_buffer, "disconnected\r\n");
        Serial.write((uint8_t*)st_buffer, st_size);
        st_cmd = 0;
        st_idx = 0;
        st_status = DATA_READY;
      }
      break;
    case CMD_WSTAT:
      {
        if (WiFi.status()==WL_CONNECTED)
          st_size = sprintf(st_buffer, "Connected to \"%s\"\r\nIP: %s\r\n", st_ssid, WiFi.localIP().toString().c_str());
        else
          st_size = sprintf(st_buffer, "Not connected\r\n");
        Serial.write((uint8_t*)st_buffer, st_size);
        st_cmd = 0;
        st_idx = 0;
        st_status = DATA_READY;
      }
      break;
    case CMD_WNET:
      /*
      for (st_idx=0; st_idx<st_size && st_idx<sizeof(st_ssid)-1; st_idx++)
        st_ssid[st_idx] = st_buffer[st_idx];
      st_ssid[st_idx]='\0';
      */
      getStrFromBuffer(&st_ssid);
      Serial.println("ssid="+String(st_ssid));
      st_cmd = 0;
      st_status=0;
      break;    
    case CMD_WPASS:
      /*
      for (st_idx=0; st_idx<st_size && st_idx<sizeof(st_password)-1; st_idx++)
        st_password[st_idx] = st_buffer[st_idx];
      st_password[st_idx]='\0';
      */
      getStrFromBuffer(&st_password);
      Serial.println("password="+String(st_password));
      st_cmd = 0;
      st_status=0;
      break;    
    case CMD_WLOAD:
      Serial.print("load=");
      for (int i=0; i<st_size; i++)
        Serial.print(String(st_buffer[i]));
      st_cmd = 0;
      st_status=0;
      break;    
    case CMD_WBLOAD:
      Serial.print("bload=");
      for (int i=0; i<st_size; i++)
        Serial.print(String(st_buffer[i]));
      st_cmd = 0;
      st_status=0;
      break;    
    case CMD_TCPSVR:
      st_buffer[st_size]='\0';
      strcpy(st_server, strtok(st_buffer, ":\0"));
      st_server_port=atoi(strtok(NULL, "\0"));
      Serial.println("server="+String(st_server)+":"+String(st_server_port));
      st_cmd = 0;
      st_status=0;
      break;    
    case CMD_TCPSEND:
      {
        WiFiClient client;
        if (client.connect(st_server, st_server_port)) {
          client.write(st_buffer, st_size);
          st_size = sprintf(st_buffer, "Sent Ok\r\n");
        }
        else
        {
          st_size = sprintf(st_buffer, "ERROR\r\n");
        }
        st_cmd = 0;
        st_idx = 0;
        st_status = DATA_READY;
      }
      break;
    case CMD_FNAME:
      for (st_idx=0; st_idx<st_size && st_idx<sizeof(st_file_name)-1; st_idx++)
        st_file_name[st_idx] = st_buffer[st_idx];
      st_file_name[st_idx]='\0';
      Serial.println("st_file_name="+String(st_file_name));
      st_cmd = 0;
      st_status=0;
      break;    
    case CMD_FSAVE:
      {
        Serial.println("FSAVE st_file_name="+String(st_file_name));
        String fname = "/"+String(st_file_name);
        File f = SPIFFS.open(fname.c_str(), "w");
        if (f) {
          f.write((uint8_t*)st_buffer, st_size);
          f.close();
        }
        st_cmd = 0;
        st_status=0;
      }
      break;    
    case CMD_FLOAD:
      {
        for (st_idx=0; st_idx<st_size && st_idx<sizeof(st_file_name)-1; st_idx++)
          st_file_name[st_idx] = st_buffer[st_idx];
        st_file_name[st_idx]='\0';
        Serial.println("FLOAD st_file_name="+String(st_file_name));
        String fname = "/"+String(st_file_name);
        File f = SPIFFS.open(fname.c_str(), "r");
        if (f) {
          st_size=f.size()+2;
          st_buffer[0]=(uint8_t)(st_size & 0xff);
          st_buffer[1]=(uint8_t)(st_size >> 8);
          Serial.println(String(f.size()));
          Serial.println(String(st_size));
          Serial.println(String((int)st_buffer[0]));
          Serial.println(String((int)st_buffer[1]));
          f.read((uint8_t*)(st_buffer+2), f.size());
          f.close();
        }
        st_idx=0;
        st_cmd = 0;
        st_status=DATA_READY;
      }
      break;    
    case CMD_LOADROM:
      {
        for (st_idx=0; st_idx<st_size && st_idx<sizeof(st_file_name)-1; st_idx++)
          st_file_name[st_idx] = st_buffer[st_idx];
        st_file_name[st_idx]='\0';
        //sprintf(st_file_name, "game.rom");
        Serial.println("FROM st_file_name="+String(st_file_name));
        String fname = "/"+String(st_file_name);
        File f = SPIFFS.open(fname.c_str(), "r");
        if (f) {
          st_size=f.size()+2;
          st_buffer[0]=(uint8_t)(st_size & 0xff);
          st_buffer[1]=(uint8_t)(st_size >> 8);
          Serial.println(String(f.size()));
          Serial.println(String(st_size));
          Serial.println(String((int)st_buffer[0]));
          Serial.println(String((int)st_buffer[1]));
          f.read((uint8_t*)(st_buffer+2), f.size());
          f.close();
        }
        st_idx=0;
        st_cmd = 0;
        st_status=DATA_READY;
      }
      break;    
    case CMD_FFILES:
      {
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        st_size=0;
       
        while(file){
          st_size += sprintf(&st_buffer[st_size], "%-13s\t%6i\r\n", file.name(), file.size());
          file = root.openNextFile();
        }
        file.close();

        Serial.write((uint8_t*)st_buffer, st_size);
        st_cmd = 0;
        st_idx = 0;
        st_status = DATA_READY;
      }
      break;
    case CMD_CLOUDROM:
      {
        //char st_url[128];
        //for (st_idx=0; st_idx<st_size && st_idx<sizeof(st_url)-1; st_idx++)
        //  st_url[st_idx] = st_buffer[st_idx];
        //st_url[st_idx]='\0';
        char *st_url=NULL;
        getStrFromBuffer(&st_url);

        String file_name=String(st_url).substring(String(st_url).lastIndexOf('/'));
        //Serial.println(st_url);
        HTTPClient http;
        File f = SPIFFS.open(file_name, "w");
        if (f) {
          http.begin(st_url);
          int httpCode = http.GET();
          if (httpCode > 0) 
          {
            if (httpCode == HTTP_CODE_OK) 
            {
              http.writeToStream(&f);
              st_size = sprintf(st_buffer, "Downloaded to \"%s\"\r\n", file_name.c_str());
            }
            else if (httpCode == 301) //redireccion
            {
              Serial.println("ups");
              st_size = sprintf(st_buffer, "HTTP GET redirect\r\n", httpCode);
            }
            else
            {
              Serial.printf("[HTTP] GET... failed, error: %s\r\n", http.errorToString(httpCode).c_str());
              st_size = sprintf(st_buffer, "HTTP GET error: %i\r\n", httpCode);
            }          
          } else 
          {
            Serial.printf("[HTTP] GET... failed, error: %s\r\n", http.errorToString(httpCode).c_str());
            st_size = sprintf(st_buffer, "HTTP GET error: %i\r\n", httpCode);
          }
          f.close();
        }
        else
        {
          st_size = sprintf(st_buffer, "Error openning file\r\n");
        }
        http.end();

        delete st_url;
        st_status = DATA_READY;
        st_cmd = 0;
        st_idx = 0;
      }
      break;
    case CMD_GD_UPL:
      {
        char *fileName = NULL;
        getStrFromBuffer(&fileName);

        String file_name= "/" + String(fileName);
        File f = SPIFFS.open(file_name.c_str(), "r");
        if (f) {
          st_size=f.size();
          f.read((uint8_t*)(st_buffer), f.size());
          f.close();
          if (upload(st_buffer, st_size, 10000))
            st_size = sprintf(st_buffer, "Upload Ok\r\n");
          else
            st_size = sprintf(st_buffer, "Upload Error\r\n");
        }
        else
        {
          st_size = sprintf(st_buffer, "Error openning file\r\n");
        }

        delete fileName;
        st_status = DATA_READY;
        st_cmd = 0;
        st_idx = 0;
      }
      break;
    case CMD_SENDSTR:
    case CMD_NULL:
      break;
    default:
      Serial.println("comando desconocido "+String(st_cmd));
  }
}
