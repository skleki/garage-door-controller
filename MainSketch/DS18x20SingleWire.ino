#include "util\FilteredValue.h"

#ifdef DS18x20_pin1

#include <OneWire.h>

#define MAX_NUM_OF_DS18x20_SENSORS 3
OneWire ds0(DS18x20_pin1);  

#ifdef DS18x20_pin2
  OneWire ds1(DS18x20_pin2);  
#endif


// States
#define DS18_S0_GetAndSendCommand 0
#define DS18_S1_ReadTemp 1
#define DS18_S2_Error 2

// Actions
#define DS18_A_100msTimer 0
#define DS18_A_1sTimer 1

byte s0_addr[8], s1_addr[8];
byte s0_type_s, s1_type_s;
int S0_State = DS18_S0_GetAndSendCommand;
int S1_State = DS18_S0_GetAndSendCommand;

FilteredValue s0_temp, s1_temp;

// function prototypes
void Timer1s_Handler_PerSensor(OneWire& ds, int index, String logHeader);
bool GetSensorAndSendReadCommand(OneWire& ds, byte* addr, byte* type_s, int index, String logHeader);
bool ReadTemp(OneWire& ds, byte* addr, byte* type_s, int index, String logHeader, float* celsius);
void DS18_StateMachine(int* state, int action, OneWire& ds, byte* addr, byte* type_s, FilteredValue& temp, int index, String logHeader);

inline void DS18x20_Init(bool loggingEnabled)
{
  s0_temp.init(10, 10);
#ifdef DS18x20_pin2
  s1_temp.init(10, 10);
#endif
}

inline void DS18x20_Timer100ms_Handler()
{
  DS18_StateMachine(&S0_State, DS18_A_100msTimer, ds0, s0_addr, &s0_type_s, s0_temp, 0, "Line 0: ");
#ifdef DS18x20_pin2
  DS18_StateMachine(&S1_State, DS18_A_100msTimer, ds1, s1_addr, &s1_type_s, s1_temp, 1, "Line 1: ");
#endif
}

inline void DS18x20_Timer1s_Handler()
{
  DS18_StateMachine(&S0_State, DS18_A_1sTimer, ds0, s0_addr, &s0_type_s, s0_temp, 0, "Line 0: ");
  s0_temp.ExecuteOn1secondInterval();
#ifdef DS18x20_pin2
  DS18_StateMachine(&S1_State, DS18_A_1sTimer, ds1, s1_addr, &s1_type_s, s1_temp, 1, "Line 1: ");
  s1_temp.ExecuteOn1secondInterval();
#endif


  static int cc;
  cc++;
  if (cc > 120)
  {
    cc = 0;
//    Serialprintln("");

    bool hasValue = s0_temp.hasValue();
    if (hasValue)
    {
      double temp = s0_temp.getValue();
//      Serialprint("Line 0 filtered temp: ");
//      Serialprintln(temp);
    }
    
    hasValue = s1_temp.hasValue();
    if (hasValue)
    {
      double temp = s1_temp.getValue();
//      Serialprint("Line 1 filtered temp: ");
//      Serialprintln(temp);
    }

//    Serialprintln("");
  }
}











bool GetSensorAndSendReadCommand(OneWire& ds, byte* addr, byte* type_s, int index, String logHeader)
{
  byte i;
  byte data[12];
  
  if ( !ds.search(addr)) {
//    Serialprint(logHeader);
//    Serialprintln("No more addresses.");
//    Serialprintln();
    ds.reset_search();
    return false;
  }
  
//  Serialprint(logHeader);
//  Serialprint("ROM =");
//  for( i = 0; i < 8; i++) {
//    Serialprint(' ');
//    Serialprint2A(addr[i], HEX);
//  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
//      Serialprint(logHeader);
//      Serialprintln("CRC is not valid!");
      return false;
  }
  Serialprintln();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
//    case 0x10:
//      Serialprint(logHeader);
      Serialprintln("  Chip = DS18S20");  // or old DS1820
      *type_s = 1;
      break;
    case 0x28:
//      Serialprint(logHeader);
//      Serialprintln("  Chip = DS18B20");
      *type_s = 0;
      break;
    case 0x22:
//      Serialprint(logHeader);
//      Serialprintln("  Chip = DS1822");
      *type_s = 0;
      break;
    default:
//      Serialprint(logHeader);
//      Serialprintln("Device is not a DS18x20 family device.");
      return false;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
//  Serialprint(logHeader);
//  Serialprintln("Sent start conversion command.");

  return true;
}

bool ReadTemp(OneWire& ds, byte* addr, byte* type_s, int index, String logHeader, float* celsius)
{
  byte i;
  byte present = 0;
  byte data[12];
  
//  Serialprint(logHeader);
//  Serialprint("reading temp for ROM=");
//  for( i = 0; i < 8; i++) {
//    Serialprint(' ');
//    Serialprint2A(addr[i], HEX);
//  }
//  Serialprintln("");

  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

//  Serialprint(logHeader);
//  Serialprint("  Data = ");
//  Serialprint2A(present, HEX);
//  Serialprint(" ");
//  for ( i = 0; i < 9; i++) {           // we need 9 bytes
//    Serialprint2A(data[i], HEX);
//    Serialprint(" ");
//  }

  if (OneWire::crc8(data, 8) != data[8])
  {
    Serialprint(" INVALID CRC=");
    Serialprint2A(OneWire::crc8(data, 8), HEX);
    Serialprintln();
    return false;
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (*type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  *celsius = (float)raw / 16.0;
//  Serialprint(logHeader);
//  Serialprint("  Temperature = ");
//  Serialprint(*celsius);
//  Serialprintln(" Celsius, ");

  return true;
}





void DS18_StateMachine(int* state, int action, OneWire& ds, byte* addr, byte* type_s, FilteredValue& temp, int index, String logHeader)
{
  float celsius;

  switch(*state)
  {
    case DS18_S0_GetAndSendCommand:
      switch(action)
      {
        case DS18_A_100msTimer:
          if (GetSensorAndSendReadCommand(ds, addr, type_s, index, logHeader))
            *state = DS18_S1_ReadTemp;
           else
            *state = DS18_S2_Error;
        break;
      }
    break;
    
    case DS18_S1_ReadTemp:
      switch(action)
      {
        case DS18_A_1sTimer:
          if (ReadTemp(ds, addr, type_s, index, logHeader, &celsius))
          {
            temp.add(celsius);
            // zabelezi temperaturu            
                        Serialprint("*** Sensor on line ");
                        Serialprint(index);
                        Serialprint(" with ROM: ");
                        Serialprint2A(addr[7], HEX);
                        Serialprint(" has Temperature: ");
                        Serialprintln(celsius);

            *state = DS18_S0_GetAndSendCommand;
          }
          else
          {
            *state = DS18_S2_Error;
          }
        break;
      }
    break;

    case DS18_S2_Error:
      switch(action)
      {
        case DS18_A_1sTimer:
          *state = DS18_S0_GetAndSendCommand;
        break;
      }
    break;
  }
}







#endif
