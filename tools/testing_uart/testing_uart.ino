//MEGA2560

const uint8_t D[] = {37,39,41,51,49,47,45,43};
#define CS 53
#define A0 35
#define RD 33
#define EN 31

String hexByte(uint8_t b)
{
  String res = "";
  if (b<16) res = "0";
  res = res + String(b, HEX);
  return res;
}

void setup() {
  pinMode(CS, INPUT);
  pinMode(A0, INPUT);
  pinMode(RD, INPUT);
  pinMode(EN, OUTPUT);
  
  for (int i=0; i<sizeof(D); i++)
    pinMode(D[i], INPUT);
    
  Serial.begin(115200);  
  Serial.print("Esperando CS alto...");
  while (digitalRead(CS)==0);
  Serial.println("Ok");
  digitalWrite(EN, HIGH); //habilito el decoder
}

void loop() {
  
  //espero un acceso al puerto 0
  while (digitalRead(CS)==1);

  if (digitalRead(RD)==HIGH)
  {
    //leo un byte desde la MSX
    uint8_t b=0;
    uint8_t bit=1;
    for (int i=0; i<sizeof(D); i++)
    {
      if (digitalRead(D[i])==HIGH)
        b = b | bit;
      bit=bit<<1;
    }
    Serial.print(hexByte(b)+" ");
    
    digitalWrite(EN, LOW); //suelto el WAIT
    //while (digitalRead(CS)==HIGH);
    delayMicroseconds(10);
    digitalWrite(EN, HIGH); //habilito para esperar el siguiente acceso
  }
  else
  {
    //envío un byte a la MSX
    uint8_t b=123;
    uint8_t bit=1;
    for (int i=0; i<sizeof(D); i++)
    {
      pinMode(D[i], OUTPUT);
      digitalWrite(D[i], (b & bit)?HIGH: LOW);
      bit=bit<<1;
    }
    
    digitalWrite(EN, LOW); //suelto el WAIT
    while (digitalRead(CS)==HIGH);
    //pongo data bus como entrada
    for (int i=0; i<sizeof(D); i++)
      pinMode(D[i], INPUT);
    digitalWrite(EN, HIGH); //habilito para esperar el siguiente acceso
  }
  //delay(1000);
}
