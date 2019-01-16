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


#define CMD_RESTORE 0x00
#define CMD_SEEK    0x10
#define CMD_STEP    0x20
#define CMD_STEPIN  0x40
#define CMD_STEPOUT 0x60

#define CMD_READ    0x80
#define CMD_WRITE   0x90

#define CMD_READ_ADDR 0xc4
#define CMD_READ_TRK  0xe4
#define CMD_WRITE_TRK 0xf4

#define CMD_FORCE_INTERRUPT 0xd0

#define CMD_RESTORE_MASK 0xf0
#define CMD_SEEK_MASK 0xf0
#define CMD_STEP_MASK 0xe0
#define CMD_STEPIN_MASK 0xe0
#define CMD_STEPOUT_MASK 0xe0
#define CMD_READ_MASK 0xe0
#define CMD_WRITE_MASK 0xe0
#define CMD_READ_ADDR_MASK 0xff
#define CMD_READ_TRK_MASK 0xfe
#define CMD_WRITE_TRK_MASK 0xff
#define CMD_FORCE_INTERRUPT_MASK 0xf0

volatile int currentCommand;
volatile int currentDrive = 0xff;
volatile int trackNum = 0;
volatile int sectorNum = 0;
volatile int dataRegister = 0;
volatile int commandRegister = 0;
volatile int byteCtr = 0;
volatile int busyCtr = 0;
volatile int DRQCtr = 0;
volatile int statusRegister;
volatile int iIndexHole = 1;
volatile int iTrackDirection = IN;
volatile int motorRunningCtr;

volatile int sectorsRead = 0;
volatile int lastDiskCmdCtr;



void init1771Emulation() {
  statusRegister = NOTREADY | TRACKZERO;
}


inline void cmdRestore() {
    p("    RESTORE CMD\n");
    trackNum = 0;
    disk1File.seek(0);
    statusRegister = HEADENGAGED | TRACKZERO;
    currentCommand = CMD_RESTORE;
    //invokeTRS80Interrupt(DISK);
}

inline void cmdSeek() {
    p("    SEEK CMD\n");
    trackNum = dataRegister;
    disk1File.seek(trackNum * 2560 + sectorNum * 256);
    statusRegister = HEADENGAGED | BUSY;
    if(trackNum == 0)
        statusRegister |= TRACKZERO;
    busyCtr = 79;
    currentCommand = CMD_SEEK;
    //invokeTRS80Interrupt(DISK);    
}

inline void cmdStep() {
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
    currentCommand = CMD_STEP;  
    //invokeTRS80Interrupt(DISK);
}


inline void cmdStepIn() {
    p("    STEP IN CMD\n");
    if(commandRegister & 0x10 == 0x10) {           // only do this stuff if the update flag is set on the track register, otherwise just generate an interupt like we complied
      trackNum++;
    }
    iTrackDirection = IN; // IN is away from 0
    disk1File.seek(trackNum * 2560 + sectorNum * 256);
    statusRegister = INDEXHOLE;
    currentCommand = CMD_STEPIN;
    //invokeTRS80Interrupt(DISK);
}


inline void cmdStepOut() {  
    p("    STEP OUT CMD\n");
    if(commandRegister & 0x10 == 0x10) {           // only do this stuff if the update flag is set on the track register, otherwise just generate an interupt like we complied
      if(trackNum > 0) 
         trackNum--;
    }
    iTrackDirection = OUT;                         // OUT is toward 0
    disk1File.seek(trackNum * 2560 + sectorNum * 256);
    statusRegister = INDEXHOLE;
    currentCommand = CMD_STEPOUT;
    //invokeTRS80Interrupt(DISK);
}

inline void cmdRead() {
    byteCtr = 256;
    busyCtr = 260;
    p("    READ SECTOR CMD\n");
    statusRegister = BUSY;
    currentCommand = CMD_READ;
    DRQCtr = 0;
    if(sectorsRead == 0)
       DRQCtr = 4;
    else
       statusRegister |= DRQ;

    sectorsRead++;
    //invokeTRS80Interrupt(DISK);
}

inline void cmdForceInterrupt() {
    p("    FORCE INTERRUPT\n");
    statusRegister = INDEXHOLE;
    if(trackNum == 0)
        statusRegister |= TRACKZERO;

    currentCommand = CMD_FORCE_INTERRUPT;
}


inline void cmdWrite() {
  p("    WRITE CMD (unimpl)\n");
  currentCommand = CMD_WRITE;
}

inline void cmdReadAddr() {
   p("    READ ADDR (unimpl)\n");
   currentCommand = CMD_READ_ADDR;
}

inline void cmdReadTrk() {
  p("    READ TRK (unimpl)\n"); 
  currentCommand = CMD_READ_TRK ;
}

inline void cmdWriteTrk() {
  p("    WRITE TRK (unimpl)\n");
  currentCommand = CMD_WRITE_TRK;
}


void invokeCommand() {
    p("<::command reg::> \n");
    commandRegister = dataBus;
    lastDiskCmdCtr = 400;
  
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
  
  if((commandRegister & CMD_RESTORE_MASK) == CMD_RESTORE) {              // restore command
    cmdRestore();
    return;
  }
  if((commandRegister & CMD_READ_MASK) == CMD_READ) {        // read command
    cmdRead();
    return;
  }
  if((commandRegister & CMD_FORCE_INTERRUPT_MASK) == CMD_FORCE_INTERRUPT) {        // force interrupt command
     cmdForceInterrupt();
     return;
  }
  if((commandRegister & CMD_SEEK_MASK) == CMD_SEEK) {         // seek command
    cmdSeek();
    return;
  }
  if((commandRegister & CMD_STEP_MASK) == CMD_STEP) {         // step command
    cmdStep();
    return;
  }
  if((commandRegister & CMD_STEPIN_MASK) == CMD_STEPIN) {        // step in command
    cmdStepIn();
    return;
  }
  if((commandRegister & CMD_STEPOUT_MASK) == CMD_STEPOUT) {        // step out command
    cmdStepOut();
    return;
  }
  if((commandRegister & CMD_WRITE_MASK) == CMD_WRITE) {        // write command
    cmdWrite();
    return; 
  }
  if((commandRegister & CMD_READ_ADDR_MASK) == CMD_READ_ADDR) {               // read address command
    cmdReadAddr();
    return;
  }
  if((commandRegister & CMD_READ_TRK_MASK) == CMD_READ_TRK) {        // read track command
    cmdReadTrk();
    return;      
  }
  if((commandRegister & CMD_WRITE_TRK_MASK) == CMD_WRITE_TRK) {               // write track command
     cmdWriteTrk();
     return;
  }

  currentCommand = 0;
  p("    UNKNOWN COMMAND\n");
}


inline void driveSelect() {
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
    if(trackNum == 0) {
      statusRegister |= TRACKZERO;
    }
    motorRunningCtr = 80;
}

inline void setTrkReg() {
    p("<::track reg::> \n");
    trackNum = dataBus;  // track
    disk1File.seek(trackNum * 2560 + sectorNum * 256);
    statusRegister = INDEXHOLE;
    return;
}

inline void setSectorReg() {
    p("<::sector reg::> \n");
    sectorNum = dataBus; // sector
    disk1File.seek(trackNum * 2560 + sectorNum * 256);
    statusRegister = INDEXHOLE;
    return;
}

inline void setDataReg() {
    p("<::data reg::> \n");
    dataRegister = dataBus;  // data byte
    statusRegister = INDEXHOLE;
    return;
}


/* ***********************************************************************
 *  Data pushes from TRS-80
 *  **********************************************************************
 */
void PokeFromTRS80() {
  p(":---> (0x%02X)  ---> 0x%04X ",dataBus, address);  
  
  if((address & 0xfffc) == 0x37e0) { // drive select
     return driveSelect();
  } 
  if(address == 0x37ec) { // command invokation request
    return invokeCommand();
  }
  if(address == 0x37ed) { // set track register
    return setTrkReg();
  }
  if(address == 0x37ee) { // set sector register
    return setSectorReg();
  } 
  if(address == 0x37ef) {  // set data register
    return setDataReg();
  }

  p("Unhandled POKE!\n\n");
}


inline int getStatusRegister() {
    if(DRQCtr > 0) {
      DRQCtr--;
    }
    if(busyCtr > 0) {
      busyCtr--;
    }

    if(currentCommand == CMD_READ) {
      if(DRQCtr == 0) 
          statusRegister |= DRQ;
    }

    if(busyCtr == 0) {
      statusRegister &= ~(BUSY);
      statusRegister &= ~(DRQ);
    }

    if(motorRunningCtr == 0)
       statusRegister |= NOTREADY;
    else
       statusRegister &= ~(NOTREADY);

    p(" <--- (0x%02X) <::status reg::>\n",statusRegister);
    return statusRegister;
}

inline int getDataRegister() {
    if(byteCtr > 0) {
       byteCtr--;
       if(byteCtr == 0) {
          busyCtr = 3;
       }
       dataRegister = disk1File.read();
    }
    else {
      statusRegister &= ~(DRQ);
    }      
    
    p(" <--- (0x%02X) <::data reg:: [%d]> \n", dataRegister,byteCtr);
    return dataRegister;
}


/*****************************************************************
 * Read Requests from TRS-80
 * ***************************************************************
 */
int PeekFromTRS80() {
  p("<---: 0x%04X ",address); 

  if(address == 0x37ec) {             // read status register
    return getStatusRegister();
  }

  if(address == 0x37ef) {             // read data register
    return getDataRegister();
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
