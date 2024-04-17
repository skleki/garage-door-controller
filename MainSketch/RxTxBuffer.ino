//#include "HWconfigAndConstants.h"

#if (defined (RS232_COMMS) || defined (RF24_COMMS) || defined(MY_WIFI_ESP8266) || defined(MYETHERNET))

// *****************************************************************************************
// *****************              Rx and Tx buffer control           ***********************
// *****************************************************************************************

#include <Arduino.h>

int (*SendMessageFuntion)(byte *data, int len);

volatile byte RxBuf[RX_BUF_SIZE];
volatile int RxBufInP;
volatile int RxBufOutP;
volatile int RxBufFree;
volatile byte TxBuf[TX_BUF_SIZE];
volatile int TxBufInP;
volatile int TxBufOutP;
volatile int TxBufFree;
byte ReceivedParsedFullMessage[MaxRxMsgLen];
byte TxFullMessage[MaxTxMsgLen];
byte SendMessageData[MaxTxMsgLen];
byte SendChungBuff[MAX_TX_CHUNK_SIZE];

volatile byte RxParseFlag;



// Functions definitions
inline void SetSendMessageFunction(int (*funct)(byte *data, int len));
inline byte RxBufParser(void);


inline void RxTxBuffer_Initialize(void)
{
  SetSendMessageFunction(DummySendMessageFunction);

  RxBufFree = RX_BUF_SIZE;
  RxBufInP = 0;
  RxBufOutP = 0;
  TxBufFree = TX_BUF_SIZE;
  TxBufInP = 0;
  TxBufOutP = 0;
  RxParseFlag = 0;
}

void RxTxBuffer_MailLoop(void)
{
  while (RxParseFlag == 1)
  {
    RxParseFlag = RxBufParser();
  }

  if (TxBufFree < TX_BUF_SIZE)
  {
      int dataToSendLen = MAX_TX_CHUNK_SIZE;
      if (dataToSendLen > (TX_BUF_SIZE - TxBufFree))
      {
          dataToSendLen = TX_BUF_SIZE - TxBufFree;
      }
      for (int i = 0; i < dataToSendLen; i++)
      {
          int outP = TxBufOutP + i;
          if (outP >= TX_BUF_SIZE)
              outP = outP - TX_BUF_SIZE;
          SendChungBuff[i] = TxBuf[outP];
      }

      int sentBytes = SendMessageFuntion(SendChungBuff, dataToSendLen);
      if (sentBytes == dataToSendLen)
      {
          for (int i = 0; i < dataToSendLen; i++)
          {
              TxBufOutP++;
              if (TxBufOutP == TX_BUF_SIZE)
                  TxBufOutP = 0;
              TxBufFree++;
          }
//          Serialprint("RxTxBuffer_MailLoop SendMessageFuntion success, TxBufFree: ");
//          Serialprintln(TxBufFree);
      }
      else
      {
        Serialprint("SendMessageFuntion not sent requested amount of bytes, sent only: ");
        Serialprintln(sentBytes);
      }
  }  
}

inline void SetSendMessageFunction(int (*funct)(byte *data, int len))
{
  SendMessageFuntion = funct;
}

inline bool InsertReceivedData(byte *data, int len)
{
  bool retValue = false;
  if (RxBufFree >= len)
  {
    Serialprint("InsertReceivedData, RxBufferFree: ");
    Serialprint(RxBufFree);
    Serialprint(", inserting message len: ");
    Serialprintln(len);
    for (int i = 0; i < len; i++)
    {
        RxBuf[RxBufInP++] = data[i];
        if (RxBufInP == RX_BUF_SIZE)
          RxBufInP = 0;
        RxBufFree--;
        RxParseFlag = 1;
    }
    retValue = true;
  }
  else
  {
    Serialprint("InsertReceivedData, RxBufferFree not enough for message size: ");
    Serialprintln(len);
  }
  return retValue;
}

inline bool SendData(byte *data, int len)
{
  bool retValue = false;
  TxFullMessage[0] = 0xF0;
  TxFullMessage[1] = (len + 5) & 0xFF;
  TxFullMessage[2] = ((len + 5) >> 8) & 0xFF;
  for (int i = 0; i < len; i++)
    TxFullMessage[3 + i] = data[i];
  byte cs = 0;
  for (int i = 1; i < (len + 3); i++)
    cs = cs ^ TxFullMessage[i];
  TxFullMessage[len + 3] = cs;
  TxFullMessage[len + 4] = 0xFE;
  //  return true;

  if (TxBufFree >= (len + 5))
  {
    for (int i = 0; i < (len + 5); i++)
    {
        TxBuf[TxBufInP++] = TxFullMessage[i];
        if (TxBufInP >= TX_BUF_SIZE)
          TxBufInP = 0;
        TxBufFree--;
    }
    retValue = true;
  }
  else
  {
      Serialprint("TxBuffer free space not long enough to send bytes: ");
      Serialprintln(len + 5);
  }
  return retValue;

/* Message format
    int i;
    quint8 cs;
    QList<quint8> fullMessage;
    fullMessage.append(0xF0);
    fullMessage.append((quint8)((dataBytes.count() + 5) & 0xFF));
    fullMessage.append((quint8)((dataBytes.count() + 5) >> 8));
    for (i = 0; i < dataBytes.count(); i++)
        fullMessage.append(dataBytes.at(i));
    cs = 0;
    for (i = 1; i < (dataBytes.count()  + 3); i++)
        cs = cs ^ fullMessage.at(i);
    fullMessage.append(cs);
    fullMessage.append(0xFE);
    this->LogMessage(QString("Created message: ") + Utilities::ConvertBytesToHexString(fullMessage));
    return fullMessage;
 */
}

inline bool SendDataIfNotPresent(byte *data, int len)
{
  TxFullMessage[0] = 0xF0;
  TxFullMessage[1] = (len + 5) & 0xFF;
  TxFullMessage[2] = ((len + 5) >> 8) & 0xFF;
  for (int i = 0; i < len; i++)
    TxFullMessage[3 + i] = data[i];
  byte cs = 0;
  for (int i = 1; i < (len + 3); i++)
    cs = cs ^ TxFullMessage[i];
  TxFullMessage[len + 3] = cs;
  TxFullMessage[len + 4] = 0xFE;

  int pointer;
  bool messagePresent = false;
  for (int i = 0; i < (TX_BUF_SIZE - TxBufFree);i++)
  {
    pointer = TxBufOutP + i;
    pointer = pointer % TX_BUF_SIZE;
    if (TxBuf[pointer] == TxFullMessage[0])
    {
      if (i + (len + 5) > (TX_BUF_SIZE - TxBufFree)) // nema dovoljno bajtova za celu poruku u baferu
      {
        break;
      }
      
      messagePresent = true;
      for (int j = 0; (j < (len + 5)) && messagePresent; j++)
      {
        pointer = TxBufOutP + i + j;
        pointer = pointer % TX_BUF_SIZE;
        if (TxBuf[pointer] != TxFullMessage[j])
        {
          messagePresent = false;
        }
      }
      if (messagePresent)
      {
//        Serialprint("TxBuf11111");
        return true;
      }
    }
  }
  
  if (TxBufFree >= (len + 5))
  {
    for (int i = 0; i < (len + 5); i++)
    {
        TxBuf[TxBufInP++] = TxFullMessage[i];
        if (TxBufInP == TX_BUF_SIZE)
          TxBufInP = 0;
        TxBufFree--;
    }
//    Serialprint("TxBuf11111");
    return true;
  }
//  Serialprint("TxBuf0000");
  return false;
}

inline int DummySendMessageFunction(byte data[], int len)
{
  return 0;
}



inline unsigned char MsgValid(unsigned char* message, int MsgLen)
{
  unsigned char cs;
  int i;
  cs = 0;
  for (i = 1; i <= (MsgLen - 3); i++)
    cs  = cs ^ message[i];
  if (cs == message[MsgLen - 2])
    return 1;
  else
    return 0;
}

inline byte RxBufParser(void)
{
  int MsgLen;
  int MaxLen, i, tmpind;
  byte UART_Parse_RxFret;
  int CurrentMsgLen;
  int CurrentMaxRxMsgLen;

//  noInterrupts();
  if ((RX_BUF_SIZE - RxBufFree) < 6)   // Minimalna poruka je velicine 6 bajtova
  {
//    interrupts();
    return 0; // jos fale bajtovi
  }

  CurrentMsgLen = 0;
  CurrentMaxRxMsgLen = 0;
  while ((RxBuf[RxBufOutP] != 0xF0) && (RxBufFree < RX_BUF_SIZE))
  {
    RxBufOutP++;
    RxBufFree++;
    if (RxBufOutP == RX_BUF_SIZE)
      RxBufOutP = 0;
  }

  // *********************************************************************
  if ((RX_BUF_SIZE - RxBufFree) >= 6)   // Minimalna poruka je velicine 6 bajtova
  {
    if (RxBufOutP < RxBufInP)  // Slucaj kada bafer nije poceo da se puni iz pocetka
    { 
      MaxLen = RX_BUF_SIZE - RxBufFree;
      MsgLen = RxBuf[RxBufOutP + 1] + (RxBuf[RxBufOutP + 2] * 256);
      CurrentMsgLen = MsgLen;
      CurrentMaxRxMsgLen = MaxLen;
      if ((MsgLen > MaxRxMsgLen) || (MsgLen == 0))
      {
        Serialprint("RxBufParser, msgLen to big for parser: ");
        Serialprint(MsgLen);
        RxBuf[RxBufOutP] = 0; // U narednom pozivu PARSERA ce se ovaj pocetak prebrisati i nema frke
      }
      else if (MsgLen <= MaxLen)
      {
        if (RxBuf[RxBufOutP + MsgLen - 1] == 0xFE) // Detektovan validan kraj poruke
        {
          for(i = 0; i < MsgLen; i++)
            ReceivedParsedFullMessage[i] = RxBuf[RxBufOutP + i];
          if (MsgValid((unsigned char*)ReceivedParsedFullMessage, MsgLen))
          {
            ServerComms_MessageAnalyser(ReceivedParsedFullMessage+3, MsgLen - 5);
          }
          RxBufOutP = RxBufOutP + MsgLen;
          RxBufFree = RxBufFree + MsgLen;
        }
        else
          RxBuf[RxBufOutP] = 0; // U narednom pozivu PARSERA ce se ovaj pocetak prebrisati i nema frke
      }
    } // Za slucaj kada bafer nije poceo da upoisuje od pocetka
    else  // Slucaj kada je bafer poceo da se puni iz pocetka
          // RxBufOutP > RxBufInP
    {
      MaxLen = RX_BUF_SIZE - RxBufFree;
      tmpind = RxBufOutP + 1;
      tmpind = tmpind % RX_BUF_SIZE;
      byte lenLowerByte = RxBuf[tmpind];
      tmpind = RxBufOutP + 2;
      tmpind = tmpind % RX_BUF_SIZE;
      byte lenHigherByte = RxBuf[tmpind];      
      MsgLen = lenLowerByte + (lenHigherByte * 256);
      CurrentMsgLen = MsgLen;
      CurrentMaxRxMsgLen = MaxLen;
      
      if ((MsgLen > MaxRxMsgLen) || (MsgLen == 0))
      {
        Serialprint("RxBufParser, msgLen to big for parser: ");
        Serialprint(MsgLen);
        RxBuf[RxBufOutP] = 0;
      }
      else if (MsgLen <= MaxLen)
      {
        tmpind = RxBufOutP + MsgLen - 1;
        tmpind = tmpind % RX_BUF_SIZE;
        if (RxBuf[tmpind] == 0xFE)
        {
          for(i = 0; i < MsgLen; i++)
          {
            tmpind = RxBufOutP + i;
            tmpind = tmpind % RX_BUF_SIZE;
            ReceivedParsedFullMessage[i] = RxBuf[tmpind];
          }
          if (MsgValid((unsigned char*)ReceivedParsedFullMessage, MsgLen))
          {
            ServerComms_MessageAnalyser(ReceivedParsedFullMessage+3, MsgLen - 5);
          }
          RxBufOutP = RxBufOutP + MsgLen;
          RxBufOutP = RxBufOutP % RX_BUF_SIZE;
          RxBufFree = RxBufFree + MsgLen;
        }
        else
          RxBuf[RxBufOutP] = 0;
      }
    }
  }

  if (((RX_BUF_SIZE - RxBufFree) < 6) || (CurrentMsgLen > CurrentMaxRxMsgLen))
    UART_Parse_RxFret = 0;
  else
    UART_Parse_RxFret = 1;
  // *********************************************************************
//  interrupts();
  return UART_Parse_RxFret;
}

#endif
