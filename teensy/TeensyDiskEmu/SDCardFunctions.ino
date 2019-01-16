String disk1FileName = "NEWDOS_80sssd_jv1.DSK";
File disk1File;
SdFatSdioEX sdEx;



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
