#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h> // needed for memset
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <time.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <glob.h>


#define RETURNCODE int
#define OK 0



/****************************************************************************
globals
*****************************************************************************/
typedef struct {
  char cmdString[256];
}
command;
unsigned int iSerialDeviceCtr = 0;
unsigned int iActiveDevice = -1;
char serialDeviceNames[256][256];
char buf[4096];
char workbuff[512];
char write_buffer[4096];
char read_buffer[4096];


unsigned char delayms;
int tty_fd;
long chksum;
unsigned int byteCtr;
unsigned long szBuf;





/****************************************************************************
stuff to put in a utility library - START
*****************************************************************************/

char* stristr( const char* str1, const char* str2 )
{
    const char* p1 = str1 ;
    const char* p2 = str2 ;
    const char* r = *p2 == 0 ? str1 : 0 ;

    while( *p1 != 0 && *p2 != 0 ){
        if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) ){
            if( r == 0 ){
                r = p1 ;
            }
            p2++ ;
        }
        else{
            p2 = str2 ;
            if( r != 0 ){
                p1 = r + 1 ;
            }

            if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) ){
                r = p1 ;
                p2++ ;
            }
            else{
                r = 0 ;
            }
        }
        p1++ ;
    }
    return *p2 == 0 ? (char*)r : 0 ;
}

int stricmp(const char *a, const char *b, int x) {
  int ca, cb,c;
  c=0;
  do {
     ca = (unsigned char) *a++;
     cb = (unsigned char) *b++;
     ca = tolower(toupper(ca));
     cb = tolower(toupper(cb));
     c++;
   } while (c < x && ca == cb && ca != '\0');
   return ca - cb;
}

int EndsWith(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return stricmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

void String_Upper(char string[]) 
{
	int i = 0;
 
	while (string[i] != '\0') 
	{
    	if (string[i] >= 'a' && string[i] <= 'z') {
        	string[i] = string[i] - 32;
    	}
      	i++;
	}
}
void String_Lower(char string[]) 
{
    int i = 0;
 
    while (string[i] != '\0') 
    {
        if (string[i] >= 'A' && string[i] <= 'Z') {
            string[i] = string[i] + 32;
        }
        i++;
    }
}

// if the string "str" starts with "pre" then returh true
bool startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : stricmp(pre, str, lenpre) == 0;
}


char *trim(char *str)
{
    size_t len = 0;
    char *frontp = str;
    char *endp = NULL;

    if( str == NULL ) { return NULL; }
    if( str[0] == '\0' ) { return str; }

    len = strlen(str);
    endp = str + len;

    /* Move the front and back pointers to address the first non-whitespace
     * characters from each end.
     */
    while( isspace((unsigned char) *frontp) ) { ++frontp; }
    if( endp != frontp )
    {
        while( isspace((unsigned char) *(--endp)) && endp != frontp ) {}
    }

    if( str + len - 1 != endp )
            *(endp + 1) = '\0';
    else if( frontp != str &&  endp == frontp )
            *str = '\0';

    /* Shift the string so that it starts at str so that if it's dynamically
     * allocated, we can still free it on the returned pointer.  Note the reuse
     * of endp to mean the front of the string buffer now.
     */
    endp = str;
    if( frontp != str )
    {
            while( *frontp ) { *endp++ = *frontp++; }
            *endp = '\0';
    }


    return str;
}

/****************************************************************************
stuff to put in a utility library - END
*****************************************************************************/



RETURNCODE getSerialDevices(char* desiredDevice) {
    DIR *dp;
    struct dirent *ep;
 
    //printf("Searching for non-bluetooth tty devices...\n");
    memset(&serialDeviceNames,0x0,256*256);

    dp = opendir("/dev");
    if(dp != NULL) {
    	while((ep = readdir(dp)) != NULL) {
    		strcpy(buf,ep->d_name);
    		//String_Upper(buf);
    		if(startsWith("TTY.",buf)) {
    			if(stristr(buf,"BLUE") == NULL) {
	    			strcat(serialDeviceNames[iSerialDeviceCtr], ep->d_name);
            if(stricmp(desiredDevice, buf, strlen(desiredDevice)) == 0)
                iActiveDevice = iSerialDeviceCtr;
	    			iSerialDeviceCtr++;
	    			if(iSerialDeviceCtr > 255) {
	    				closedir(dp);
	    				perror("Too many serial devices!\n");
	    				return -2;
	    			}
    			}
    		}
    	}
    	closedir(dp);
    }
    else {
    	perror("Couldn't open device directory /dev");
    	return -1;
    }	

    return OK;
}

void listDeviceChoices() {
	printf("Your choices: \n");
    for(int i=0;i<iSerialDeviceCtr;i++) {
    	printf("    %s\n",serialDeviceNames[i]);
    }
    printf("\n");
}

RETURNCODE getDevicesAndHandleErrors(char* desiredDevice) {
    if(getSerialDevices(desiredDevice) < OK) {
    	perror("Problem getting serial devices!  Exiting.");
    	return -1;
    }

    if(iSerialDeviceCtr != 1)
      printf("\nFound %d valid serial device(s).\n", iSerialDeviceCtr);
    
    if(iSerialDeviceCtr < 1) {
    	printf("Unable to continue.  No usable devices.\n");
    	return -2;
    }

    if(iSerialDeviceCtr == 1) {
    	iActiveDevice = 0;
    	//printf("Defaulting to serial device: %s\n", serialDeviceNames[iActiveDevice]);
    }
    else {
		if(strlen(desiredDevice) == 0) {
			printf("You must specify a desired device name, when there is more than one serial device.\n");
			listDeviceChoices();
			return -3;
		}

		if(iActiveDevice == -1) {
			printf("You attempted to specify a desired device, but it was not found.  Try again.\n");
			listDeviceChoices();
			return -4;
		}

		printf("Unexpected error.\n");
		return -5;
    }

    return 0;
}

void sleep_ms(int milliseconds) {
	struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}


char* base64Encode( char* in, int iLen,  char* out) {
    size_t sz;
    typedef unsigned long UL;
    unsigned char c[4];
    UL u, len;
    const char *alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "abcdefghijklmnopqrstuvwxyz"
                        "0123456789+/";
 

    FILE* fpIn = fmemopen(in, iLen, "rb");
    if(fpIn == NULL) {
        printf("  base64 encoding failure, fmemopen\n");
        return 0;
    }

    FILE* fpOut = open_memstream (&out, &sz);
    if(fpOut == NULL) {
        printf("  base64 encoding failure, open_memstream\n");
        return 0;
    }

    do {
        c[1] = c[2] = 0;
 
        if (!(len = fread(&c, 1, 3, fpIn))) break;

        u = (UL)c[0]<<16 | (UL)c[1]<<8 | (UL)c[2];

        fputc(alpha[u>>18], fpOut);
        fputc(alpha[u>>12 & 63], fpOut);
        fputc(len < 2 ? '=' : alpha[u>>6 & 63], fpOut);
        fputc(len < 3 ? '=' : alpha[u & 63], fpOut);
    } while (len == 3);

    fputc(0,fpOut);
    fflush(fpOut);

    fclose(fpIn);
    fclose(fpOut);

    szBuf = sz;

    return out;
}



int
set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                perror ("error from tcgetattr");
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= CRTSCTS;
        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        //tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                perror ("error from tcsetattr");
                return -1;
        }
        return 0;
}

void
set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                perror ("error from tggetattr");
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                perror ("error setting term attributes");
}


void getResponse(char* requestedCmd, bool bWatchForKeyword) {
  char buf[1024];
  int iOffset = 0;
  int iBytesRead = 0;

  // keyword search string
  strcpy(buf,">>");  
  trim(requestedCmd);
  strcat(buf,requestedCmd);
  strcat(buf,"<<");

  bool bFinished = false;
  memset(read_buffer,0x0,sizeof(read_buffer));
  int retryCtr = 0;

  while(bFinished == false && retryCtr < 100) {
    //printf("bFinished: %d, retryCtr: %d, iOffset before: %d, ",bFinished, retryCtr, iOffset);

    if(iOffset >= sizeof(read_buffer)) {
      perror("read past end of buffer.  This shouldn't happen.");
      bFinished = true;
    }
    else {
      iBytesRead = read(tty_fd, read_buffer+iOffset, sizeof(read_buffer)-iOffset);
     // printf("iBytesRead: %d, ", iBytesRead);
      iOffset += iBytesRead;
      //printf("iOffset after: %d\n", iOffset);
      
      if(bWatchForKeyword) {
        char* cc= stristr(read_buffer, buf);
        if(cc != 0x0) {
          int iStartPoint = cc - read_buffer + (strlen(requestedCmd)+4);
          int j = 0;
          for(int i=iStartPoint;*(read_buffer+i) != 0x0;i++) {
            write_buffer[j] = *(read_buffer+i);
            write_buffer[j+1] = 0x0;
            j++;
          }
          memcpy(read_buffer, write_buffer, sizeof(read_buffer));
          iOffset = strlen(read_buffer);
        }
      }

      int c = strlen(read_buffer);
      if(c > 1) {
        char ch1 = *(read_buffer+c-1);
        char ch2 = *(read_buffer+c-2);

        // we are done reading when we get two carriage returns together at the end of the buffer
        if(ch1 == 0x0a || ch1 == 0x0d) {
          if(ch2 == 0x0a || ch2 == 0x0d) {
            //printf("bFinished=true\n");
            //printf("$$%s$$\n",read_buffer);
            bFinished = true;
          }
        }
      }
    }
    if(!bFinished) {
      sleep_ms(100);
      retryCtr++;
    }
  }

  if(retryCtr >= 100)
    perror("Unable to read response from TRS80HDD before timeout.");


  trim(read_buffer);  
}


void writeRequest(char* cmd) {
  strcpy(write_buffer,cmd);
  strcat(write_buffer,"\n");
  int bytesWritten = write(tty_fd,write_buffer,strlen(write_buffer)+1);

  if(bytesWritten <= strlen(cmd)+1) {
    perror("Unable to write to TRS80HDD interface.");
    close(tty_fd);
    exit(-1);
  }
}



void sendCommand(char* cmd, bool bWatchForKeyword, bool bDisplay) {
  //printf("sendCommand: %s\n",cmd);
  memset(write_buffer,0x0,sizeof(write_buffer));
  memset(read_buffer,0x0,sizeof(read_buffer));

  writeRequest(cmd);
  getResponse(cmd,bWatchForKeyword);

  if(bDisplay)
     printf("%s\n", read_buffer);
}


int getValidCommands(command* commandStrings) {
  sendCommand((char*)"help",true,false);

  char* tok = strtok(read_buffer, "\r\n");

  int j=0;
  while(tok != NULL) {
     strcpy(commandStrings[j].cmdString, tok); 
     trim(commandStrings[j].cmdString);    
     tok = strtok(NULL, "\r\n");
     j++;
  }
  return j;
}


void failIfNot(char* resp) {
  trim(read_buffer);
  String_Lower(read_buffer);
  if(stricmp(read_buffer,resp) != 0) {
     close(tty_fd);
     printf("Expected '%s', received '%s'", resp, read_buffer);
     perror("Did not receive proper response to command.\n");
     exit(-97);
  }
}


void handleUpload(char* sUploadString) {
  FILE* fp;
  char* upload = strtok(sUploadString, " ");
  char* localFileName = strtok(NULL, " ");
  char buf1[512];
  char buf2[512];
  char buf3[1024];

  if(localFileName == NULL) {
    close(tty_fd);
    perror("You did not provide a file name to upload.\n");
    exit(-99);
  }

  trim(localFileName);

  fp = fopen(localFileName, "r");

  if(fp == NULL) {
    close(tty_fd);
    perror("Unable to open file you specified to read it.\n");
    exit(-98);
  }

  sendCommand(sUploadString,true,false);
  failIfNot("ready");

  int iBytesRead = -1;

  long lFileChkSum = 0;
  while(iBytesRead != 0) {
     int chkSum = 0;
     memset(buf1,0x0,sizeof(buf1));
     memset(buf2,0x0,sizeof(buf2));
     iBytesRead = fread(buf1, 1, 500, fp);
     if(iBytesRead > 0) {
         for(int i=0;i<iBytesRead;i++) {
            chkSum += *(buf1+i);
            lFileChkSum += *(buf1+i);
         }
         base64Encode(buf1, iBytesRead, buf2);
         sprintf(buf3,"chunk %04X %s\n",chkSum,buf2);
         sendCommand(buf3,false,false);
         failIfNot("ready");
     }
     else {
         sprintf(buf3,"done %08X\n",lFileChkSum);
         sendCommand(buf3,false,false);
         failIfNot("ok");
     }
  }
  close(fp);
}



int main(int argc,char** argv)
{
	char desiredDevice[256];
  char desiredCommand[256];
  command understoodCommands[256];
  char buf[256];
  int iunderstoodCommandCount;

	RETURNCODE r;
  setbuf(stdout, NULL);

	memset(desiredDevice,0x0,sizeof(desiredDevice));

  for(int i=1;i<argc;i++) {
     strcpy(buf, argv[i]);
     trim(buf);
     String_Lower(buf);
     if((stricmp(buf,"-tty",4) == 0) && (*(desiredDevice) == 0x0) ){
         i++;
         strcpy(desiredDevice, argv[i]);
         printf("desired tty device: %s\n", desiredDevice);
     }
     desiredCommand[0]=0x0;
     if(stricmp(buf,"-cmd",4) == 0)
         i++;

     if((stricmp(buf,"-cmd",4) == 0) || (*(desiredDevice) == 0x0) ){
        while(i<argc) {
          strcat(desiredCommand, argv[i]);
          strcat(desiredCommand, " ");
          i++;
        }
        trim(desiredCommand);
        *(buf)=0x0;
     }
  }

  r = getDevicesAndHandleErrors(desiredDevice);
    
  if(r != OK)
    exit(r);

  strcpy(buf,"/dev/");
  strcat(buf,serialDeviceNames[iActiveDevice]);

  tty_fd = open(buf, O_RDWR | O_NOCTTY | O_NDELAY);

  if(tty_fd < 0) {
    	printf("Unable to open device: %s, rc=%d\n", buf, tty_fd);
      exit(-5);
  }

  set_interface_attribs(tty_fd, B230400, 0);
  set_blocking(tty_fd, 0);


  iunderstoodCommandCount = getValidCommands(understoodCommands);

  //validate requested command
  bool bValidCommand = false;
  for(int i=0;i<iunderstoodCommandCount;i++) {
     if(startsWith(understoodCommands[i].cmdString,desiredCommand)) {
        bValidCommand = true;
     }
  }
  if(!bValidCommand) {
    printf("You requested an invalid command: '%s'.  Valid commands are:\n", desiredCommand);
    for(int i=0;i<iunderstoodCommandCount;i++) {
      printf("  %s\n", understoodCommands[i].cmdString);
    }
    printf("\n");
    close(tty_fd);
    exit(-2);
  }

  
  // at this point, user requested a valid command, so lets send it and see what the response is
  if(startsWith(desiredCommand,"upload ")) {  // upload is special since it involves transactional processing
      handleUpload(desiredCommand);
  }
  else  // all other commands are request/reply
      sendCommand(desiredCommand,true,true);

  close(tty_fd);
  return 0;
}