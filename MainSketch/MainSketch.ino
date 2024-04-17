// *****************************************************************************************
// ************       Use #define directives to enable hardware support      ***************

#include "ESP32Time.h"

// #define ESP8266
// #define ESP32   // this is already defined when you select ESP32 board
#define ESP32_WROOM_32_30PIN  // "ESP32 Dev Module"
// #define ESP32_HelTex_WiFi_Kit_36pin

#define GSM_THERMOSTAT
#define OLED_DISPLAY

#define GSM_MODEM
#define RS232_COMMS
#define ENABLE_WDT
#define UNO_BOARD_LEDS

// #define RS232_PROTOCOL


#if defined (ESP8266)
  #define SIGNAL_LED_PIN D8
  #define DS18x20_pin1 D6
  #define DS18x20_pin2 D2
#elif defined (ESP32)
  #if defined (ESP32_WROOM_32_30PIN)
    #define SIGNAL_LED_PIN 2
    #define RS232_PROTOCOL_PIN 23
    #define DS18x20_pin1 22
    #define DS18x20_pin2 23
  #elif defined (ESP32_HelTex_WiFi_Kit_36pin)   // I am using this with the first GSM Thermostat project
    #define SIGNAL_LED_PIN 25
    #define RS232_PROTOCOL_PIN 36
    #define DS18x20_pin1 32
    #define DS18x20_pin2 33
    #define EWDT_RESET_PIN 26
    #define RELAY_ACTIVE_LOW_PIN 34
    #define RELAY_ACTIVE_HIGH_PIN 35
    #define SIM800L_TX_PIN 14
    #define SIM800L_RX_PIN 27
    #define SIM800L_RESET_PIN 12
  #endif
#endif

// *****************************************************************************************
// **************                 Serial logging control code                ***************
// *****************************************************************************************
#define Serialbegin2A(baud,config) Serial.begin(baud,config)
#define Serialbegin(baud) Serial.begin(baud)

bool isRS232;
#define Serialprint2A(x,y) !isRS232 ? Serial.print(x,y) : 0
#define Serialprint(x) !isRS232 ? Serial.print(x) : 0
#define Serialprintln2A(x,y) !isRS232 ? Serial.println(x,y) : 0
#define Serialprintln(x) !isRS232 ? Serial.println(x) : 0
#define ForceSerialprint2A(x,y) !isRS232 ? Serial.print(x,y) : 0
#define ForceSerialprint(x) !isRS232 ? Serial.print(x) : 0
#define ForceSerialprintln2A(x,y) !isRS232 ? Serial.println(x,y) : 0
#define ForceSerialprintln(x) !isRS232 ? Serial.println(x) : 0

// *****************************************************************************************
// *****************************************************************************************


// *****************************************************************************************
// ************               Globaly defined Comms message codes            ***************
#define CommsRPi2UnoEnum_PingToUno 0
#define CommsRPi2UnoEnum_ResponseToUnoPing 1
#define CommsRPi2UnoEnum_GetEepromData 2
#define CommsRPi2UnoEnum_SetEepromData 3
#define CommsRPi2UnoEnum_ResetEepromData 4
#define CommsRPi2UnoEnum_GetEepromWiFiData 5
#define CommsRPi2UnoEnum_SetEepromWiFiData 6
#define CommsRPi2UnoEnum_ResetUno 7
#define CommsRPi2UnoEnum_GetCommsSignalStrength 0xD2

#define CommsUno2RPi_ResponseToRPiPing 0
#define CommsUno2RPi_PingToRPi 1
#define CommsUno2RPi_GetEepromDataResponse 2
#define CommsUno2RPi_SetEepromDataResponse 3
#define CommsUno2RPi_ResetEepromDataResponse 4
#define CommsUno2RPi_GetEepromWiFiDataResponse 5
#define CommsUno2RPi_SetEepromWiFiDataResponse 6
#define CommsUno2RPi_UnoCameOutOfReset 7
#define CommsUno2RPi_CommsSignalStrength 0xD2

#define RX_BUF_SIZE 600
#define TX_BUF_SIZE 600
#define MaxRxMsgLen 250
#define MaxTxMsgLen 250 
#define MAX_TX_CHUNK_SIZE (500 + 25)    // za Fast UDP Comms, 25 je za header



// *****************************************************************************************
// **************               Main sketch, Setup() and Loop()              ***************
// *****************************************************************************************

extern "C" {
#if defined (ESP8266)
  #include "user_interface.h"
#endif
} 



void SetSendMessageFunction(int (*funct)(byte *data, int len));
extern void Signal_LEDs_Initialise();
extern void RxTxBuffer_Initialize();
extern void RS232Comms_Initialize();
extern void ServerComms_Initialize();
extern void Timer1_Initialize();
extern bool UnoCameOutOfReset(unsigned char code);
extern void Timer1MailLoop(void);
extern void RS232Comms_MailLoop(void);
extern void ServerComms_MailLoop(void);
extern void RxTxBuffer_MailLoop(void);
extern void Signal_LEDs_10msHandler(void);
extern inline void DS18x20_Init(bool loggingEnabled);
extern inline void DS18x20_Timer1s_Handler();
extern inline bool EEData_IsCRCOK();
extern void HeaterControl_Initialise();
extern inline void HeaterControl_Loop(void);
extern inline void HeaterControl_1msHandler(void);
extern inline void HeaterControl_100msHandler(void);
extern inline void HeaterControl_1000msHandler(void);

extern ESP32Time rtc;


// *****************************************************************************************
// ******************          S E T U P        ********************************************
void setup() 
{
  LoadEEPROMData();

  pinMode(RS232_PROTOCOL_PIN, INPUT_PULLUP);
  delay(50);
  isRS232 = digitalRead(RS232_PROTOCOL_PIN) == LOW;
//  isRS232 = true;
//  isRS232 = false;
  
  Signal_LEDs_Initialise();
  
  if (isRS232)
  {
    RxTxBuffer_Initialize();
    RS232Comms_Initialize();
    ServerComms_Initialize();
  }
  else
  {
	  Serialbegin(115200);
	  ForceSerialprintln("Starting Arduino !!!");
    Serialprint("EEData data verification: ");
	  Serialprintln(EEData_IsCRCOK());
	  Serialprintln("");
	  Serialprintln("EEData data on Initialisation: ");
	  PrintEEPROMData();
  }

  MyWiFi_Initialise();
  Timer1_Initialize();

  if (isRS232)
  {
  #if defined (ESP8266)
    const rst_info* resetInfo = system_get_rst_info();
    //consolePort.print(F("system_get_rst_info() reset reason: "));
    //consolePort.println(RST_REASONS[resetInfo->reason]);   
    UnoCameOutOfReset(resetInfo->reason);
  #elif defined (ESP32)
    UnoCameOutOfReset(esp_reset_reason());
  #endif
  }

  DS18x20_Init(false);

  WDT_Initialise();
  GsmLogic_Initialise();
  HeaterControl_Initialise();
  
  Serialprintln("setup() FINISHED.\n");
}
// *****************************************************************************************
// *****************************************************************************************

// *****************************************************************************************
// ****************           M A I N    L O O P         ***********************************
void loop() 
{
	WDT_Reset();
	yield();
	Timer1MailLoop();
	yield();

  if (isRS232)
  {
  	RS232Comms_MailLoop();
  	yield();
  	ServerComms_MailLoop();
  	yield();
  	RxTxBuffer_MailLoop();
  	yield();
  }
  
  MyWiFi_Loop();
  yield();
  GsmLogic_loop();
  yield();
  HeaterControl_Loop();
}
// *****************************************************************************************
// *****************************************************************************************




// *****************************************************************************************
// ****************           T I M E R    H A N D L E R S        **************************
inline void Timer1_1msHandler(void)
{
  HeaterControl_1msHandler();
}

inline void Timer1_10msHandler(void)
{
  Signal_LEDs_10msHandler();
}

inline void Timer1_100msHandler(void)
{
  DS18x20_Timer100ms_Handler();
  GsmLogic_100msHandler();
  HeaterControl_100msHandler();
}

inline void Timer1_1000msHandler(void)
{
  DS18x20_Timer1s_Handler();
  MyWiFi_1sHandler();
  GsmLogic_1sHandler();
  HeaterControl_1000msHandler();
}

inline void ResetUno(void)
{
#if defined (ESP8266)
  wdt_enable(WDTO_15MS); // turn on the WatchDog and don't stroke it.
  for(;;);
#elif defined (ESP32)
  esp_restart();
#endif
}

// *****************************************************************************************
// *****************************************************************************************
