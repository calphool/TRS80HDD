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
#include "defines.h"


extern File disk1File;
extern String disk1FileName;
extern volatile int motorRunningCtr;
extern volatile int sectorsRead;



volatile int dataBusDirection = -1;
volatile int iBusDirection = 2;
volatile int dataBus;
volatile int address;
volatile int interruptStatus;



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
  dataBusOutFromTeensyMode();
  dataBusInToTeensyMode();
}


/* initialize output pins */
void initialPinState() {
  digitalWriteFast(INTERUPT_TO_TRS80, HIGH); // turn off TRS-80 interrupt line (it's active low)
  initFlipFlop();                            // jigger flip flop into a known state (loaded, ready for trigger)
}



IntervalTimer it;




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


/* initialize everything */
void setup() {
  int iLEDCtr = 0; 
  
  cli();                              // turn off interrupts during setup()
    
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
  
  p((char*)"\nReady.\n");
  if(!Serial) {
    L2_YELLOW();                     // unable to communicate over serial, show L2 as yellow
    L1_GREEN();
  }
  else {
    L1_GREEN();                      // all good to go
    L2_GREEN();
  }

  init1771Emulation();

  p((char*)"Setting up interval timer: %d", it.begin(clockTick, 25000));
   
  sei();                             // enable interrupts
}


void clockTick() {
  // comment out if you want to turn off the clock functionality...
  interruptStatus = interruptStatus | 0x80;
  digitalWriteFast(INTERUPT_TO_TRS80,LOW);
}


void loop() {  
}
