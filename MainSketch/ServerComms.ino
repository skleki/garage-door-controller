//#include "HWconfigAndConstants.h"

#if (defined (RS232_COMMS) || defined (RF24_COMMS) || defined(MY_WIFI_ESP8266) || defined(MYETHERNET))

// *****************************************************************************************
// ******************              Server communication control           ******************
// *****************************************************************************************

#define ServerComms_NoCommsResetInterval 900

int ServerComms_NoCommsResetCounter;
int ServerComms_PingTimerCounter;
int ServerComms_CommsLost;
int ServerComms_PingFlag;
int ServerComms_Status;
int ServerComms_LastMsgReceivedCounter;
int ServerCommsLostInterval;

inline void ServerComms_CommsLostEvent();
inline void ServerComms_CommsReestablishedEvent();



inline void ServerComms_Initialize(void)
{
  ServerComms_PingTimerCounter = 0;
  ServerComms_CommsLost = 0;
  ServerComms_PingFlag = 1;
  ServerComms_Status = 0;
  ServerComms_LastMsgReceivedCounter = 0;
  ServerComms_NoCommsResetCounter = 0;
  ServerCommsLostInterval = EEPROMDataRegular.PingTimeoutSeconds * EEPROMDataRegular.RetransmitCount;
  Serialprint("ServerComms Initialised!");
}

inline void ServerComms_MailLoop(void)
{
  if (ServerComms_PingFlag == 1)
  {
    ServerComms_PingFlag = 0;
    PingServer();
//    Serialprintln("PingServer();");
  }
}

inline void ServerComms_100msHandler(void)
{
}

inline void ServerComms_1000msHandler(void)
{
//  Serialprintln("ServerComms_1000msHandler");
//  Serialprint("ServerComms_PingTimerCounter: ");
//  Serialprintln(ServerComms_PingTimerCounter);
//  Serialprint(", EEPROMDataRegular.PingTimeoutSeconds");
//  Serialprintln(EEPROMDataRegular.PingTimeoutSeconds);
  ServerComms_PingTimerCounter++;
  if (ServerComms_PingTimerCounter >= EEPROMDataRegular.PingTimeoutSeconds)
  {
    ServerComms_PingTimerCounter = 0;
    ServerComms_PingFlag = 1;
//    Serialprintln("ServerComms_PingFlag = 1;");
  }
  ServerComms_LastMsgReceivedCounter++;
  if (ServerComms_LastMsgReceivedCounter > ServerCommsLostInterval)
  {
    #ifdef RF24_COMMS
      MyRF24_Reset();
    #endif
    if (ServerComms_Status != -1)
      ServerComms_CommsLostEvent();
    ServerComms_Status = -1;
    ServerComms_LastMsgReceivedCounter = 0;
  }
  ServerComms_NoCommsResetCounter++;
  if (ServerComms_NoCommsResetCounter > ServerComms_NoCommsResetInterval)
  {
    ResetUno();
  }
}

inline boolean ServerComms_IsCommsLost()
{
  return (ServerComms_Status == -1);
}

inline void ServerComms_CommsLostEvent()
{
  // ovde dodaj sta se izvrsava na gubitak komunikacije
  #if (defined(HM_LEDS) || defined(UNO_BOARD_LEDS))
    Signal_LED_CommsLost();
  #endif
}

inline void ServerComms_CommsReestablishedEvent()
{
  // ovde dodaj sta se izvrsava na povratak komunikacije
  #if (defined(HM_LEDS) || defined(UNO_BOARD_LEDS))
    Signal_LED_CommsActive();
  #endif
}

inline void ServerComms_MessageAnalyser(byte *data, int len)
{ // ovde se samo prosledjuju data, bez zaglavlja poruke
//  byte tmpVal;
  int dataInserted;
//  Serialwrite(data, len);

  if (len > 0)
  {
      switch(data[0])
      {
        case CommsRPi2UnoEnum_PingToUno:
          SendMessageData[0] = CommsUno2RPi_ResponseToRPiPing; // Response to Server Ping
          //SendDataIfNotPresent(SendMessageData, 1);
          SendData(SendMessageData, 1);
          if (ServerComms_Status != 1)
            ServerComms_CommsReestablishedEvent();
          ServerComms_Status = 1;
          ServerComms_LastMsgReceivedCounter = 0;
          ServerComms_NoCommsResetCounter = 0;
          break;
        case CommsRPi2UnoEnum_ResponseToUnoPing:  // Ping response
          if (ServerComms_Status != 1)
            ServerComms_CommsReestablishedEvent();
          ServerComms_Status = 1;
          ServerComms_LastMsgReceivedCounter = 0;
          ServerComms_NoCommsResetCounter = 0;
          break;
        case CommsRPi2UnoEnum_GetEepromData:
          SendMessageData[0] = CommsUno2RPi_GetEepromDataResponse;
          GetEEDataRegular(&(SendMessageData[1]), &dataInserted);
          SendData(SendMessageData, 1 + dataInserted);
          break;
        case CommsRPi2UnoEnum_SetEepromData:
          SetEEDataRegular(&(data[1]), len - 1);
          SendMessageData[0] = CommsUno2RPi_SetEepromDataResponse;
          GetEEDataRegular(&(SendMessageData[1]), &dataInserted);
          SendData(SendMessageData, 1 + dataInserted);
          break;
        case CommsRPi2UnoEnum_ResetEepromData:
          EEData_Reset();
          SaveEEPROMData();
          SendMessageData[0] = CommsUno2RPi_ResetEepromDataResponse; // Response to Server Ping
          SendDataIfNotPresent(SendMessageData, 1);
          break;

        case CommsRPi2UnoEnum_GetEepromWiFiData:
          #if defined(ESP8266) || defined(ESP32)
            SendMessageData[0] = CommsUno2RPi_GetEepromWiFiDataResponse;
            GetEEDataWiFi(&(SendMessageData[1]), &dataInserted);
            SendData(SendMessageData, 1 + dataInserted);
          #endif
          break;
        case CommsRPi2UnoEnum_SetEepromWiFiData:
          #if defined(ESP8266) || defined(ESP32)
            SetEEDataWiFi(&(data[1]), len - 1);
            SendMessageData[0] = CommsUno2RPi_SetEepromWiFiDataResponse;
            GetEEDataWiFi(&(SendMessageData[1]), &dataInserted);
            SendData(SendMessageData, 1 + dataInserted);
          #endif
          break;
        case CommsRPi2UnoEnum_ResetUno:
          ResetUno();
          break;

        case CommsRPi2UnoEnum_GetCommsSignalStrength:
          GetCommsSignalStrength_AnalyseMessage(data, len);
          break;
	  
        default:
          Serialprint("Received UNKNOWN message: ");
          for (int i = 0; i < len; i++)
          {
            Serialprint2A(data[i], 16);
            Serialprint(" ");
          }
          Serialprintln("");
          for (int i = 0; i < len; i++)
            SendMessageData[i] = data[i];
          SendData(SendMessageData, len);
          break;
      }
  }
}

inline bool GetCommsSignalStrength_AnalyseMessage(byte *data, int len)
{
  long sigStrength = SignalStrength();
  SendMessageData[0] = CommsUno2RPi_CommsSignalStrength;
  SendMessageData[1] = ((unsigned long)sigStrength >> 24) & 0xFF;
  SendMessageData[2] = ((unsigned long)sigStrength >> 16) & 0xFF;
  SendMessageData[3] = ((unsigned long)sigStrength >> 8) & 0xFF;
  SendMessageData[4] = ((unsigned long)sigStrength >> 0) & 0xFF;
  return SendDataIfNotPresent(SendMessageData, 5);
}

inline bool PingServer()
{
  SendMessageData[0] = CommsUno2RPi_PingToRPi; // Ping
  return SendDataIfNotPresent(SendMessageData, 1);
}

inline bool UnoCameOutOfReset(unsigned char code)
{
  SendMessageData[0] = CommsUno2RPi_UnoCameOutOfReset; // After reset message
  SendMessageData[1] = code;
  return SendDataIfNotPresent(SendMessageData, 2);
}

#endif
