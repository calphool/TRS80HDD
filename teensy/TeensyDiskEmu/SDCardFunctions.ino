File diskFile[5];
String sDiskFileName[5];
SdFatSdioEX sdEx;




void catalog() {
  File file;
  String workStrm;
  StringStream stream(workStrm); // set up string stream to capture SD card directory

    
    
  if (!sdEx.begin()) {             // couldn't establish SD card connection
        p((char*)"ERROR:  Unable to open SdFatSdioEX object.\n");
        sdEx.initErrorHalt("SdFatSdioEX begin() failed");
        L2_RED();
        L1_RED();
        return ;
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
          p("%s\n",workStrm.c_str());
          workStrm = "";
      }
      file.close();
  }

}



void openDiskFileByName(String sFileName, int iDriveNum) {
  File file;
  String workStrm;
  
  StringStream stream(workStrm); // set up string stream to capture SD card directory

    if (!sdEx.begin()) {             // couldn't establish SD card connection
        p((char*)"ERROR:  Unable to open SdFatSdioEX object.\n");
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
          if(workStrm == sFileName) {
             p((char*)"Opening disk %d file: ",iDriveNum);
             p((char*)workStrm.c_str());
             diskFile[iDriveNum] = sdEx.open(workStrm.c_str(), FILE_READ);
             if(!diskFile[iDriveNum]) {
                p((char*)"\nERROR:  Unable to open file\n");
                L1_RED();
                L2_RED();
                file.close();
                return;
             }
             else {
              p((char*)"\n%d bytes in file.\n",diskFile[iDriveNum].available());
             }
          }
          workStrm = "";
      }
      file.close();
  }

  sDiskFileName[iDriveNum] = sFileName;

  return;
}
