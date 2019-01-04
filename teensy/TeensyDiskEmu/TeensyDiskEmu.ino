//for use with TRSHDD board v3.0

#include <Metro.h>

#include <BlockDriver.h>
#include <FreeStack.h>
#include <MinimumSerial.h>
#include <SdFat.h>
#include <SdFatConfig.h>
#include <sdios.h>
#include <SysCall.h>
#include <StringStream.h>



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

#define IN 0
#define OUT 1


volatile int activeInterrupt = NOTHING;
volatile int a0;
volatile int a1;
volatile int port_a;
volatile int port_c;
volatile int port_d;

volatile int currentDrive = 0xff;
volatile int trackNum = 0;
volatile int sectorNum = 0;
volatile byte dataRegister = 0;
volatile byte commandRegister = 0;
volatile int byteCtr;

volatile byte interruptStatus = 0x80;

typedef unsigned int boolean_t;
#define FALSE 0
#define TRUE !FALSE
typedef union {
        struct {
                boolean_t Type1_Busy:1;
                boolean_t Type1_IndexHole:1;
                boolean_t Type1_HeadAtZero:1;
                boolean_t Type1_CRCError:1;
                boolean_t Type1_TrackNotVerified:1;
                boolean_t Type1_HeadEngaged:1;
                boolean_t Type1_WriteProtected:1;
                boolean_t Type1_MotorsOff:1;   
        };
        struct {
                boolean_t Type2_Busy:1;
                boolean_t Type2_DataRequest:1;
                boolean_t Type2_LostData:1;
                boolean_t Type2_CRCError:1;
                boolean_t Type2_RecordNotFound:1;
                boolean_t Type2_RecTypeOrFault1:1;
                boolean_t Type2_RecTypeOrFault2:1;
                boolean_t Type2_MotorsOff:1;
        };
        struct {
                boolean_t Type3_Busy:1;
                boolean_t Type3_DataRequest:1;
                boolean_t Type3_LostData:1;
                boolean_t Type3_Unused1:1;
                boolean_t Type3_Unused2:1;
                boolean_t Type3_RecTypeOrFault1:1;
                boolean_t Type3_RecTypeOrFault2:1;
                boolean_t Type3_MotorsOff:1;                      
        };
        byte rawValue;
} statusRegisterDef;

volatile statusRegisterDef statusRegister;

volatile int iIndexHole = 1;
volatile int iTrackDirection = IN;


volatile int iBusDirection = 2;
volatile byte b = 0;
volatile int busvalue;
volatile byte currentCommand = 0;
#define CMD_NONE 0
#define CMD_READADDRESS 1

String strmOutput;
String disk1FileName = "NEWDOS_80sssd_jv1.DSK";


SdFatSdioEX sdEx;
File file;

File disk1File;

File diskImage1;



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
inline void dataOutMode() {
  if(iBusDirection == OUT)
     return;
  iBusDirection = OUT;

/*
  pinMode(D7,OUTPUT);
  pinMode(D6,OUTPUT);
  pinMode(D5,OUTPUT);
  pinMode(D4,OUTPUT);
  pinMode(D3,OUTPUT);
  pinMode(D2,OUTPUT);
  pinMode(D1,OUTPUT);
  pinMode(D0,OUTPUT);
*/

  GPIOA_PDDR = 143392;
  GPIOC_PDDR = 2318;
  GPIOD_PDDR = 253;
}


/* direct data bus pins inward */
inline void dataInMode() {
  if(iBusDirection == IN)
      return;
  iBusDirection = IN;
  /*
  pinMode(D7,INPUT);
  pinMode(D6,INPUT);
  pinMode(D5,INPUT);
  pinMode(D4,INPUT);
  pinMode(D3,INPUT);
  pinMode(D2,INPUT);
  pinMode(D1,INPUT);
  pinMode(D0,INPUT);
  */
  GPIOA_PDDR = 131104;
  GPIOC_PDDR = 2310;
  GPIOD_PDDR = 96;    
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
  pinMode(A1, INPUT);          // A1 from TRS-80
  pinMode(A0, INPUT);          // A0 from TRS-80
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

  pinMode(D7,INPUT);
  pinMode(D6,INPUT);
  pinMode(D5,INPUT);
  pinMode(D4,INPUT);
  pinMode(D3,INPUT);
  pinMode(D2,INPUT);
  pinMode(D1,INPUT);
  pinMode(D0,INPUT);
  iBusDirection = IN;
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
  //digitalWrite(FF_PRE,HIGH);
  digitalWriteFast(FF_CLR,LOW);
  delayMicroseconds(1);
  digitalWriteFast(FF_CLR,HIGH);
  delayMicroseconds(1);
  digitalWriteFast(INTERUPT_TO_TRS80, HIGH);
}


void BinaryStrZeroPad(int Number,char ZeroPadding){
signed char i=ZeroPadding;

  while(i>=0){
      if((Number & (1<<i)) > 0) Serial.write('1');
      else Serial.write('0');
      --i;
  }

        Serial.println();
}


inline int convertBusValue() {
  dataInMode();
  port_a = GPIOA_PDIR;
  port_c = GPIOC_PDIR;
  port_d = GPIOD_PDIR;
  
  unsigned int bv = (port_d & 0x0001); //D0, pin 2
  bv = bv + ((port_a & 0x1000) >> 11); //D1, pin 3
  bv = bv + ((port_a & 0x2000) >> 11); //D2, pin 4
  bv = bv + ((port_d & 0x0080) >> 4);  //D3, pin 5
  bv = bv + ((port_d & 0x0010));       //D4, pin 6
  bv = bv + ((port_d & 0x0004) << 3);  //D5, pin 7
  bv = bv + ((port_d & 0x0008) << 3);  //D6, pin 8
  bv = bv + ((port_c & 0x0008) << 4);  //D7, pin 9
  
  return bv;
}

void displayAddress(unsigned int address) {
  if(address == 0x37e0) 
    p("- Interrupt latch");
  else
  if(address == 0x37e1)
    p("- Disk drive select latch (1)");
  else
  if(address == 0x37e4)
    p("- Cassette select latch");
  else
  if(address == 0x37e8)
    p("- Line printer");
  else
  if(address == 0x37ec) 
    p("- disk controller command/status register");
  else
  if(address == 0x37ed) 
    p("- disk track position register");
  else
  if(address == 0x37ee)
    p("- disk sector register");
  else
  if(address == 0x37ef)
    p("- disk data register");
  else
    p("- unknown");  
}


void setStatusRegisterToNormalType1Response() {
    statusRegister.Type1_Busy = 0;
    statusRegister.Type1_IndexHole = (iIndexHole == 1);
    if(trackNum == 0) statusRegister.Type1_HeadAtZero = 1; else statusRegister.Type1_HeadAtZero = 0;
    statusRegister.Type1_CRCError = 0;
    statusRegister.Type1_TrackNotVerified = 0;
    statusRegister.Type1_HeadEngaged = 1;
    statusRegister.Type1_WriteProtected = 0;
    statusRegister.Type1_MotorsOff = 1;
}

void invokeCommand() {
  //
  // Command   Type    76543210
  //
  // RESTORE     1     0000XXXX
  // SEEK        1     0001XXXX
  // STEP        1     001uXXXX    u = update track register
  // STEP IN     1     010uXXXX
  // STEP OUT    1     011uXXXX
  // READ CMD    2     100mbXXX    m = muliple record, b = 128-1024 block length (0) or 16-4096 block length(1)
  // WRITE CMD   2     101mbXXX
  // READ ADDR   3     11000100
  // READ TRK    3     1110010X
  // WRITE TRK   3     11110100
  // FORCE INTRP 4     1101XXXX
  
  if((commandRegister & 0xf0) == 0x00) {              // restore command
    p(" RESTORE CMD ");
    trackNum = 0;
    disk1File.seek(0);
    currentCommand = CMD_NONE;
    setStatusRegisterToNormalType1Response();
    interruptStatus = 0x40;
    digitalWriteFast(INTERUPT_TO_TRS80, LOW);
  }
  else if((commandRegister & 0xf0) == 0x10) {         // seek command
    p(" SEEK CMD ");
    trackNum = dataRegister;
    disk1File.seek(trackNum * 2560);
    currentCommand = CMD_NONE;
    setStatusRegisterToNormalType1Response();
    interruptStatus = 0x40;
    digitalWriteFast(INTERUPT_TO_TRS80, LOW);
  }
  else if((commandRegister & 0xe0) == 0x20) {         // step command
    p(" STEP CMD ");
    if(commandRegister & 0x10 == 0x10) {            // only do this stuff if the update flag is set on the track register, otherwise just generate an interupt like we complied
      if(iTrackDirection == OUT) {
          if(trackNum > 0)
              trackNum--;
      }
      else {
        trackNum++;
      }
    }
    disk1File.seek(trackNum*2560);
    currentCommand = CMD_NONE;
    setStatusRegisterToNormalType1Response();
    interruptStatus = 0x40;
    digitalWriteFast(INTERUPT_TO_TRS80, LOW);
  }
  else if((commandRegister & 0xe0) == 0x40) {        // step in command
    p(" STEP IN CMD ");
    if(commandRegister & 0x10 == 0x10) {           // only do this stuff if the update flag is set on the track register, otherwise just generate an interupt like we complied
      trackNum++;
    }
    iTrackDirection = IN; // IN is away from 0
    currentCommand = CMD_NONE;
    disk1File.seek(trackNum*2560);
    setStatusRegisterToNormalType1Response();
    interruptStatus = 0x40;
    digitalWriteFast(INTERUPT_TO_TRS80, LOW);
  }
  else if((commandRegister & 0xe0) == 0x60) {        // step out command
    p(" STEP OUT CMD ");
    if(commandRegister & 0x10 == 0x10) {           // only do this stuff if the update flag is set on the track register, otherwise just generate an interupt like we complied
      if(trackNum > 0) 
         trackNum--;
    }
    iTrackDirection = OUT;                         // OUT is toward 0
    currentCommand = CMD_NONE;
    disk1File.seek(trackNum*2560);
    setStatusRegisterToNormalType1Response();
    interruptStatus = 0x40;
    digitalWriteFast(INTERUPT_TO_TRS80, LOW);
  }
  else if((commandRegister & 0xe0) == 0x80) {        // read command
     byteCtr = 256;

     p(" READ SECTOR CMD");

     statusRegister.Type2_Busy = 1;        
     statusRegister.Type2_DataRequest = 1;
     statusRegister.Type2_LostData = 0;
     statusRegister.Type2_CRCError = 0;
     statusRegister.Type2_RecordNotFound = 0;
     statusRegister.Type2_RecTypeOrFault1 = 0;
     statusRegister.Type2_RecTypeOrFault2 = 0;
     statusRegister.Type2_MotorsOff = 0;
     setDataBus(0x40);
     digitalWriteFast(INTERUPT_TO_TRS80, LOW);
  }
  else if((commandRegister & 0xe0) == 0x90) {        // write command
     p(" WRITE CMD (unimpl) ");    
  }
  else if(commandRegister == 0xc4) {               // read address command
     p(" READ ADDR (unimpl) ");
  }
  else if((commandRegister & 0xfe) == 0xe4) {        // read track command
     p(" READ TRK (unimpl) ");        
  }
  else if(commandRegister == 0xf4) {               // write track command
     p(" WRITE TRK (unimpl) ");        
  }
  else if((commandRegister & 0xf0) == 0xd0) {        // force interrupt command
     p(" FORCE INTERRUPT ");
     setStatusRegisterToNormalType1Response();
     currentCommand = CMD_NONE;            
  }
  else {
    p("- Unknown command: %x\n", commandRegister);
  }
}

void statusOut() {
  p("\nCD: %1d  TRK: %3d  SEC: %3d  DAT: %2X  SR: %2X\n\n", currentDrive, trackNum, sectorNum, dataRegister, statusRegister.rawValue);
}

void PokeFromTRS80(unsigned int address, int byt) {
  p("\nPoke: %8X, receiving: %x ",address,byt);
  iIndexHole = -iIndexHole;
  
  displayAddress(address);
  if((address & 0xfffc) == 0x37e0) { // drive select
    if(byt == 1)
      currentDrive = 0;
    else if(byt == 2)
      currentDrive = 1;
    else if(byt == 4)
      currentDrive = 2;
    else if(byt == 8)
      currentDrive = 3;
    else {
      p(" ??Received nonsense value for drive number, assuming 0??\n");
      currentDrive = 0;
    }

    setStatusRegisterToNormalType1Response(); 
    return;
  } 
  else
  if(address == 0x37ed) {
    trackNum = byt;  // track
    disk1File.seek(trackNum * 2560);
    setStatusRegisterToNormalType1Response(); 
    return;
  }
  else
  if(address == 0x37ee) {
    sectorNum = byt; // sector
    disk1File.seek(trackNum * 2560 + sectorNum * 256);
    setStatusRegisterToNormalType1Response(); 
    return;
  }
  else
  if(address == 0x37ef) {
    dataRegister = byt;  // data byte
    setStatusRegisterToNormalType1Response(); 
    return;
  }
  if(address == 0x37ec) {
    commandRegister = byt;
    invokeCommand();
    statusOut();
    return;
  }

  p("Unhandled POKE!\n\n");
}

byte PeekFromTRS80(unsigned int address) {
  p("\nPeek: %8X ",address); 
  iIndexHole = -iIndexHole;
  
  displayAddress(address);
  statusRegister.Type1_IndexHole = (iIndexHole == 1);
  if(trackNum == 0) statusRegister.Type1_HeadAtZero = 1; else statusRegister.Type1_HeadAtZero = 0;

  if(address == 0x37ec) {
    statusOut();
    return statusRegister.rawValue;
    //return 0;
  }

  if(address == 0x37ef) {
    if(statusRegister.Type2_Busy == 1) {
     byteCtr--;
     dataRegister = disk1File.read();
     p(" PEEK DATA %d %x", byteCtr, dataRegister);

     if(byteCtr > 0)
        statusRegister.Type2_Busy = 1;
     else {
        statusRegister.Type2_Busy = 0;
        statusRegister.Type2_MotorsOff = 1;
     }
    }
    return dataRegister;
  }

  if(address == 0x37ee) {
    return sectorNum;
  }

  if(address == 0x37ed) {
    return trackNum;
  }

  if(address == 0x37e0) {
    return interruptStatus;
  }

  p(" returning 0xfe - unknown behavior\n\n");

  return 0xfe;
}


#define GET_LOW_ADDRESSES()   a0 = digitalReadFast(A0); a1 = digitalReadFast(A1);
void _37E0WRInterrupt() {
  busvalue = convertBusValue();
  GET_LOW_ADDRESSES()
  PokeFromTRS80(0x37e0 + (a1<<1) + a0, busvalue); 
  resetWaitLatch();    
}

void _37E8WRInterrupt() {
  busvalue = convertBusValue();
  GET_LOW_ADDRESSES()
  PokeFromTRS80(0x37e8 + (a1<<1) + a0, busvalue); 
  resetWaitLatch();
} 

void _37E4WRInterrupt() {
  busvalue = convertBusValue();
  GET_LOW_ADDRESSES() 
  PokeFromTRS80(0x37e4 + (a1<<1) + a0, busvalue); 
  resetWaitLatch();
}

void _37ECWRInterrupt() {
  busvalue = convertBusValue();
  GET_LOW_ADDRESSES()
  PokeFromTRS80(0x37ec + (a1<<1) + a0, busvalue); 
  resetWaitLatch();
}


void _37ECRDInterrupt() {
  GET_LOW_ADDRESSES()
  setDataBus(PeekFromTRS80(0x37ec + (a1<<1) + a0));
  resetWaitLatch();
}

void _37E8RDInterrupt() {
  GET_LOW_ADDRESSES()
  setDataBus(PeekFromTRS80(0x37e8 + (a1<<1) + a0));
  resetWaitLatch();
}

void _37E4RDInterrupt() {
  GET_LOW_ADDRESSES()
  setDataBus(PeekFromTRS80(0x37e4 + (a1<<1) + a0));
  resetWaitLatch();
}

void _37E0RDInterrupt() {
  GET_LOW_ADDRESSES()
  setDataBus(PeekFromTRS80(0x37e0 + (a1<<1) + a0));
  interruptStatus = 0x00;  // reset interrupt status after this is invoked
  resetWaitLatch();  
}


/* set the value of the data bus */
inline void setDataBus(int b) {
  int x = b;
  dataOutMode();
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
  statusRegister.rawValue = 6;
  
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

  p("F_CPU: %d\n", F_CPU);
  p("F_BUS: %d\n", F_BUS);

  convertBusValue();
  
  StringStream stream(strmOutput);
  
  if (!sdEx.begin()) {
      sdEx.initErrorHalt("SdFatSdioEX begin() failed");
  }
  sdEx.chvol();

  sdEx.vwd()->rewind();
  while (file.openNext(sdEx.vwd(), O_RDONLY)) {
      if(!file.isHidden()) {
          file.printName(&stream);
          if (file.isDir()) {
           stream.write('/');
          }
          stream.flush();
          if(strmOutput == disk1FileName) {
             p("Opening disk 1 file: ");
             p((char*)strmOutput.c_str());
             disk1File = sdEx.open(strmOutput.c_str(), FILE_READ);
             if(!disk1File) {
                p("\n*** ERROR:  Unable to open file ***\n");
                L1_RED();
                L2_RED();
             }
             else {
              p("\n%d bytes in file.\n",disk1File.available());
             }
          }
          strmOutput = "";
      }
      file.close();
  }
  p("\nReady.\n");
  setStatusRegisterToNormalType1Response();
  sei();
  //disk1File.close();
}


Metro trs80ClockPulse = Metro(25);

void loop() {
  /*
    if(trs80ClockPulse.check() == 1) { // invoke clock interrupt every 25ms
       cli();
       interruptStatus = 0x80;
       if(iBusDirection == IN)
          pinMode(D7,OUTPUT);
       digitalWriteFast(D7,HIGH);
       digitalWriteFast(INTERUPT_TO_TRS80, LOW);
       if(iBusDirection == IN) {
          delayMicroseconds(1);
          pinMode(D7,INPUT);
       }
       sei();
    }
    */
}
