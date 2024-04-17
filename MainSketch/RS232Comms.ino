//#include "HWconfigAndConstants.h"

#ifdef RS232_COMMS

// *****************************************************************************************
// **********************              RS232 Comms control           ***********************
// *****************************************************************************************

void RS232Comms_Initialize(void)
{
  Serial.begin(57600);
  SetSendMessageFunction(RS232Comms_SendMessage);
}

void RS232Comms_MailLoop(void)
{
    byte oneByteData[1];
    if(Serial.available())
    { 
      oneByteData[0] = Serial.peek();             
      bool inserted = InsertReceivedData(oneByteData, 1);
      if (inserted)
      {
        oneByteData[0] = Serial.read();
      }
      else
      {
        return;
      }
       
       while (Serial.available())
       { 
          oneByteData[0] = Serial.peek();             
          bool inserted = InsertReceivedData(oneByteData, 1);
          if (inserted)
          {
            oneByteData[0] = Serial.read();
          }
          else
          {
            return;
          }
       }
    }
}

int RS232Comms_SendMessage(byte *data, int len)
{
  int available;
#if defined(ESP32)
  available = 200;
#elif defined(ESP8266)
  available = Serial.availableForWrite();
#else
  available = Serial.availableForWrite();
#endif

  
  if (available >= len)
  {
    Serial.write(data, len);
    return len;
  }
  return 0;
}

#endif
