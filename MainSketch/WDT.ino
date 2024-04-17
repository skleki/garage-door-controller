#if defined (ENABLE_WDT)
	#if defined (ESP32)
		#include <esp_task_wdt.h>
	#endif

	void WDT_Initialise(void)
	{
    #if defined (ESP8266)
      wdt_enable(WDTO_8S);
    #elif defined (ESP32)
      esp_task_wdt_init(16, true); //enable panic so ESP32 restarts
      esp_task_wdt_add(NULL); //add current thread to WDT watch
    #endif
	}

	void WDT_Reset(void)
	{
	#if defined (ESP8266)
		wdt_reset();
	#elif defined (ESP32)
		esp_task_wdt_reset();
	#endif
	}

#else  
	void WDT_Initialise(void)
	{
	}

	void WDT_Reset(void)
	{
	}
#endif





















#ifdef HM_LEDS

// *****************************************************************************************
// **********         Heater Module LEDs State Machine Implementation        ***************
// *****************************************************************************************

//typedef enum led1
//{
//  CommsLost,
//  CommsActive
//} HM_COMMS_LED_Statue_Enum;

const byte LEDS_COMMS_ACTIVE_SEQ_LEN = 2;
const unsigned int LEDS_COMMS_ACTIVE_SEQ[] = { 2900, 100 };
const byte LEDS_COMMS_LOST_SEQ_LEN = 4;
const unsigned int LEDS_COMMS_LOST_SEQ[] = { 50, 200, 50, 4700 };

const byte LEDS_HEATER_COMMAND0_POWSW0_POWHT0_SEQ_LEN = 2;
const unsigned int LEDS_HEATER_COMMAND0_POWSW0_POWHT0_SEQ[] = { 0, 3800,  };
const byte LEDS_HEATER_COMMAND0_POWSW1_POWHT0_SEQ_LEN = 4;
const unsigned int LEDS_HEATER_COMMAND0_POWSW1_POWHT0_SEQ[] = { 0, 1200, 100, 2500 };
const byte LEDS_HEATER_COMMAND0_POWSW0_POWHT1_SEQ_LEN = 6;
const unsigned int LEDS_HEATER_COMMAND0_POWSW0_POWHT1_SEQ[] = { 0, 1200, 100, 100, 100, 2300 }; // 3800
const byte LEDS_HEATER_COMMAND0_POWSW1_POWHT1_SEQ_LEN = 8;
const unsigned int LEDS_HEATER_COMMAND0_POWSW1_POWHT1_SEQ[] = { 0, 1200, 100, 100 , 100, 100, 100, 2100}; //3800
const byte LEDS_HEATER_COMMAND1_POWSW0_POWHT0_SEQ_LEN = 2;
const unsigned int LEDS_HEATER_COMMAND1_POWSW0_POWHT0_SEQ[] = { 1000, 2800,  };
const byte LEDS_HEATER_COMMAND1_POWSW1_POWHT0_SEQ_LEN = 4;
const unsigned int LEDS_HEATER_COMMAND1_POWSW1_POWHT0_SEQ[] = { 1000, 200, 100, 2500 };
const byte LEDS_HEATER_COMMAND1_POWSW0_POWHT1_SEQ_LEN = 6;
const unsigned int LEDS_HEATER_COMMAND1_POWSW0_POWHT1_SEQ[] = { 1000, 200, 100, 100, 100, 2300 }; // 3800
const byte LEDS_HEATER_COMMAND1_POWSW1_POWHT1_SEQ_LEN = 8;
const unsigned int LEDS_HEATER_COMMAND1_POWSW1_POWHT1_SEQ[] = { 1000, 200, 100, 100 , 100, 100, 100, 2100}; //3800
const byte LEDS_HEATER_ERROR_SEQ_LEN = 2;
const unsigned int LEDS_HEATER_ERROR_SEQ[] = { 100, 100 }; //200


unsigned int *LedCommsSeq;
byte LedCommsSeqLen;
byte LedCommsSeqCurrentSeg;  
unsigned int LedCommsCounter;

unsigned int *LedHeaterSeq;
byte LedHeaterSeqLen;
byte LedHeaterSeqCurrentSeg;  
unsigned int LedHeaterCounter;

inline void HM_LEDs_Initialise(void)
{
  pinMode(HM_COMMS_LED_PIN, OUTPUT);
  LedCommsSeq = (unsigned int *)LEDS_COMMS_LOST_SEQ;
  LedCommsSeqLen = LEDS_COMMS_LOST_SEQ_LEN;
  LedCommsSeqCurrentSeg = 0;  
  LedCommsCounter = 0;
  digitalWrite(HM_COMMS_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)

  pinMode(HM_HEATER_LED_PIN, OUTPUT);
  LedHeaterSeq = (unsigned int *)LEDS_HEATER_COMMAND0_POWSW0_POWHT0_SEQ;
  LedHeaterSeqLen = LEDS_HEATER_COMMAND0_POWSW0_POWHT0_SEQ_LEN;
  LedHeaterSeqCurrentSeg = 0;  
  LedHeaterCounter = 0;
  digitalWrite(HM_HEATER_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
}

inline void HM_LED_CommsLost(void)
{
  LedCommsSeq = (unsigned int *)LEDS_COMMS_LOST_SEQ;
  LedCommsSeqLen = LEDS_COMMS_LOST_SEQ_LEN;
  LedCommsSeqCurrentSeg = 0;  
  LedCommsCounter = 0;
  digitalWrite(HM_COMMS_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
}
inline void HM_LED_CommsActive(void)
{
  LedCommsSeq = (unsigned int *)LEDS_COMMS_ACTIVE_SEQ;
  LedCommsSeqLen = LEDS_COMMS_ACTIVE_SEQ_LEN;
  LedCommsSeqCurrentSeg = 0;  
  LedCommsCounter = 0;
  digitalWrite(HM_COMMS_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
}

inline void HM_LED_HeaterCommand0PowSw0PowHt0(void)
{
  LedHeaterSeq = (unsigned int *)LEDS_HEATER_COMMAND0_POWSW0_POWHT0_SEQ;
  LedHeaterSeqLen = LEDS_HEATER_COMMAND0_POWSW0_POWHT0_SEQ_LEN;
  LedHeaterSeqCurrentSeg = 0;  
  LedHeaterCounter = 0;
  digitalWrite(HM_HEATER_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
}
inline void HM_LED_HeaterCommand0PowSw1PowHt0(void)
{
  LedHeaterSeq = (unsigned int *)LEDS_HEATER_COMMAND0_POWSW1_POWHT0_SEQ;
  LedHeaterSeqLen = LEDS_HEATER_COMMAND0_POWSW1_POWHT0_SEQ_LEN;
  LedHeaterSeqCurrentSeg = 0;  
  LedHeaterCounter = 0;
  digitalWrite(HM_HEATER_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
}
inline void HM_LED_HeaterCommand0PowSw0PowHt1(void)
{
  LedHeaterSeq = (unsigned int *)LEDS_HEATER_COMMAND0_POWSW0_POWHT1_SEQ;
  LedHeaterSeqLen = LEDS_HEATER_COMMAND0_POWSW0_POWHT1_SEQ_LEN;
  LedHeaterSeqCurrentSeg = 0;  
  LedHeaterCounter = 0;
  digitalWrite(HM_HEATER_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
}
inline void HM_LED_HeaterCommand0PowSw1PowHt1(void)
{
  LedHeaterSeq = (unsigned int *)LEDS_HEATER_COMMAND0_POWSW1_POWHT1_SEQ;
  LedHeaterSeqLen = LEDS_HEATER_COMMAND0_POWSW1_POWHT1_SEQ_LEN;
  LedHeaterSeqCurrentSeg = 0;  
  LedHeaterCounter = 0;
  digitalWrite(HM_HEATER_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
}
inline void HM_LED_HeaterCommand1PowSw0PowHt0(void)
{
  LedHeaterSeq = (unsigned int *)LEDS_HEATER_COMMAND1_POWSW0_POWHT0_SEQ;
  LedHeaterSeqLen = LEDS_HEATER_COMMAND1_POWSW0_POWHT0_SEQ_LEN;
  LedHeaterSeqCurrentSeg = 0;  
  LedHeaterCounter = 0;
  digitalWrite(HM_HEATER_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
}
inline void HM_LED_HeaterCommand1PowSw1PowHt0(void)
{
  LedHeaterSeq = (unsigned int *)LEDS_HEATER_COMMAND1_POWSW1_POWHT0_SEQ;
  LedHeaterSeqLen = LEDS_HEATER_COMMAND1_POWSW1_POWHT0_SEQ_LEN;
  LedHeaterSeqCurrentSeg = 0;  
  LedHeaterCounter = 0;
  digitalWrite(HM_HEATER_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
}
inline void HM_LED_HeaterCommand1PowSw0PowHt1(void)
{
  LedHeaterSeq = (unsigned int *)LEDS_HEATER_COMMAND1_POWSW0_POWHT1_SEQ;
  LedHeaterSeqLen = LEDS_HEATER_COMMAND1_POWSW0_POWHT1_SEQ_LEN;
  LedHeaterSeqCurrentSeg = 0;  
  LedHeaterCounter = 0;
  digitalWrite(HM_HEATER_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
}
inline void HM_LED_HeaterCommand1PowSw1PowHt1(void)
{
  LedHeaterSeq = (unsigned int *)LEDS_HEATER_COMMAND1_POWSW1_POWHT1_SEQ;
  LedHeaterSeqLen = LEDS_HEATER_COMMAND1_POWSW1_POWHT1_SEQ_LEN;
  LedHeaterSeqCurrentSeg = 0;  
  LedHeaterCounter = 0;
  digitalWrite(HM_HEATER_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
}
inline void HM_LED_HeaterError(void)
{
  LedHeaterSeq = (unsigned int *)LEDS_HEATER_ERROR_SEQ;
  LedHeaterSeqLen = LEDS_HEATER_ERROR_SEQ_LEN;
  LedHeaterSeqCurrentSeg = 0;  
  LedHeaterCounter = 0;
  digitalWrite(HM_HEATER_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
}

#define LED_TIMER_INCREMENT 10
inline void HM_LEDs_10msHandler(void)
{
  LedCommsCounter += LED_TIMER_INCREMENT;
  if (LedCommsCounter >= LedCommsSeq[LedCommsSeqCurrentSeg])
  {
    LedCommsSeqCurrentSeg++;
    LedCommsCounter = 0;
    if (LedCommsSeqCurrentSeg >= LedCommsSeqLen)
    {
      LedCommsSeqCurrentSeg = 0;
    }
    if ((LedCommsSeqCurrentSeg % 2) == 0)
    {
      digitalWrite(HM_COMMS_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
    }
    else
    {
      digitalWrite(HM_COMMS_LED_PIN, false);   // turn the LED on (HIGH is the voltage level)
    }
  }

  LedHeaterCounter += LED_TIMER_INCREMENT;
  if (LedHeaterCounter >= LedHeaterSeq[LedHeaterSeqCurrentSeg])
  {
    LedHeaterSeqCurrentSeg++;
    LedHeaterCounter = 0;
    if (LedHeaterSeqCurrentSeg >= LedHeaterSeqLen)
    {
      LedHeaterSeqCurrentSeg = 0;
    }
    if ((LedHeaterSeqCurrentSeg % 2) == 0)
    {
      digitalWrite(HM_HEATER_LED_PIN, true);   // turn the LED on (HIGH is the voltage level)
    }
    else
    {
      digitalWrite(HM_HEATER_LED_PIN, false);   // turn the LED on (HIGH is the voltage level)
    }
  }
}

// *****************************************************************************************
// *****************************************************************************************

#endif
