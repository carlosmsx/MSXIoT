const uint8_t D[] = {53,52,51,50,49,48,47,46};
const uint8_t A[] = {43,41,39,37,35,33,31,29,32,34,40,36,27,30,28,25,23,26};
#define OE 38
#define CE 42
#define WE 24

/*
 * 42 -> CE
 * 40 -> A10
 * 38 -> OE
 * 36 -> A11
 * 34 -> A9
 * 32 -> A8
 * 30 -> A13
 * 28 -> A14
 * 26 -> A17
 * 24 -> WE
 */

/*
 * 43 -> A0
 * 41 -> A1
 * 39 -> A2
 * 37 -> A3
 * 35 -> A4
 * 33 -> A5
 * 31 -> A6
 * 29 -> A7
 * 27 -> A12
 * 25 -> A15
 * 23 -> A16
 */

uint8_t readByte(uint32_t address)
{
  uint32_t bit=0x1;
  
  digitalWrite(OE, HIGH);
  digitalWrite(CE, HIGH);
  digitalWrite(WE, HIGH);
  
  for (int i=0; i<sizeof(A); i++)
  {
    digitalWrite(A[i], (address & bit)?HIGH:LOW);
    bit = bit<<1;
  }

  for (int i=0; i<sizeof(D); i++)
    pinMode(D[i], INPUT);

  digitalWrite(CE, LOW);
  digitalWrite(OE, LOW);
  
  uint8_t d=0;
  bit=0x1;
  for (int i=0; i<sizeof(D); i++)
  {
    if (digitalRead(D[i])) 
      d |= bit;
    bit=bit<<1; 
  }
  
  digitalWrite(OE, HIGH);
  digitalWrite(CE, HIGH);

  return d;
}

void writeByte(uint32_t address, uint8_t b)
{
  uint32_t bit=0x1;
  
  digitalWrite(OE, HIGH);
  digitalWrite(CE, HIGH);
  digitalWrite(WE, HIGH);
  
  for (int i=0; i<sizeof(A); i++)
  {
    digitalWrite(A[i], (address & bit)?HIGH:LOW);
    bit = bit<<1;
  }

  bit=0x1;
  for (int i=0; i<sizeof(D); i++)
  {
    pinMode(D[i], OUTPUT);
    digitalWrite(D[i], (b & bit)?HIGH:LOW);
    bit=bit<<1; 
  }
  
  digitalWrite(CE, LOW);
  digitalWrite(WE, LOW);
  delayMicroseconds(1);
  digitalWrite(WE, HIGH);
  digitalWrite(CE, HIGH);
  
  for (int i=0; i<sizeof(D); i++)
    pinMode(D[i], INPUT);
}

void erase()
{
  uint8_t b = readByte(0);
/*
0 Write 5555H AAH
1 Write 2AAAH 55H
2 Write 5555H 80H
3 Write 5555H AAH
4 Write 2AAAH 55H
5 Write 5555H 10H
*/
  writeByte(0x5555, 0xAA);  
  writeByte(0x2AAA, 0x55);  
  writeByte(0x5555, 0x80);  
  writeByte(0x5555, 0xAA);  
  writeByte(0x2AAA, 0x55);  
  writeByte(0x5555, 0x10);  
  delay(50);
  writeByte(0, b);
}

void unlock()
{
  uint8_t b = readByte(0);
/*  
0 Write 5555H AAH
1 Write 2AAAH 55H
2 Write 5555H 80H
3 Write 5555H AAH
4 Write 2AAAH 55H
5 Write 5555H 20H
 */
  writeByte(0x5555, 0xAA);  
  writeByte(0x2AAA, 0x55);  
  writeByte(0x5555, 0x80);  
  writeByte(0x5555, 0xAA);  
  writeByte(0x2AAA, 0x55);  
  writeByte(0x5555, 0x20);  
  //delay(10);
  delay(10);
  writeByte(0, b);
}


String hexByte(uint8_t b)
{
  String res = "";
  if (b<16) res = "0";
  res = res + String(b, HEX);
  return res;
}

void dump32(uint32_t address)
{
  String s = hexByte(address>>16) + hexByte(address >> 8) + hexByte(address & 0xFF) + " > ";
  String ascii = ": ";
  for (uint32_t i=0; i<32; i++)
  {
    uint8_t b = readByte(address+i);
    s = s + hexByte(b) + " ";
    if (b>=32 && b<128) 
      ascii = ascii + (char)b;
    else
      ascii = ascii + ".";
  }
  Serial.println(s + ascii);
}

void dump128(uint32_t address)
{
  dump32(address);
  dump32(address+32);
  dump32(address+64);
  dump32(address+96);
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  
  pinMode(OE, OUTPUT); digitalWrite(OE, HIGH);
  pinMode(CE, OUTPUT); digitalWrite(CE, HIGH);
  pinMode(WE, OUTPUT); digitalWrite(WE, HIGH);
  
  for (int i=0; i<sizeof(D); i++)
    pinMode(D[i], INPUT);
    
  for (int i=0; i<sizeof(A); i++)
  {
    pinMode(A[i], OUTPUT);
    digitalWrite(A[i], LOW);
  }

  Serial.println("---------------");
  unlock();
  dump128(0);
  erase();
  Serial.println("---------------");
}

uint32_t address = 0;
void loop() {
  dump128(address);
  address += 128;
  delay(1000);
}
