
#include <stdarg.h>

#define L1_RED()    digitalWriteFast(21,HIGH);digitalWriteFast(22,LOW);digitalWriteFast(23,LOW);
#define L1_GREEN()  digitalWriteFast(21,LOW);digitalWriteFast(22,HIGH);digitalWriteFast(23,LOW);
#define L1_BLUE()   digitalWriteFast(21,LOW);digitalWriteFast(22,LOW);digitalWriteFast(23,HIGH);
#define L1_YELLOW() digitalWriteFast(21,HIGH);digitalWriteFast(22,HIGH);digitalWriteFast(23,LOW);
#define L1_CYAN()   digitalWriteFast(21,LOW);digitalWriteFast(22,HIGH);digitalWriteFast(23,HIGH);
#define L1_VIOLET() digitalWriteFast(21,HIGH);digitalWriteFast(22,LOW);digitalWriteFast(23,HIGH);
#define L1_WHITE()  digitalWriteFast(21,HIGH);digitalWriteFast(22,HIGH);digitalWriteFast(23,HIGH);
#define L1_BLACK()  digitalWriteFast(21,LOW);digitalWriteFast(22,LOW);digitalWriteFast(23,LOW);

#define L2_BLUE()   digitalWriteFast(18,HIGH);digitalWriteFast(19,LOW);digitalWriteFast(20,LOW);
#define L2_GREEN()  digitalWriteFast(18,LOW);digitalWriteFast(19,HIGH);digitalWriteFast(20,LOW);
#define L2_RED()    digitalWriteFast(18,LOW);digitalWriteFast(19,LOW);digitalWriteFast(20,HIGH);
#define L2_YELLOW() digitalWriteFast(18,LOW);digitalWriteFast(19,HIGH);digitalWriteFast(20,HIGH);
#define L2_CYAN()   digitalWriteFast(18,HIGH);digitalWriteFast(19,HIGH);digitalWriteFast(20,LOW);
#define L2_VIOLET() digitalWriteFast(18,HIGH);digitalWriteFast(19,LOW);digitalWriteFast(20,HIGH);
#define L2_WHITE()  digitalWriteFast(18,HIGH);digitalWriteFast(19,HIGH);digitalWriteFast(20,HIGH);
#define L2_BLACK()  digitalWriteFast(18,LOW);digitalWriteFast(19,LOW);digitalWriteFast(20,LOW);


#define TRS80_REQUESTS_READ 0
#define TRS80_REQUESTS_WRITE 1


#define D0_PIN 2
#define D1_PIN 3
#define D2_PIN 4
#define D3_PIN 5
#define D4_PIN 6
#define D5_PIN 7
#define D6_PIN 8
#define D7_PIN 9

#define DATA_BUS_ENABLE 24
#define FF_CLR 25

#define TRS80_RD_ 26
#define TRS80_WR_ 27

#define INTRPT_PIN 28
#define FF_PRE 29

#define A0_PIN 33
#define A1_PIN 34
#define A2_PIN 35
#define A3_PIN 36
#define A4_PIN 37
#define A5_PIN 38
#define A6_PIN 39
#define A7_PIN 30


void p(char *fmt, ... ){
    char buf[80];
        
    va_list args;
    va_start (args, fmt );
    vsnprintf(buf, 128, fmt, args);
    va_end (args);
    Serial.print(buf);
}


inline void bounceFlipFlop() {
  digitalWriteFast(FF_PRE,HIGH);
  digitalWriteFast(FF_CLR,LOW);
  delayMicroseconds(1);
  digitalWriteFast(FF_CLR,HIGH);
}


void dataOutMode() {
  pinMode(D0_PIN,         OUTPUT);
  pinMode(D1_PIN,         OUTPUT);
  pinMode(D2_PIN,         OUTPUT);
  pinMode(D3_PIN,         OUTPUT);
  pinMode(D4_PIN,         OUTPUT);
  pinMode(D5_PIN,         OUTPUT);
  pinMode(D6_PIN,         OUTPUT);
  pinMode(D7_PIN,         OUTPUT);
}


void dataInMode() {
  pinMode(D0_PIN,         INPUT);
  pinMode(D1_PIN,         INPUT);
  pinMode(D2_PIN,         INPUT);
  pinMode(D3_PIN,         INPUT);
  pinMode(D4_PIN,         INPUT);
  pinMode(D5_PIN,         INPUT);
  pinMode(D6_PIN,         INPUT);
  pinMode(D7_PIN,         INPUT);  
}


void pinSetup() {
  pinMode(18,             OUTPUT);
  pinMode(19,             OUTPUT);
  pinMode(20,             OUTPUT);
  pinMode(21,             OUTPUT);
  pinMode(22,             OUTPUT);
  pinMode(23,             OUTPUT);
  dataInMode();
  pinMode(DATA_BUS_ENABLE,OUTPUT);
  pinMode(FF_CLR,         OUTPUT);
  pinMode(TRS80_RD_,      INPUT);
  pinMode(TRS80_WR_,      INPUT);
  pinMode(INTRPT_PIN,     INPUT);
  pinMode(FF_PRE,         OUTPUT);
  pinMode(A0_PIN,         INPUT);
  pinMode(A1_PIN,         INPUT);
  pinMode(A2_PIN,         INPUT);
  pinMode(A3_PIN,         INPUT);
  pinMode(A4_PIN,         INPUT);
  pinMode(A5_PIN,         INPUT);
  pinMode(A6_PIN,         INPUT);
  pinMode(A7_PIN,         INPUT);
}


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


void setup() {
  int i=0;
  
  cli();    // turn off interrupts
  
  Serial.begin(2000000); // high speed serial over USB

  pinSetup();  // set pin modes


  // turn LED1 and LED2 red
  L1_RED();
  L2_RED();
    

  // wait for up to 2 seconds for serial library to come online, flashing LED2 from yellow to red
  while(!Serial && i < 10) {
    i++;
    delay(100);
    L2_YELLOW();
    delay(100);
    L2_RED();
  }

  // init flipflop
  initFlipFlop();

  // disconnect 74LS245 data bus interface
  digitalWriteFast(DATA_BUS_ENABLE, LOW);
  
  // turn LED2 yellow
  L2_YELLOW();

  // attach interrupt function
  attachInterrupt(digitalPinToInterrupt(INTRPT_PIN), interruptTriggered, RISING);

  // turn LED1 & LED2 green
  L2_GREEN();
  L1_GREEN();

  // enable interrupts
  
  
  
  sei();
}


byte getLSBOfAddress() {
  volatile byte b;
  
  b  = (digitalReadFast(A7_PIN) <<7);
  b += (digitalReadFast(A6_PIN) <<6);
  b += (digitalReadFast(A5_PIN) <<5);
  b += (digitalReadFast(A4_PIN) <<4);
  b += (digitalReadFast(A3_PIN) <<3);
  b += (digitalReadFast(A2_PIN) <<2);
  b += (digitalReadFast(A1_PIN) <<1);
  b +=  digitalReadFast(A0_PIN);

  return b;
}

byte getDataBus() {
  volatile byte b =0;
  dataInMode();

p("K");
  digitalWrite(DATA_BUS_ENABLE, HIGH);
  delayMicroseconds(1);
/*
  b  = (digitalReadFast(D7_PIN) << 7);
  b |= (digitalReadFast(D6_PIN) << 6);
  b |= (digitalReadFast(D5_PIN) << 5);
  b |= (digitalReadFast(D4_PIN) << 4);
  b |= (digitalReadFast(D3_PIN) << 3);
  b |= (digitalReadFast(D2_PIN) << 2);
  b |= (digitalReadFast(D1_PIN) << 1);
  b |=  digitalReadFast(D0_PIN) ;
*/
  digitalWrite(DATA_BUS_ENABLE, LOW); 
delayMicroseconds(1);
p("L\n");
  return b;
}

void setDataBus(byte b) {
  dataOutMode();
  digitalWriteFast(DATA_BUS_ENABLE, HIGH);
  if(b >= 128) { digitalWriteFast(D7_PIN, HIGH); b=b-128; } else digitalWriteFast(D7_PIN, LOW);
  if(b >= 64)  { digitalWriteFast(D6_PIN, HIGH); b=b-64;  } else digitalWriteFast(D6_PIN, LOW);
  if(b >= 32)  { digitalWriteFast(D5_PIN, HIGH); b=b-32;  } else digitalWriteFast(D5_PIN, LOW);
  if(b >= 16)  { digitalWriteFast(D4_PIN, HIGH); b=b-16;  } else digitalWriteFast(D4_PIN, LOW);
  if(b >=  8)  { digitalWriteFast(D3_PIN, HIGH); b=b-8;   } else digitalWriteFast(D3_PIN, LOW);
  if(b >=  4)  { digitalWriteFast(D2_PIN, HIGH); b=b-4;   } else digitalWriteFast(D2_PIN, LOW);
  if(b >=  2)  { digitalWriteFast(D1_PIN, HIGH); b=b-2;   } else digitalWriteFast(D1_PIN, LOW);
  if(b >=  1)  { digitalWriteFast(D0_PIN, HIGH); b=b-1;   } else digitalWriteFast(D0_PIN, LOW);
  //delay(200);
}


volatile byte addr;
volatile byte bRW;
volatile byte ch;

inline byte getReadWriteMode() {
  if(digitalReadFast(TRS80_RD_) == LOW)
    return TRS80_REQUESTS_READ;

  if(digitalReadFast(TRS80_WR_) == LOW)
    return TRS80_REQUESTS_WRITE;

    return 255;
}

void interruptTriggered() {
  cli();  
  //L2_WHITE();
  
  bRW = getReadWriteMode();
  addr = getLSBOfAddress();

  if(bRW == TRS80_REQUESTS_READ) {
  }
  else {
    ch = getDataBus();
  }

  if(addr >= 0xe0 && addr <= 0xef) {
    L2_BLUE();
    
    if(bRW == TRS80_REQUESTS_READ)
      p("Interrupt triggered,  addr=%#2x, rw=%d\n",addr,bRW);
    else
      p("Interrupt triggered,  addr=%#2x, rw=%d, ch=%d\n",addr,bRW, ch);
    
    switch(addr) {
      case 0xe0:
         p("Disk Drive Select Latch.\n");
      break;

      case 0xe4:
         p("Cassette Drive Latch.\n");
      break;

      case 0xe8:
      if(bRW == TRS80_REQUESTS_READ) {
         p("Line Printer Status Request, sending 90\n");
         setDataBus(90);
      }
      else {
         p("Line Printer Data Send\n");
      }
      break;

      case 0xec:
      if(bRW == TRS80_REQUESTS_READ) {
         p("Disk Status Request, sending 0\n");
         setDataBus(0);
      }
      else {
         p("Disk Data Send\n");
      }
      break;

      case 0xed:
         p("Disk Track Register\n");
      break;

      case 0xee:
         p("Disk Sector Register\n");
      break;
      
      case 0xef:
         p("Disk Data Register\n");
      break;

      default:
         p("Unknown command, %#2x\n", addr);
    }
  }
  digitalWriteFast(DATA_BUS_ENABLE, LOW);

  L2_BLACK();  
  sei();
  bounceFlipFlop();
}


void loop() {
}
