SdFatSdioEX sdEx;

extern drivesettings Drives[4];



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
          p((char*)"%s\n",workStrm.c_str());
          workStrm = "";
      }
      file.close();
  }

}



void openDiskFileByName(String sFileName, int iDriveNum) {
  File file;
  String workStrm;
  boolean bLoaded = false;
  
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
             Drives[iDriveNum].diskFile = sdEx.open(workStrm.c_str(), FILE_READ);
             if(!Drives[iDriveNum].diskFile) {
                p((char*)"ERROR:  Unable to open file\n");
                L1_RED();
                L2_RED();
                file.close();
                return;
             }
             else {
              p((char*)"\n%d bytes in file.\n",Drives[iDriveNum].diskFile.available());
              bLoaded = true;
             }
          }
          workStrm = "";
      }
      file.close();
  }

  if(bLoaded == true)
      Drives[iDriveNum].sDiskFileName = sFileName;
  else
      p((char*)"ERROR: Unable to load file %s.\n",sFileName);

  return;
}
