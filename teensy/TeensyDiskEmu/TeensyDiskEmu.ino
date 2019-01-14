//for use with TRSHDD board v4

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


#define L1_RED()    digitalWriteFast(5,HIGH);digitalWriteFast(7,LOW); digitalWriteFast(6,LOW);
#define L1_GREEN()  digitalWriteFast(5,LOW); digitalWriteFast(7,LOW); digitalWriteFast(6,HIGH);
#define L1_BLUE()   digitalWriteFast(5,LOW); digitalWriteFast(7,HIGH);digitalWriteFast(6,LOW);
#define L1_YELLOW() digitalWriteFast(5,HIGH);digitalWriteFast(7,LOW); digitalWriteFast(6,HIGH);
#define L1_CYAN()   digitalWriteFast(5,LOW); digitalWriteFast(7,HIGH);digitalWriteFast(6,HIGH);
#define L1_VIOLET() digitalWriteFast(5,HIGH);digitalWriteFast(7,HIGH);digitalWriteFast(6,LOW);
#define L1_WHITE()  digitalWriteFast(5,HIGH);digitalWriteFast(7,HIGH);digitalWriteFast(6,HIGH);
#define L1_BLACK()  digitalWriteFast(5,LOW); digitalWriteFast(7,LOW); digitalWriteFast(6,LOW);

#define L2_RED()    digitalWriteFast(2,HIGH);digitalWriteFast(4,LOW); digitalWriteFast(3,LOW);
#define L2_BLUE()   digitalWriteFast(2,LOW); digitalWriteFast(4,HIGH);digitalWriteFast(3,LOW);
#define L2_GREEN()  digitalWriteFast(2,LOW); digitalWriteFast(4,LOW); digitalWriteFast(3,HIGH);
#define L2_CYAN()   digitalWriteFast(2,LOW); digitalWriteFast(4,HIGH);digitalWriteFast(3,HIGH);
#define L2_VIOLET() digitalWriteFast(2,HIGH);digitalWriteFast(4,HIGH);digitalWriteFast(3,LOW);
#define L2_YELLOW() digitalWriteFast(2,HIGH);digitalWriteFast(4,LOW); digitalWriteFast(3,HIGH);
#define L2_WHITE()  digitalWriteFast(2,HIGH);digitalWriteFast(4,HIGH);digitalWriteFast(3,HIGH);
#define L2_BLACK()  digitalWriteFast(2,LOW); digitalWriteFast(4,LOW); digitalWriteFast(3,LOW);


//                        0 - disconnected
//                        1 - disconnected
#define LED2_RED          2 /* PORTD BIT 0 */
#define LED2_GREEN        3 /* PORTA BIT 12 */
#define LED2_BLUE         4 /* PORTA BIT 13 */
#define LED1_RED          5 /* PORTD BIT 7 */
#define LED1_GREEN        6 /* PORTD BIT 4 */
#define LED1_BLUE         7 /* PORTD BIT 2 */

#define INTERUPT_TO_TRS80 8 /* PORTD BIT 3 */

#define D3                9 /* PORTC BIT 3 */
#define D4               10 /* PORTC BIT 4 */
#define D6               11 /* PORTC BIT 6 */
#define D7               12 /* PORTC BIT 7 */
#define D5               13 /* PORTC BIT 5 */
//                       14 - disconnected
#define D0               15 /* PORTC BIT 0 */
//                       16 - disconnected
//                       17 - disconnected
//                       18 - disconnected
//                       19 - disconnected
//                       20 - disconnected
//                       21 - disconnected
#define D1               22 /* PORTC BIT 1 */
#define D2               23 /* PORTC BIT 2 */
//                       24 - disconnected
#define FF_CLR           25 /* PORTA BIT 5 */
#define _37ECRD          26 /* PORTA BIT 14 */
#define _37E0WR          27 /* PORTA BIT 15 */
#define _37E4WR          28 /* PORTA BIT 16 */
#define FF_PRE           29 /* PORTB BIT 18 */
#define _37E8RD          30 /* PORTB BIT 19 */
#define _37E0RD          31 /* PORTB BIT 10 */
#define _37E4RD          32 /* PORTB BIT 11 */
#define _37E8WR          33 /* PORTE BIT 24 */
#define _37ECWR          34 /* PORTE BIT 25 */
#define _A0              35 /* PORTC BIT 8 */
#define _A1              36 /* PORTC BIT 9 */
#define NOTHING1         37 /* PORTC BIT 10 */
#define NOTHING2         38 /* PORTC BIT 11 */
#define NOTHING3         39 /* PORTA BIT 17 */

#define IN 0
#define OUT 1

volatile int currentDrive = 0xff;
volatile int trackNum = 0;
volatile int sectorNum = 0;
volatile int dataRegister = 0;
volatile int commandRegister = 0;
volatile int byteCtr = 0;
volatile int busyCtr = 0;
volatile int dataBusDirection = -1;


// type 1 status bits
#define BUSY 0x01
#define INDEXHOLE 0x02
#define TRACKZERO 0x04
#define CRCERR 0x08
#define SEEKERR 0x10
#define HEADENGAGED 0x20
#define WRITEPROT 0x40
#define NOTREADY 0x80

// read status bits
#define DRQ 0x02
#define LOSTDATA 0x04
#define NOTFND 0x10
#define FA 0x20
#define F9 0x40
#define F8 0x60
#define FB 0x00

// write status bits
#define WRITEFAULT 0x20


volatile int statusRegister;

volatile int iIndexHole = 1;
volatile int iTrackDirection = IN;
volatile int iBusDirection = 2;
volatile int dataBus;
volatile int address;

String disk1FileName = "NEWDOS_80sssd_jv1.DSK";
File disk1File;
SdFatSdioEX sdEx;


/* printf() to serial output */
void p(char *fmt, ... ){
    char buf[128];        
    va_list args;
    va_start (args, fmt );
    vsnprintf(buf, 128, fmt, args);
    va_end (args);
    Serial.print(buf);
    Serial.flush();
}


inline void dataBusOutFromTeensyMode() {
  // didn't seem to be working, replaced with pinMode for now...
  //GPIOC_PDDR = GPIOC_PDDR | 0x00ff;

  if(iBusDirection == OUT) return;
  
  pinMode(D0,OUTPUT);
  pinMode(D1,OUTPUT);
  pinMode(D2,OUTPUT);
  pinMode(D3,OUTPUT);
  pinMode(D4,OUTPUT);
  pinMode(D5,OUTPUT);
  pinMode(D6,OUTPUT);
  pinMode(D7,OUTPUT);  
  iBusDirection = OUT;
}


inline void dataBusInToTeensyMode() {
  if(iBusDirection == IN) return;
  
  GPIOC_PDDR = GPIOC_PDDR & 0xff00;
  iBusDirection = IN;
}



/* set up pin directionality */
void pinSetup() {
  pinMode(LED1_RED, OUTPUT);
  pinMode(LED1_GREEN, OUTPUT);
  pinMode(LED1_BLUE, OUTPUT);
  pinMode(LED2_RED, OUTPUT);
  pinMode(LED2_GREEN, OUTPUT);
  pinMode(LED2_BLUE, OUTPUT);
  pinMode(INTERUPT_TO_TRS80, OUTPUT);
  pinMode(FF_CLR, OUTPUT);
  pinMode(FF_PRE, OUTPUT);
  pinMode(_37E0RD, INPUT);
  pinMode(_37E0WR, INPUT);
  pinMode(_37E4RD, INPUT);
  pinMode(_37E4WR, INPUT);
  pinMode(_37E8RD, INPUT);
  pinMode(_37E8WR, INPUT);
  pinMode(_37ECRD, INPUT);
  pinMode(_37ECWR, INPUT);
  pinMode(_A0, INPUT);
  pinMode(_A1, INPUT);
  pinMode(NOTHING1, OUTPUT);
  pinMode(NOTHING2, OUTPUT);
  pinMode(NOTHING3, OUTPUT);
  dataBusInToTeensyMode();
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


inline int getDataBusValue() {
  dataBusInToTeensyMode();
  return (GPIOC_PDIR & 0x00ff);
}


#define DISK  0x40
#define CLOCK 0x80

inline void invokeTRS80Interrupt(int type) {
  setDataBus(type);
  digitalWriteFast(INTERUPT_TO_TRS80, LOW);
  delayMicroseconds(1);
  digitalWriteFast(INTERUPT_TO_TRS80, HIGH);
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
    p("    RESTORE CMD\n");
    trackNum = 0;
    disk1File.seek(0);
    statusRegister = HEADENGAGED | TRACKZERO;
    invokeTRS80Interrupt(DISK);
  }
  else if((commandRegister & 0xf0) == 0x10) {         // seek command
    p("    SEEK CMD\n");
    trackNum = dataRegister;
    disk1File.seek(trackNum * 2560 + sectorNum * 256);
    statusRegister = HEADENGAGED;
    busyCtr = 77;
    invokeTRS80Interrupt(DISK);  
  }
  else if((commandRegister & 0xe0) == 0x20) {         // step command
    p("    STEP CMD\n");
    if((commandRegister & 0x10) == 0x10) {            // only do this stuff if the update flag is set on the track register, otherwise just generate an interupt like we complied
      if(iTrackDirection == OUT) {
          if(trackNum > 0)
              trackNum--;
      }
      else {
        trackNum++;
      }
    }
    disk1File.seek(trackNum * 2560 + sectorNum * 256);
    statusRegister = INDEXHOLE;    
    invokeTRS80Interrupt(DISK);
  }
  else if((commandRegister & 0xe0) == 0x40) {        // step in command
    p("    STEP IN CMD\n");
    if(commandRegister & 0x10 == 0x10) {           // only do this stuff if the update flag is set on the track register, otherwise just generate an interupt like we complied
      trackNum++;
    }
    iTrackDirection = IN; // IN is away from 0
    disk1File.seek(trackNum * 2560 + sectorNum * 256);
    statusRegister = INDEXHOLE;
    invokeTRS80Interrupt(DISK);
  }
  else if((commandRegister & 0xe0) == 0x60) {        // step out command
    p("    STEP OUT CMD\n");
    if(commandRegister & 0x10 == 0x10) {           // only do this stuff if the update flag is set on the track register, otherwise just generate an interupt like we complied
      if(trackNum > 0) 
         trackNum--;
    }
    iTrackDirection = OUT;                         // OUT is toward 0
    disk1File.seek(trackNum * 2560 + sectorNum * 256);
    statusRegister = INDEXHOLE;
    invokeTRS80Interrupt(DISK);
  }
  else if((commandRegister & 0xe0) == 0x80) {        // read command
    byteCtr = 256;
    busyCtr = 259;
    p("    READ SECTOR CMD\n");
    statusRegister = BUSY;
    invokeTRS80Interrupt(DISK);
  }
  else if((commandRegister & 0xe0) == 0x90) {        // write command
     p("    WRITE CMD (unimpl)\n");    
  }
  else if(commandRegister == 0xc4) {               // read address command
     p("    READ ADDR (unimpl)\n");
  }
  else if((commandRegister & 0xfe) == 0xe4) {        // read track command
     p("    READ TRK (unimpl)\n");        
  }
  else if(commandRegister == 0xf4) {               // write track command
     p("    WRITE TRK (unimpl)\n");        
  }
  else if((commandRegister & 0xf0) == 0xd0) {        // force interrupt command
     p("    FORCE INTERRUPT\n");
    statusRegister = INDEXHOLE;
    if(trackNum == 0)
        statusRegister |= TRACKZERO;    
  }
  else {
    p("    UNKNOWN COMMAND\n");
  }
}


void PokeFromTRS80() {
  p(":---> (0x%02X)  ---> 0x%04X ",dataBus, address);  
  //p("\n");return;
  
  if((address & 0xfffc) == 0x37e0) { // drive select
    p("<::drive select::> \n");
    if(dataBus == 1)
      currentDrive = 0;
    else if(dataBus == 2)
      currentDrive = 1;
    else if(dataBus == 4)
      currentDrive = 2;
    else if(dataBus == 8)
      currentDrive = 3;
    else {
      p(" ??Received nonsense value for drive number, assuming 0??\n");
      currentDrive = 0;
    }
    statusRegister = HEADENGAGED;
    return;
  } 
  else
  if(address == 0x37ec) { // command invokation request
    p("<::command reg::> \n");
    commandRegister = dataBus;
    invokeCommand();
    return;
  }
  else
  if(address == 0x37ed) {
    p("<::track reg::> \n");
    trackNum = dataBus;  // track
    disk1File.seek(trackNum * 2560 + sectorNum * 256);
    statusRegister = INDEXHOLE;
    return;
  }
  else
  if(address == 0x37ee) {
    p("<::sector reg::> \n");
    sectorNum = dataBus; // sector
    disk1File.seek(trackNum * 2560 + sectorNum * 256);
    statusRegister = INDEXHOLE;
    return;
  }
  else
  if(address == 0x37ef) {
    p("<::data reg::> \n");
    dataRegister = dataBus;  // data byte
    statusRegister = INDEXHOLE;
    return;
  }

  p("Unhandled POKE!\n\n");
}

int PeekFromTRS80() {
  p("<---: 0x%04X ",address); 

  if(address == 0x37ec) {             // read status register
    p(" <--- (0x%02X) <::status reg::>\n",statusRegister);
    if(busyCtr > 0) {
      busyCtr--;
    }
    else {
      statusRegister &= ~(BUSY);        
    }
    if(byteCtr > 0) {
       statusRegister |= DRQ;      
    }
        
    return statusRegister;
    //return 0;                       // return a zero if you want to boot without drives
  }

  if(address == 0x37ef) {             // read data register
    if(byteCtr > 0) {
       byteCtr--;
       if(byteCtr == 0) busyCtr = 3;
       dataRegister = disk1File.read();
       p(" <--- (0x%02X) <::data reg::> \n", dataRegister);
    }
    else {
      statusRegister &= ~(DRQ);
      p(" <--- (0x%02X) <::data reg::> \n", dataRegister);
    }
    return dataRegister;
  }

  if(address == 0x37ee) {              // read sector register
    p(" <--- (0x%02X) <::sector reg::>\n", sectorNum);
    return sectorNum;
  }

  if(address == 0x37ed) {              // read track register
    p(" <--- (0x%02X) <::track reg::>\n", trackNum);
    return trackNum;
  }  

  if(address == 0x37e0) {             // read interrupt latch (supposed to reset the latch)
    p(" <--- (0x80) <::interupt latch::>\n");
    return 0x80;
  }

  p(" <--- (0xfe) <::HUH Why Am I Here?::>\n");
  return 0xfe;
}


void _37E0WRInterrupt() {
  dataBus = getDataBusValue();
  address = 0x37e0 + (GPIOC_PDIR>>8);
  PokeFromTRS80(); 
  resetWaitLatch();    
}

void _37E8WRInterrupt() {
  dataBus = getDataBusValue();
  address = 0x37e8 + (GPIOC_PDIR>>8);
  PokeFromTRS80(); 
  resetWaitLatch();
} 

void _37E4WRInterrupt() {
  dataBus = getDataBusValue();
  address = 0x37e4 + (GPIOC_PDIR>>8);
  PokeFromTRS80(); 
  resetWaitLatch();
}

void _37ECWRInterrupt() {
  dataBus = getDataBusValue();
  address = 0x37ec + (GPIOC_PDIR>>8);
  PokeFromTRS80(); 
  resetWaitLatch();
}


void _37ECRDInterrupt() {
  address = 0x37ec + (GPIOC_PDIR>>8);
  setDataBus(PeekFromTRS80());
  resetWaitLatch();
}

void _37E8RDInterrupt() {
  address = 0x37e8 + (GPIOC_PDIR>>8);
  setDataBus(PeekFromTRS80());
  resetWaitLatch();
}

void _37E4RDInterrupt() {
  address = 0x37e4 + (GPIOC_PDIR>>8);
  setDataBus(PeekFromTRS80());
  resetWaitLatch();
}

void _37E0RDInterrupt() {
  address = 0x37e0 + (GPIOC_PDIR>>8);
  setDataBus(PeekFromTRS80());
  resetWaitLatch();  
}


/* set the value of the data bus */
inline void setDataBus(int b) {
  dataBusOutFromTeensyMode();
  if(b >= 128) {digitalWriteFast(D7,HIGH); b=b-128;} else {digitalWriteFast(D7,LOW);}
  if(b >= 64)  {digitalWriteFast(D6,HIGH); b=b-64; } else {digitalWriteFast(D6,LOW);}
  if(b >= 32)  {digitalWriteFast(D5,HIGH); b=b-32; } else {digitalWriteFast(D5,LOW);}
  if(b >= 16)  {digitalWriteFast(D4,HIGH); b=b-16; } else {digitalWriteFast(D4,LOW);}
  if(b >= 8)   {digitalWriteFast(D3,HIGH); b=b-8;  } else {digitalWriteFast(D3,LOW);}
  if(b >= 4)   {digitalWriteFast(D2,HIGH); b=b-4;  } else {digitalWriteFast(D2,LOW);}
  if(b >= 2)   {digitalWriteFast(D1,HIGH); b=b-2;  } else {digitalWriteFast(D1,LOW);}
  if(b >= 1)   {digitalWriteFast(D0,HIGH); b=b-1;  } else {digitalWriteFast(D0,LOW);}
}


/* wire up the interrupts */
void configureInterrupts() {
  attachInterrupt(digitalPinToInterrupt(_37ECRD), _37ECRDInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(_37E8RD), _37E8RDInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(_37E4RD), _37E4RDInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(_37E0RD), _37E0RDInterrupt, FALLING);

  attachInterrupt(digitalPinToInterrupt(_37ECWR), _37ECWRInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(_37E8WR), _37E8WRInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(_37E4WR), _37E4WRInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(_37E0WR), _37E0WRInterrupt, FALLING);
}



void openDiskFileByName(String sFileName) {
  File file;
  String strmOutput;
  
  StringStream stream(strmOutput); // set up string stream to capture SD card directory
  
  if (!sdEx.begin()) {             // couldn't establish SD card connection
      p("ERROR:  Unable to open SdFatSdioEX object.\n");
      sdEx.initErrorHalt("SdFatSdioEX begin() failed");
      L2_RED();
      L1_RED();
      return;
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
          if(strmOutput == sFileName) {
             p("Opening disk 1 file: ");
             p((char*)strmOutput.c_str());
             disk1File = sdEx.open(strmOutput.c_str(), FILE_READ);
             if(!disk1File) {
                p("\nERROR:  Unable to open file\n");
                L1_RED();
                L2_RED();
                file.close();
                return;
             }
             else {
              p("\n%d bytes in file.\n",disk1File.available());
             }
          }
          strmOutput = "";
      }
      file.close();
  }

  return;
}




/* initialize everything */
void setup() {
  int iLEDCtr = 0; 
  
  cli();                              // turn off interrupts during setup()
  
  statusRegister = NOTREADY | TRACKZERO;              // default value for status register
  
  Serial.begin(2000000);              // high speed serial over USB
  pinSetup();                         // set pin modes
  initialPinState();                  // put pins in initial configuration

  L1_RED();
  L2_RED();
  while(!Serial && iLEDCtr < 30) {    // attempt to establish serial connection from Teensy
    iLEDCtr++;
    delay(100);
    L2_YELLOW();
    delay(100);
    L2_RED();
  }

  if(!Serial) {                       // couldn't establish serial connection within 6 seconds ((100+100)*30 milliseconds)
    L2_YELLOW();
    L1_YELLOW();
  }
  else {                              // okay, serial established, let's move on
    L2_CYAN();
    L1_CYAN();
  }
  
  configureInterrupts();                             // tie interrupt lines to code blocks
  openDiskFileByName(disk1FileName);                 // open file specified from SD card
  
  p("\nReady.\n");
  if(!Serial) {
    L2_YELLOW();                     // unable to communicate over serial, show L2 as yellow
    L1_GREEN();
  }
  else {
    L1_GREEN();                      // all good to go
    L2_GREEN();
  }
   
  sei();                             // enable interrupts
}





Metro trs80ClockPulse = Metro(25);
void loop() {  
    if(trs80ClockPulse.check() == 1) { // invoke clock interrupt every 25ms
       cli();
       invokeTRS80Interrupt(CLOCK);
       sei();
    }    
}
