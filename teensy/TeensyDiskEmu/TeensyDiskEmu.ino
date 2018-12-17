
#include <stdarg.h>

#define L1_RED()    digitalWriteFast(21,HIGH);digitalWriteFast(22,LOW); digitalWriteFast(23,LOW);
#define L1_GREEN()  digitalWriteFast(21,LOW); digitalWriteFast(22,LOW); digitalWriteFast(23,HIGH);
#define L1_BLUE()   digitalWriteFast(21,LOW); digitalWriteFast(22,HIGH);digitalWriteFast(23,LOW);
#define L1_YELLOW() digitalWriteFast(21,HIGH);digitalWriteFast(22,LOW); digitalWriteFast(23,HIGH);
#define L1_CYAN()   digitalWriteFast(21,LOW); digitalWriteFast(22,HIGH);digitalWriteFast(23,HIGH);
#define L1_VIOLET() digitalWriteFast(21,HIGH);digitalWriteFast(22,HIGH);digitalWriteFast(23,LOW);
#define L1_WHITE()  digitalWriteFast(21,HIGH);digitalWriteFast(22,HIGH);digitalWriteFast(23,HIGH);
#define L1_BLACK()  digitalWriteFast(21,LOW); digitalWriteFast(22,LOW); digitalWriteFast(23,LOW);

#define L2_RED()    digitalWriteFast(18,HIGH);digitalWriteFast(19,LOW); digitalWriteFast(20,LOW);
#define L2_BLUE()   digitalWriteFast(18,LOW); digitalWriteFast(19,HIGH);digitalWriteFast(20,LOW);
#define L2_GREEN()  digitalWriteFast(18,LOW); digitalWriteFast(19,LOW); digitalWriteFast(20,HIGH);
#define L2_CYAN()   digitalWriteFast(18,LOW); digitalWriteFast(19,HIGH);digitalWriteFast(20,HIGH);
#define L2_VIOLET() digitalWriteFast(18,HIGH);digitalWriteFast(19,HIGH);digitalWriteFast(20,LOW);
#define L2_YELLOW() digitalWriteFast(18,HIGH);digitalWriteFast(19,LOW); digitalWriteFast(20,HIGH);
#define L2_WHITE()  digitalWriteFast(18,HIGH);digitalWriteFast(19,HIGH);digitalWriteFast(20,HIGH);
#define L2_BLACK()  digitalWriteFast(18,LOW); digitalWriteFast(19,LOW); digitalWriteFast(20,LOW);

#define A1 37
#define A0 36
#define INTERUPT_TO_TRS80 35
#define _37ECWR 34
#define _37E8WR 33
#define FF_PRE 29
#define _37E4WR 28
#define _37E0WR 27
#define _37ECRD 26
#define FF_CLR 25
#define _37E8RD 12
#define _37E4RD 11
#define _37E0RD 10
#define D7 9
#define D6 8
#define D5 7
#define D4 6
#define D3 5
#define D2 4
#define D1 3
#define D0 2
#define NOTHING 0

volatile int activeInterrupt = NOTHING;
volatile int a0;
volatile int a1;




/* printf() to serial output */
void p(char *fmt, ... ){
    char buf[80];
        
    va_list args;
    va_start (args, fmt );
    vsnprintf(buf, 128, fmt, args);
    va_end (args);
    Serial.print(buf);
    Serial.flush();
}


/* direct data bus pins outward */
void dataOutMode() {
  pinMode(D7,OUTPUT);
  pinMode(D6,OUTPUT);
  pinMode(D5,OUTPUT);
  pinMode(D4,OUTPUT);
  pinMode(D3,OUTPUT);
  pinMode(D2,OUTPUT);
  pinMode(D1,OUTPUT);
  pinMode(D0,OUTPUT);
}


/* direct data bus pins inward */
void dataInMode() {
  pinMode(D7,INPUT);
  pinMode(D6,INPUT);
  pinMode(D5,INPUT);
  pinMode(D4,INPUT);
  pinMode(D3,INPUT);
  pinMode(D2,INPUT);
  pinMode(D1,INPUT);
  pinMode(D0,INPUT);
}


/* set up pin directionality */
void pinSetup() {
  pinMode(18,OUTPUT);                 // L2 LEDs (leg pins may be rearranged depending on what RGB LED used -- just change the macro names around)
  pinMode(19,OUTPUT);                 // L2 LEDs
  pinMode(20,OUTPUT);                 // L2 LEDs
  pinMode(21,OUTPUT);                 // L1 LEDs
  pinMode(22,OUTPUT);                 // L1 LEDs
  pinMode(23,OUTPUT);                 // L1 LEDs
  pinMode(39,OUTPUT);                 // nothing, unused pin
  pinMode(38,OUTPUT);                 // nothing, unused pin
  pinMode(A1, INPUT_PULLUP);                 // A1 from TRS-80
  pinMode(A0, INPUT_PULLUP);                 // A0 from TRS-80
  pinMode(INTERUPT_TO_TRS80, OUTPUT); // pin to trigger interrupts on the TRS-80
  pinMode(_37ECWR, INPUT);            // pin to detect when write to address 37EC has been triggered (low)
  pinMode(_37E8WR, INPUT);            // pin to detect when write to address 37E8 has been triggered (low)
  pinMode(30,OUTPUT);                 // nothing, unused pin
  pinMode(FF_PRE,OUTPUT);             // flip flop PRE 
  pinMode(_37E4WR,INPUT);             // pin to detect when write to address 37E4 has been triggered (low)
  pinMode(_37E0WR,INPUT);             // pin to detect when write to address 37E0 has been triggered (low)
  pinMode(_37ECRD,INPUT);             // pin to detect when read from address 37E0 has been triggered (low)
  pinMode(FF_CLR,OUTPUT);             // flip flop CLR 
  pinMode(_37E8RD,INPUT);             // pin to detect when read from address 37E8 has been triggered (low)
  pinMode(_37E4RD,INPUT);             // pin to detect when read from address ﻿37E4 has been triggered (low)
  pinMode(_37E0RD,INPUT);             // pin to detect when read from address 37E0 has been triggered (low)
  dataInMode();
}


/* jigger the flip flop that's tied to the WAIT* line on the TRS-80 to make sure it's in a known state */
void initFlipFlop() {
  digitalWrite(FF_PRE,LOW);
  digitalWrite(FF_CLR,LOW);

  digitalWrite(FF_PRE,HIGH);
  digitalWrite(FF_CLR,LOW);

  digitalWrite(FF_PRE,LOW);
  digitalWrite(FF_CLR,HIGH);

  digitalWrite(FF_PRE,HIGH);
  digitalWrite(FF_CLR,LOW);

  digitalWrite(FF_PRE,HIGH);
  digitalWrite(FF_CLR,HIGH);
}



/* initialize output pins */
void initialPinState() {
  digitalWriteFast(INTERUPT_TO_TRS80, HIGH); // turn off TRS-80 interrupt line (it's active low)
  initFlipFlop();                            // jigger flip flop into a known state (loaded, ready for trigger)
}


/* reset the latch that's tied to TRS-80 WAIT* line */
inline void resetWaitLatch() {
  digitalWrite(FF_PRE,HIGH);
  digitalWrite(FF_CLR,LOW);
  //delayMicroseconds(1);
  digitalWrite(FF_CLR,HIGH);
}


#define GET_LOW_ADDRESSES()   a0 = digitalReadFast(a0); a1 = digitalReadFast(a1);

void _37E0WRInterrupt() {
  //p("_37E0WRInterrupt\n");
  GET_LOW_ADDRESSES()
  activeInterrupt = _37E0WR;          
}

void _37E8WRInterrupt() {
  //p("_37E8WRInterrupt\n");
  GET_LOW_ADDRESSES()
  activeInterrupt = _37E8WR;  
}

void _37E4WRInterrupt() {
  //p("_37E4WRInterrupt\n");
  GET_LOW_ADDRESSES()
  activeInterrupt = _37E4WR;  
}

void _37ECWRInterrupt() {
  //p("_37ECWRInterrupt\n");
  GET_LOW_ADDRESSES()
  activeInterrupt = _37ECWR;
}

void _37ECRDInterrupt() {
  //p("_37ECRDInterrupt\n");
  GET_LOW_ADDRESSES()
  activeInterrupt = _37ECRD;
}

void _37E8RDInterrupt() {
  //p("_37E8RDInterrupt\n");
  GET_LOW_ADDRESSES()
  activeInterrupt = _37E8RD;
}

void _37E4RDInterrupt() {
  //p("_37E4RDInterrupt\n");
  GET_LOW_ADDRESSES()
  activeInterrupt = _37E4RD;
}

void _37E0RDInterrupt() {
  //p("_37E0RDInterrupt\n");
  GET_LOW_ADDRESSES()
  activeInterrupt = _37E0RD;
}


/* retrieve content of data bus */
byte getDataBus() {
  byte x = 0;

  if(digitalRead(D7) == HIGH) x+=128;
  if(digitalRead(D6) == HIGH) x+=64;
  if(digitalRead(D5) == HIGH) x+=32;
  if(digitalRead(D4) == HIGH) x+=16;
  if(digitalRead(D3) == HIGH) x+=8;
  if(digitalRead(D2) == HIGH) x+=4;
  if(digitalRead(D1) == HIGH) x+=2;
  if(digitalRead(D0) == HIGH) x+=1;

  return x;
}


/* set the value of the data bus */
void setDataBus(byte b) {
  byte x = b;

  if(x >= 128) {digitalWriteFast(D7,HIGH); x-=128;} else {digitalWriteFast(D7,LOW);}
  if(x >= 64)  {digitalWriteFast(D6,HIGH); x-=64; } else {digitalWriteFast(D6,LOW);}
  if(x >= 32)  {digitalWriteFast(D5,HIGH); x-=32; } else {digitalWriteFast(D5,LOW);}
  if(x >= 16)  {digitalWriteFast(D4,HIGH); x-=16; } else {digitalWriteFast(D4,LOW);}
  if(x >= 8)   {digitalWriteFast(D3,HIGH); x-=8;  } else {digitalWriteFast(D3,LOW);}
  if(x >= 4)   {digitalWriteFast(D2,HIGH); x-=4;  } else {digitalWriteFast(D2,LOW);}
  if(x >= 2)   {digitalWriteFast(D1,HIGH); x-=2;  } else {digitalWriteFast(D1,LOW);}
  if(x >= 1)   {digitalWriteFast(D0,HIGH);        } else {digitalWriteFast(D0,LOW);}
}


/* wire up the interrupts */
void configureInterrupts() {
  attachInterrupt(digitalPinToInterrupt(_37ECWR), _37ECWRInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(_37E8WR), _37E8WRInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(_37E4WR), _37E4WRInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(_37E0WR), _37E0WRInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(_37ECRD), _37ECRDInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(_37E8RD), _37E8RDInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(_37E4RD), _37E4RDInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(_37E0RD), _37E0RDInterrupt, FALLING);
}


/* initialize everything */
void setup() {
  int i = 0;
  cli();
  Serial.begin(2000000); // high speed serial over USB
  pinSetup();            // set pin modes
  initialPinState();     // put pins in initial configuration

  L1_RED();
  L2_RED();
  while(!Serial && i < 10) {
    i++;
    delay(100);
    L2_YELLOW();
    delay(100);
    L2_RED();
  }
  L2_GREEN();
  configureInterrupts(); // tie interrupt lines to code blocks
  L1_GREEN();  

  sei();
  p("Ready.\n");
}



/* Main Loop                                                               */
/* ---------                                                               */
/* When activeInterrupt changes, split it into a "read" or "write" request */
/* For read requests, set data bus direction to outward (from the Teensy   */
/* to the TRS-80).  For write requests, set the data bus direction to      */
/* inward (from the TRS-80 to the Teensy).                                 */
/*                                                                         */
/* Then switch into each address's intended function                       */
void loop() {
    static int i;
    i++;
    if(activeInterrupt == NOTHING) {
    }
    else {
        cli();
        
        if(activeInterrupt == _37ECWR || activeInterrupt == _37E8WR || activeInterrupt == _37E4WR || activeInterrupt == _37E0WR) {
          dataInMode();
          L1_CYAN();
          byte b = getDataBus();
          p("write - ");
          if(activeInterrupt == _37ECWR) {
            L2_RED();
            p("Disk command register   - 0x37EC - %2X %1X %1X\n",b, a1, a0);
          }
          else
          if(activeInterrupt == _37E8WR) {
            L2_BLUE();
            p("Line printer data port  - 0x37E8 - %2X %1X %1X\n",b, a1, a0);
          }
          else
          if(activeInterrupt == _37E4WR) {
            L2_YELLOW();
            p("Cassette drive latch    - 0x37E4 - %2X %1X %1X\n",b, a1, a0);
          }
          else {
            L2_VIOLET();
            p("Disk drive select latch - %d - %2X %1X %1X\n",activeInterrupt, b, a1, a0);
          }
        }
        else {
          dataOutMode();
          L1_VIOLET();
          p("read  - ");
          if(activeInterrupt == _37ECRD) {
            L2_RED();
            p("Disk status register    - 0x37EC %1X %1X\n", a1, a0);
            setDataBus(0);
          }
          else
          if(activeInterrupt == _37E8RD) {
            L2_BLUE();
            p("Line printer status port- 0x37E8 %1X %1X\n", a1, a0);
            setDataBus(0);
          }
          else
          if(activeInterrupt == _37E4RD) {
            L2_YELLOW();
            p("Cassette drive latch    - 0x37E4 %1X %1X\n", a1, a0);
            setDataBus(0);
          }
          else {
            L2_VIOLET(); 
            p("Disk drive select latch - %d %1X %1X\n", activeInterrupt, a1, a0);
            setDataBus(i);           
          }
        
        }

        L1_GREEN();
        L2_GREEN();
        
        activeInterrupt = NOTHING;
        resetWaitLatch();
        //p("sei invokation\n");
        sei();  
    }
}
