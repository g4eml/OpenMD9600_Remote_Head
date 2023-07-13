#include <SPI.h>
#include <EEPROM.h>


#define COM1TX   0      //Pin 1
#define COM1RX   1      //Pin 2
#define COM2TX   8      //Pin 11   (don't need this pin so it is redefined as GPIO for use as VOLUD)
#define COM2RX   9      //Pin 12
#define LAMP 15         //Pin 20
#define MIC_POWER  17   //Pin 22
#define PTT 14          //Pin 19
#define K0 10            //Pin 14
#define K1 7            //Pin 10
#define K2 6            //Pin 9
#define K3 5            //Pin 7
#define K4 4            //Pin 6
#define K5 3            //Pin 5
#define K6 2            //Pin 4
#define ROTARYA 13       //Pin 17
#define ROTARYB 12      //Pin 16
#define MICP2 A0        //Pin 31
#define MICP1 A1        //Pin 32
#define LCDRESET 21     //Pin 27
#define LCDRX  16       //Pin 21      Not used but must be defined.
#define LCDCD  20       //Pin 26
#define LCDCS  22       //Pin 29
#define LCDCK  18       //Pin 24
#define LCDDA  19       //Pin 25   
#define VOLCS  11       //Pin 15
#define VOLUD  8        //Pin 11
#define VOLPOT A2       //Pin 34

unsigned int frontKeys;
int8_t rotaryDirection;
uint8_t micRow;
uint8_t micCol;
uint8_t brightness;
uint8_t micPower;
static uint8_t poweredDown = 0;
int volumePot = 0;
static bool GPSon = 0;
static bool DataReceived;

bool reSend = false;

void setup() 
{
Serial.begin(250000);             //Debug USB port
Serial1.setRX(COM1RX);
Serial1.setTX(COM1TX);
Serial2.setRX(COM2RX);
Serial2.setTX(COM2TX);          //will be reset as a GPIO later. 
Serial1.begin(250000);          //Comms to/from radio
Serial2.begin(9600);            //Comms from GPS if fitted
EEPROM.begin(256);

if(EEPROM.read(0) == 73)              //valid EEPROM data?
{
  poweredDown = EEPROM.read(1);         //restore power status from arbitary location. 
} 

pinMode(LAMP,OUTPUT);
pinMode(MIC_POWER,OUTPUT);
pinMode(K0,OUTPUT);
pinMode(K1,OUTPUT);
pinMode(K2,OUTPUT);
pinMode(K3,OUTPUT);
pinMode(LCDRESET,OUTPUT);
pinMode(LCDCD,OUTPUT);
pinMode(LCDCS,OUTPUT);

pinMode(VOLCS,OUTPUT);
gpio_set_function(VOLUD, GPIO_FUNC_NULL);           //redefine the unused Serial Port 2 Tx pin as a GPIO. 
pinMode(VOLUD,OUTPUT);

pinMode(PTT,INPUT_PULLUP);
pinMode(K4,INPUT_PULLUP);
pinMode(K5,INPUT_PULLUP);
pinMode(K6,INPUT_PULLUP);
pinMode(ROTARYA,INPUT_PULLUP);
pinMode(ROTARYB,INPUT_PULLUP);

analogWriteFreq(20000);           //This value works well with version 1 radios but not with version 3 or later without modification. 
analogWriteRange(255);

analogWrite(LAMP,0);
digitalWrite(MIC_POWER,LOW);
digitalWrite(K0,HIGH);
digitalWrite(K1,HIGH);
digitalWrite(K2,HIGH);
digitalWrite(K3,HIGH);

digitalWrite(VOLCS, HIGH);
digitalWrite(VOLUD,HIGH);

analogReadResolution(12);
attachInterrupt(digitalPinToInterrupt(ROTARYA), rotaryISR, CHANGE);


SPI.setRX(LCDRX);
SPI.setTX(LCDDA);
SPI.setSCK(LCDCK);
SPI.beginTransaction(SPISettings(8000000,MSBFIRST,SPI_MODE3));
SPI.begin(false);           //Start SPI and manually handle the CS pin

digitalWrite(LCDCS,HIGH);
digitalWrite(LCDCD,HIGH);
digitalWrite(LCDRESET,LOW);
delay(1);
digitalWrite(LCDRESET,HIGH);
delay(5);

brightness=255;
micPower=1;
LCDinit();
LCDClear();
LCDlogo();
LCDtest();

if(poweredDown)
  {
    powerDown();
  } 

while((frontKeys != 0) || (micRow != 5) || (micCol != 4))
{
  readKeys();
  readMicButtons();
}

volPotStep(-64);          //ensure digital pot starts at zero volume. 
 
}

void setup1()
{
  
}

//First Core task is to send to the radio
void loop() 
{
static int lastkeys=0;
static int lastVolume=0;
static uint8_t lastMRow=5;
static uint8_t lastMCol=4;
static uint8_t lastBrightness = 0;
static uint8_t lastMicPower = 0;
static char buff[10];

delay(1);
 if (micPower != lastMicPower)
    {
      digitalWrite(MIC_POWER,micPower);
      lastMicPower=micPower;
    }

  if(brightness != lastBrightness)
    {
    analogWrite(LAMP,brightness);
    lastBrightness=brightness;
    }
    
readKeys();

if(! poweredDown)
  {
    if((frontKeys != lastkeys) || (reSend))
    {
      reSend = false;
      lastkeys=frontKeys;
      Serial1.write('K');
      Serial1.write(frontKeys >> 8);
      Serial1.write(frontKeys & 0xFF);
      Serial.print("Keys ");
      Serial.println(frontKeys,HEX);
    }

  readMicButtons();
  if((micRow != lastMRow) || (micCol != lastMCol))
    {
      lastMRow=micRow;
      lastMCol=micCol;
      Serial1.write('M');
      Serial1.write(micRow);
      Serial1.write(micCol);
      Serial.print("Mic ");
      Serial.println((micRow<<8)+micCol,HEX);
     }
  }
  else
  {
  if(frontKeys == 1)    // Power Button
    {
      poweredDown = false;
      EEPROM.write(0,73);
      EEPROM.write(1,0);
      EEPROM.commit();
      sendBreak();
    }
  }
 

 readVol();
 if((volumePot - lastVolume) != 0)
   {
      volPotStep(volumePot - lastVolume);
      Serial.print(" Vol = ");
      Serial.println(volumePot);
      lastVolume=volumePot;
   }

  if(Serial2.available() > 0)         //received something from the GPS module
    {
      for(int n=Serial2.available();n > 0; n--)
      {
        if(GPSon)
        {
          Serial1.write("G");
          Serial1.write(Serial2.read());
          if(n > 1)
          {
          Serial1.write(Serial2.read());
          n--;  
          }
          else
          {
          Serial1.write((byte)0x00);
          }   
        }
        else
        {
          char dummy=Serial2.read();          //throw away the GPS data
        }
      }
    }
}


//Second core task is to receive from the radio
void loop1()
{
  int ch;
  int bytecount;
  if(Serial1.available() > 1)        //wait till there are at least two characters available
  {
    ch=Serial1.read();
    switch(ch)
    {
      case 'C':
      digitalWrite(LCDCS,LOW);
      digitalWrite(LCDCD,LOW);  // command mode
      ch=Serial1.read();
      LCDtransfer(ch);
      digitalWrite(LCDCS,HIGH);     
      break;

      case 'D':           
      bytecount=Serial1.read();
      digitalWrite(LCDCS,LOW);
      digitalWrite(LCDCD,HIGH);  // data mode
      while(bytecount>0)
      {
       if(Serial1.available()>0)
        {
          ch=Serial1.read();
          LCDtransfer(ch);
          bytecount--; 
        }
      }      
      digitalWrite(LCDCS,HIGH);     
      break;

      case 'B':        //Brightness value
      ch=Serial1.read();
      brightness=ch; 
      break;

      case 'P':         //Mic Power
      ch=Serial1.read();
      micPower = ch & 0x01; 
      break;

      case 'R':         //rescan and resend the keys
      ch=Serial1.read();    //Discard the second byte
      reSend=true; 
      break;

      case 'O':         //Power down
      ch=Serial1.read();    //Discard the second byte
      powerDown();      
      break;
      
      case 'G':         //GPS data on/off
      ch=Serial1.read();
      GPSon = ch & 0x01;
      Serial.print("GPS Data ");
      if(GPSon)
      {
        Serial.println("On");
      }
      else
      {
        Serial.println("Off");
      } 
      break;
    } 
  }
}

void sendBreak(void)
{
  Serial1.end();
  digitalWrite(0,HIGH);
  pinMode(0,OUTPUT);
  digitalWrite(0,LOW);
  delay(100);
  digitalWrite(0,HIGH);
  Serial1.begin(250000);
}

void powerDown(void)
{
  brightness = 0;
  micPower = 0;
  digitalWrite(LCDCS,LOW);
  digitalWrite(LCDCD,LOW);  // command mode

  LCDtransfer(0xAE); // Display Off

  LCDtransfer(0xA5);// All Pixels On

  EEPROM.write(0,73);
  EEPROM.write(1,1);
  EEPROM.commit();
  poweredDown = true;  
}

void rotaryISR(void)
{
  static int lasta=0;
  bool rota=digitalRead(ROTARYA);
  bool rotb=digitalRead(ROTARYB);

if(lasta != rota)
{
   lasta=rota;
    if(rota==rotb)
   {
    rotaryDirection=-1;
   }
   else
   {
    rotaryDirection=1;
   }
}
   
}


void readKeys(void)
{
  frontKeys =0;
  digitalWrite(K0,LOW);
  if(digitalRead(K6) == LOW) frontKeys=frontKeys | 0x0001;
  if(digitalRead(K5) == LOW) frontKeys=frontKeys | 0x0002;
  if(digitalRead(K4) == LOW) frontKeys=frontKeys | 0x0004;
  digitalWrite(K0,HIGH);

  digitalWrite(K1,LOW);
  if(digitalRead(K6) == LOW) frontKeys=frontKeys | 0x0008;
  if(digitalRead(K5) == LOW) frontKeys=frontKeys | 0x0010;
  if(digitalRead(K4) == LOW) frontKeys=frontKeys | 0x0020;
  digitalWrite(K1,HIGH); 

   
  digitalWrite(K2,LOW);
  if(digitalRead(K6) == LOW) frontKeys=frontKeys | 0x0040;
  if(digitalRead(K5) == LOW) frontKeys=frontKeys | 0x0080;
  if(digitalRead(K4) == LOW) frontKeys=frontKeys | 0x0100;
  digitalWrite(K2,HIGH); 

     
  digitalWrite(K3,LOW);
  if(digitalRead(K6) == LOW) frontKeys=frontKeys | 0x0200;
  digitalWrite(K3,HIGH); 

  if(digitalRead(PTT)== LOW) frontKeys=frontKeys | 0x8000;

  if(rotaryDirection == 1) frontKeys=frontKeys | 0x4000;
  if(rotaryDirection == -1) frontKeys=frontKeys | 0x2000;
  rotaryDirection = 0;
  
}

void readMicButtons(void)
{
  int rl = 9;
  int cl = 9;

  while((micRow != rl) || (micCol != cl)) 
  {
    rl=micRow;
    cl=micCol;
    micRow = (int)(analogRead(MICP1) / 800);
    micCol = (int)(analogRead(MICP2) / 1000); 
    delay(10);
  }


//prevent half decodes while the row and column are changing. 
  if(micRow > 4) micCol = 4;
  if(micCol >3) micRow=5;
}

void readVol(void)
{
  int ave;
  int total = 0;
  for(ave=0;ave<10;ave++)
  {
  total += analogRead(VOLPOT); 
  }
  volumePot= (total/10) >>6 ;       //convert 12 bit sample to 6 bits. 
}

void volPotStep(int steps)
{
  int s;
  if(steps > 0)       //step up
  {    
    digitalWrite(VOLUD,HIGH);
    delayMicroseconds(1);
    digitalWrite(VOLCS, LOW);
    delayMicroseconds(1);
    digitalWrite(VOLUD, LOW);
    delayMicroseconds(4);
    for(s=steps; s > 0 ; s--)
    {
      digitalWrite(VOLUD, LOW);
      delayMicroseconds(1);
      digitalWrite(VOLUD, HIGH);
      delayMicroseconds(1);   
    }
    digitalWrite(VOLCS, HIGH);
    delay(10);
  }
  else                //step down
  {
    digitalWrite(VOLUD,LOW);
    delayMicroseconds(1);
    digitalWrite(VOLCS, LOW);
    delayMicroseconds(4);
    for(s=steps; s < 0 ; s++)
    {
      digitalWrite(VOLUD, LOW);
      delayMicroseconds(1);
      digitalWrite(VOLUD, HIGH);
      delayMicroseconds(1);;    
    }
    delayMicroseconds(1);
    digitalWrite(VOLCS, HIGH); 
    delay(10);  
  }
}

void LCDinit(void)
{ 
  digitalWrite(LCDCS,LOW);
  digitalWrite(LCDCD,LOW);  // command mode

  LCDtransfer(0xE2); // System Reset

  LCDtransfer(0x2F);// Voltage Follower On
  LCDtransfer(0x81);// Set Electronic Volume = 15
  LCDtransfer(0x12); // Contrast
  LCDtransfer(0xA2); // Set Bias = 1/9
  LCDtransfer(0xA1); // SEG Direction
  LCDtransfer(0xC0); // COM Direction
  LCDtransfer(0xA6); // White background, black pixels
  LCDtransfer(0xAF); // enable

  digitalWrite(LCDCS,HIGH);
}


void LCDtransfer(uint8_t data1)
{
  SPI.transfer(&data1, 1);
}

void LCDClear(void)
{
  digitalWrite(LCDCS,LOW);

  for(int row=0;row<(64/8);row++)
  {
    digitalWrite(LCDCD,LOW);  // command mode
    LCDtransfer(0xB0 | row);        //Start Row
    LCDtransfer(0x10);        //Set X High Byte
    LCDtransfer(0x04);        //4 pixels from the left
    digitalWrite(LCDCD,HIGH);  // data mode
    for(int i=0;i<128 ;i++)
      {
        LCDtransfer(0x00);        //8 vertical bits of data
      }
  }
  digitalWrite(LCDCS,HIGH);
}

void LCDlogo(void)
{
  uint8_t logo[] = {
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x7f, 0x02, 0x0c, 0x02, 0x7f, 0x00,0x7f, 0x41, 0x41, 0x22, 0x1c, 0x00,0x06, 0x49, 0x49, 0x29, 0x1e, 0x00,0x3c, 0x4a, 0x49, 0x49, 0x30, 0x00,0x3e, 0x51, 0x49, 0x45, 0x3e, 0x00,0x3e, 0x51, 0x49, 0x45, 0x3e, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x7f, 0x09, 0x19, 0x29, 0x46, 0x00,0x38, 0x54, 0x54, 0x54, 0x18, 0x00,0x7c, 0x04, 0x18, 0x04, 0x78, 0x00,0x38, 0x44, 0x44, 0x44, 0x38, 0x00,0x04, 0x3f, 0x44, 0x40, 0x20, 0x00,
                    0x38, 0x54, 0x54, 0x54, 0x18, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x7f, 0x08, 0x08, 0x08, 0x7f, 0x00,0x38, 0x54, 0x54, 0x54, 0x18, 0x00,0x20, 0x54, 0x54, 0x54, 0x78, 0x00,0x38, 0x44, 0x44, 0x48, 0x7f, 0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
                    };
  digitalWrite(LCDCS,LOW);

    digitalWrite(LCDCD,LOW);  // command mode
    LCDtransfer(0xB4);        //Start Row
    LCDtransfer(0x10);        //Set X High Byte
    LCDtransfer(0x04);        //4 pixels from the left
    digitalWrite(LCDCD,HIGH);  // data mode
    for(int i=0;i<128 ;i++)
      {
        LCDtransfer(logo[i]);        //8 vertical bits of data
      }
    digitalWrite(LCDCS,HIGH);
}

void LCDtest(void)
{
    digitalWrite(LCDCS,LOW);
    digitalWrite(LCDCD,LOW);  // command mode
    LCDtransfer(0xB7);        //Start Row
    LCDtransfer(0x10);        //Set X High Byte
    LCDtransfer(0x04);        //4 pixels from the left
    digitalWrite(LCDCD,HIGH);  // data mode
    digitalWrite(LCDCS,HIGH);
}
