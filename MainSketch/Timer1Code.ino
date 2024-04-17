// *****************************************************************************************
// *******************         TIMER Interrupt Implementation        ***********************
// *****************************************************************************************

#if defined(ESP32)
  #include <Ticker.h>
  Ticker timer1MS;
#elif defined(ESP8266)
  extern "C" {
  #include "user_interface.h"
  } 
  os_timer_t myTimer; 
#else
  #include <TimerOne.h>
#endif

#define MYTIMER_PERIOD_MS 10

unsigned char Timer1_10msCounter = 0;
unsigned char Timer1_100msCounter = 0;
unsigned char Timer1_1000msCounter = 0;
volatile boolean Timer1_1ms_flag = false;
volatile boolean Timer1_10ms_flag = false;
volatile boolean Timer1_100ms_flag = false;
volatile boolean Timer1_1000ms_flag = false;

// metode za glavni Main fajl
inline void Timer1OnInterrupExecute();

inline void Timer1_Initialize(void)
{
#if defined(ESP32)
  timer1MS.attach(0.01, Timer1OnInterrupExecute);
#elif defined(ESP8266)
  os_timer_setfn(&myTimer, timerCallback, NULL); 
  os_timer_arm(&myTimer, MYTIMER_PERIOD_MS, true); 
#else
  Timer1.initialize(MYTIMER_PERIOD_MS * 1000); // 10 ms interrupt
  Timer1.attachInterrupt(Timer1InterruptHandler); // blinkLED to run every 0.10 seconds
#endif
}

inline void timerCallback(void *pArg) 
{
  Timer1OnInterrupExecute();
}

inline void Timer1InterruptHandler(void)
{
  Timer1OnInterrupExecute();
}

void Timer1OnInterrupExecute()
{
  Timer1_1ms_flag = true;
  Timer1_10msCounter++;
  Timer1_10ms_flag = true;
  if (Timer1_10msCounter == 10)
  {
    Timer1_100ms_flag = true;
    Timer1_10msCounter = 0;
    Timer1_100msCounter++;
    if (Timer1_100msCounter == 10)
    {
      Timer1_1000ms_flag = true;
      Timer1_100msCounter = 0;
    }  
  }  
}

inline void Timer1MailLoop(void)
{
  if (Timer1_1ms_flag)
  {
    Timer1_1ms_flag = false;
    Timer1_1msHandler();    
  }
  if (Timer1_10ms_flag)
  {
    Timer1_10ms_flag = false;
    Timer1_10msHandler();    
    if (Timer1_100ms_flag)
    {
      Timer1_100ms_flag = false;
      Timer1_100msHandler();
      if (Timer1_1000ms_flag)
      {
        Timer1_1000ms_flag = false;
        Timer1_1000msHandler();
      }
    }
  }
}

// *****************************************************************************************
// *****************************************************************************************
