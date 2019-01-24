/* printf() to serial output */
void p(char *fmt, ... ){
    char buf[128];        

// comment out the return statement for debugging purposes
//return;
    
    va_list args;
    va_start (args, fmt );
    vsnprintf(buf, 128, fmt, args);
    va_end (args);
    Serial.print(buf);
    Serial.flush();
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
