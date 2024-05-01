// *****************************************************************************************
// ************       Use #define directives to enable hardware support      ***************

// #define ESP8266
// #define ESP32   // this is already defined when you select ESP32 board
#define ESP32_WROOM_32_30PIN  // "ESP32 Dev Module"
// #define ESP32_HelTex_WiFi_Kit_36pin

#define GSM_THERMOSTAT
#define OLED_DISPLAY

#define GSM_MODEM
#define ENABLE_WDT
#define UNO_BOARD_LEDS


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
extern void Signal_LEDs_10msHandler(void);
extern inline void DS18x20_Init(bool loggingEnabled);
extern inline void DS18x20_Timer1s_Handler();


// *****************************************************************************************
// ******************          S E T U P        ********************************************
void setup() 
{
	pinMode(RS232_PROTOCOL_PIN, INPUT_PULLUP);
	delay(50);

	Signal_LEDs_Initialise();

	Serialbegin(115200);
	ForceSerialprintln("Starting Arduino !!!");

	Timer1_Initialize();

	DS18x20_Init(false);

  WDT_Initialise();
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
}
// *****************************************************************************************
// *****************************************************************************************




// *****************************************************************************************
// ****************           T I M E R    H A N D L E R S        **************************
inline void Timer1_1msHandler(void)
{
//  HeaterControl_1msHandler();
}

inline void Timer1_10msHandler(void)
{
  Signal_LEDs_10msHandler();
}

inline void Timer1_100msHandler(void)
{
  DS18x20_Timer100ms_Handler();
  GsmLogic_100msHandler();
//  HeaterControl_100msHandler();
}

inline void Timer1_1000msHandler(void)
{
  DS18x20_Timer1s_Handler();
  GsmLogic_1sHandler();
//  HeaterControl_1000msHandler();
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
