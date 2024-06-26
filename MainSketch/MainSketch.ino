// *****************************************************************************************
// ************       Use #define directives to enable hardware support      ***************

// #define ESP8266
// #define ESP32   // this is already defined when you select ESP32 board
#define ESP32_WROOM_32_30PIN  // "ESP32 Dev Module"

#define DOOR_CONTROL

#define GSM_MODEM
#define ENABLE_WDT
#define UNO_BOARD_LEDS
#define ENABLE_EWDT


#define SIGNAL_LED_PIN 2
#define RS232_PROTOCOL_PIN 23
#define DS18x20_pin1 33
#define EWDT_RESET_PIN 22
#define SIM800L_RX_PIN 16
#define SIM800L_TX_PIN 17
#define SIM800L_RESET_PIN 4
#define DOOR_CLOSE_PIN_ACTIVE_LOW 15
#define DOOR_CLOSE_PIN_ACTIVE_HIGH 19
#define DOOR_OPEN_PIN_ACTIVE_LOW 18
#define DOOR_OPEN_PIN_ACTIVE_HIGH 5
#define DOOR_CLOSED_SENSOR_PIN 12


// *****************************************************************************************
// **************                 Serial logging control code                ***************
// *****************************************************************************************
#define Serialbegin2A(baud,config) Serial.begin(baud,config)
#define Serialbegin(baud) Serial.begin(baud)

#define Serialprint2A(x,y) Serial.print(x,y)
#define Serialprint(x) Serial.print(x)
#define Serialprintln2A(x,y) Serial.println(x,y)
#define Serialprintln(x) Serial.println(x)
#define ForceSerialprint2A(x,y) Serial.print(x,y)
#define ForceSerialprint(x) Serial.print(x)
#define ForceSerialprintln2A(x,y) Serial.println(x,y)
#define ForceSerialprintln(x) Serial.println(x)

// *****************************************************************************************
// *****************************************************************************************


// *****************************************************************************************
// **************               Main sketch, Setup() and Loop()              ***************
// *****************************************************************************************

extern "C" {
#if defined (ESP8266)
  #include "user_interface.h"
#endif
} 



extern void Signal_LEDs_Initialise();
extern void Timer1_Initialize();
extern bool UnoCameOutOfReset(unsigned char code);
extern void Timer1MailLoop(void);
extern void EWDT_1msHandler(void);
extern void EWDT_1sHandler(void);
extern void EWDT_Initialise(void);
extern void WDT_Initialise(void);
extern void WDT_Reset(void);
extern void DoorControl_Initialise(void);
extern void DoorControl_Loop(void);
extern void DoorControl_1msHandler(void);
extern void DoorControl_10msHandler(void);
extern void DoorControl_1000msHandler(void);
extern void Signal_LEDs_10msHandler(void);
extern inline void DS18x20_Init(bool loggingEnabled);
extern inline void DS18x20_Timer1s_Handler();
extern void GsmLogic_Initialise(void);
extern void GsmLogic_loop(void);
extern void GsmLogic_100msHandler(void);
extern void GsmLogic_1sHandler(void);


// *****************************************************************************************
// ******************          S E T U P        ********************************************
void setup() 
{
  EWDT_Initialise();
  
  pinMode(RS232_PROTOCOL_PIN, INPUT_PULLUP);
  delay(50);
  
  Signal_LEDs_Initialise();
  
  Serialbegin(115200);
  ForceSerialprintln("Starting Arduino !!!");
  
  Timer1_Initialize();
  
  DS18x20_Init(false);
  
  WDT_Initialise();
  
  DoorControl_Initialise();
  GsmLogic_Initialise();
  
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

  GsmLogic_loop();
  yield();
  DoorControl_Loop();
  yield();
}
// *****************************************************************************************
// *****************************************************************************************




// *****************************************************************************************
// ****************           T I M E R    H A N D L E R S        **************************
inline void Timer1_1msHandler(void)
{
  DoorControl_1msHandler();
}

inline void Timer1_10msHandler(void)
{
  EWDT_1msHandler();
  Signal_LEDs_10msHandler();
  DoorControl_10msHandler();
}

inline void Timer1_100msHandler(void)
{
  DS18x20_Timer100ms_Handler();
  GsmLogic_100msHandler();
}

inline void Timer1_1000msHandler(void)
{
  EWDT_1sHandler();
  DS18x20_Timer1s_Handler();
  GsmLogic_1sHandler();
  DoorControl_1000msHandler();
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
