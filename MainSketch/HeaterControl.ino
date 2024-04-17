// ******************************************************
// Sredi situaciju ako nije dostupna HeaterControl_IsRoomTemperatureAvailable()

#if defined(GSM_THERMOSTAT)
// This is the main Heater control module
// DESC: Manuelna ili Auto kontrola odredjuju da li grejac treba da radi (HeaterControl_HeaterShouldBeOnFlag)
// a histeresis kontrola ga stvarno pali i gasi na osnovu sone temp

#define HEATER_PIN_ACTIVE_LOW 27
#define HEATER_PIN_ACTIVE_HIGH 14

#define THERMOSTAT_HISTERESIS_RANGE 0.4

bool HeaterControl_HeaterState;
bool HeaterControl_HeaterShouldBeOnFlag;
inline void HeaterControl_SetDisplayData(void);

extern ESP32Time rtc;

void HeaterControl_Initialise()
{
    pinMode(HEATER_PIN_ACTIVE_LOW, OUTPUT);
    digitalWrite(HEATER_PIN_ACTIVE_LOW, HIGH);
    pinMode(HEATER_PIN_ACTIVE_HIGH, OUTPUT);
    digitalWrite(HEATER_PIN_ACTIVE_HIGH, LOW);

  
  Serialprint("HeaterControl: ManualEnabled: ");
  Serialprintln(EEPROMDataThermostat.ManualEnabled);
  Serialprint("HeaterControl: DesiredTemp: ");
  Serialprintln(EEPROMDataThermostat.DesiredTemp);
  Serialprint("HeaterControl: AutomaticEnabled: ");
  Serialprintln(EEPROMDataThermostat.AutomaticEnabled);
  Serialprint("HeaterControl: AutomaticStart: ");
  Serialprint(EEPROMDataThermostat.AutomaticStartHour);
  Serialprint(":");
  Serialprintln(EEPROMDataThermostat.AutomaticStartMin);
  Serialprint("HeaterControl: AutomaticEnd: ");
  Serialprint(EEPROMDataThermostat.AutomaticEndHour);
  Serialprint(":");
  Serialprintln(EEPROMDataThermostat.AutomaticEndMin);

  if (!EEPROMDataThermostat.ManualEnabled && !EEPROMDataThermostat.AutomaticEnabled)
  {
    HeaterControl_HeaterShouldBeOn(false);
  }
  else if (EEPROMDataThermostat.ManualEnabled)
  {
    HeaterControl_HeaterShouldBeOn(true);
  }
  else if (EEPROMDataThermostat.AutomaticEnabled)
  {
    HeaterControl_CheckAutomaticControl();
  }

  HeaterControl_SetDisplayData();
}

inline void HeaterControl_Loop(void)
{
}

inline void HeaterControl_1msHandler(void)
{
}

inline void HeaterControl_100msHandler(void)
{
}

inline void HeaterControl_1000msHandler(void)
{
  static int counterCheckAutomatic = 0;
  counterCheckAutomatic++;
  if (counterCheckAutomatic >= 10)
  {
    counterCheckAutomatic = 0;
        
    if (EEPROMDataThermostat.AutomaticEnabled)
    {
      HeaterControl_CheckAutomaticControl();
    }
  }

  static int counterCheckHeaterOn = 0;
  counterCheckHeaterOn++;
  if (counterCheckHeaterOn >= 10)
  {
    counterCheckHeaterOn = 0;

    if (HeaterControl_HeaterShouldBeOnFlag)
    {
      HeaterControl_HeaterHisteresisControl();
    }
  }

  if (HeaterControl_IsRoomTemperatureAvailable())
  {
  }
  if (HeaterControl_IsWaterTemperatureAvailable())
  {
  }
}

inline void HeaterControl_SetDisplayData(void)
{
}

inline bool HeaterControl_IsRoomTemperatureAvailable()
{
  return s0_temp.hasValue();
}

inline float HeaterControl_RoomTemperature()
{
    bool hasValue = s0_temp.hasValue();
    if (hasValue)
    {
      return s0_temp.getValue();
    }
    return 999.99;
}

inline bool HeaterControl_IsWaterTemperatureAvailable()
{
  return s1_temp.hasValue();
}

inline float HeaterControl_WaterTemperature()
{
    bool hasValue = s1_temp.hasValue();
    if (hasValue)
    {
      return s1_temp.getValue();
    }
    return 999.99;
}

inline bool HeaterControl_IsHeaterOnATM()
{
  return HeaterControl_HeaterState;
}

inline void HeaterControl_GetManualEnabled(bool* enabled, float* desiredTemp)
{
  *enabled = EEPROMDataThermostat.ManualEnabled;
  *desiredTemp = EEPROMDataThermostat.DesiredTemp;
}

inline void HeaterControl_GetAutomaticEnabled(bool* enabled, float* desiredTemp, byte* startHour, byte* startMin, byte* endHour, byte* endMin)
{
  *enabled = EEPROMDataThermostat.AutomaticEnabled;
  *desiredTemp = EEPROMDataThermostat.DesiredTemp;
  *startHour = EEPROMDataThermostat.AutomaticStartHour;
  *startMin= EEPROMDataThermostat.AutomaticStartMin;
  *endHour = EEPROMDataThermostat.AutomaticEndHour;
  *endMin= EEPROMDataThermostat.AutomaticEndMin;
}

inline void HeaterControl_ManualEnable(float desiredTemp)
{
  Serialprint("HeaterControl => Manual ENABLE with desired temp: ");
  Serialprintln(desiredTemp);
  
  EEPROMDataThermostat.ManualEnabled = true;
  EEPROMDataThermostat.DesiredTemp = desiredTemp;
  EEPROMDataThermostat.AutomaticEnabled = false;
  SaveEEPROMDataThermostat();
  HeaterControl_HeaterShouldBeOn(true);

  HeaterControl_SetDisplayData();
}

inline void HeaterControl_ManualDisable()
{
  Serialprintln("HeaterControl => Manual DISABLE!");

  if (EEPROMDataThermostat.ManualEnabled)
  {
    EEPROMDataThermostat.ManualEnabled = false;
    SaveEEPROMDataThermostat();
    HeaterControl_HeaterShouldBeOn(false);

    HeaterControl_SetDisplayData();
  }
}

inline void HeaterControl_AutomaticEnable(float desiredTemp, byte startHour, byte startMin, byte endHour, byte endMin)
{
  Serialprint("HeaterControl => Automatic ENABLE with desired temp: ");
  Serialprintln(desiredTemp);
  Serialprint("   Start time: ");
  Serialprintln(startHour);
  Serialprint(":");
  Serialprint(startMin);
  Serialprint("   End time: ");
  Serialprint(endHour);
  Serialprint(":");
  Serialprint(endMin);

  EEPROMDataThermostat.AutomaticEnabled = true;
  EEPROMDataThermostat.DesiredTemp = desiredTemp;
  EEPROMDataThermostat.AutomaticStartHour = startHour;
  EEPROMDataThermostat.AutomaticStartMin = startMin;
  EEPROMDataThermostat.AutomaticEndHour = endHour;
  EEPROMDataThermostat.AutomaticEndMin = endMin;
  EEPROMDataThermostat.ManualEnabled = false;
  SaveEEPROMDataThermostat();

  HeaterControl_SetDisplayData();
  
  HeaterControl_CheckAutomaticControl();
}

inline void HeaterControl_AutomaticDisable()
{
  Serialprintln("HeaterControl => Automatic DISABLE!");

  if (EEPROMDataThermostat.AutomaticEnabled)
  {
    EEPROMDataThermostat.AutomaticEnabled = false;
    SaveEEPROMDataThermostat();
    HeaterControl_HeaterShouldBeOn(false);

    HeaterControl_SetDisplayData();
  }
}

bool HeaterControl_IntervalIsActive(byte startHour, byte startMin,
                                    byte endHour, byte endMin,
                                    byte currentHour, byte currentMin)
{
  int start = startHour * 60 + startMin;
  int end = endHour * 60 + endMin;
  int current = currentHour * 60 + currentMin;
 
  if (start <= end)
  {
    if (current >= start && current <= end)
    {
     return true;
    }
    else
    {
     return false;
    }
  }
  else // Interval includes midnight
  {
    if (current >= start || current <= end)
    {
     return true;
    }
    else
    {
     return false;
    }
  }
}

inline void HeaterControl_CheckAutomaticControl()
{
  if (!HeaterControl_IsRoomTemperatureAvailable())
  {
    // Nije moguca auto kontrola ako ne moze da se iscita temp sobe
    Serialprintln("Heater AutomaticControl not possible when room temp is unavailable");
    return;
  }

  if (!rtc.isTimeSet())
  {
    Serialprintln("Heater AutomaticControl not possible when time is not configured");
    return;
  }

  int currentHour = rtc.getHour(true);
  int currentMin = rtc.getMinute();
  HeaterControl_HeaterShouldBeOn(
    HeaterControl_IntervalIsActive(EEPROMDataThermostat.AutomaticStartHour, EEPROMDataThermostat.AutomaticStartMin,
                                   EEPROMDataThermostat.AutomaticEndHour, EEPROMDataThermostat.AutomaticEndMin,
                                   currentHour, currentMin));
}

void HeaterControl_HeaterShouldBeOn(bool heaterOn)
{
  HeaterControl_HeaterShouldBeOnFlag = heaterOn;
  if (!HeaterControl_HeaterShouldBeOnFlag)
  {
    HeaterControl_HeaterEnabled(HeaterControl_HeaterShouldBeOnFlag);
  }
  
  HeaterControl_HeaterHisteresisControl();
}

void HeaterControl_HeaterHisteresisControl()
{
  if (HeaterControl_HeaterShouldBeOnFlag)
  {
    if (HeaterControl_IsRoomTemperatureAvailable())
    {
      if ((HeaterControl_RoomTemperature() < EEPROMDataThermostat.DesiredTemp) &&
          (EEPROMDataThermostat.DesiredTemp - HeaterControl_RoomTemperature()) > THERMOSTAT_HISTERESIS_RANGE/2)
      {
        HeaterControl_HeaterEnabled(true);
      }
      else if ((HeaterControl_RoomTemperature() > EEPROMDataThermostat.DesiredTemp) &&
               (HeaterControl_RoomTemperature() - EEPROMDataThermostat.DesiredTemp) > THERMOSTAT_HISTERESIS_RANGE/2)
      {
        HeaterControl_HeaterEnabled(false);
      }
    }
    else
    {
      if (EEPROMDataThermostat.ManualEnabled)
      {
        HeaterControl_HeaterEnabled(true);
      }
      else
      {
        HeaterControl_HeaterEnabled(false);
      }
    }
  }
}

void HeaterControl_HeaterEnabled(bool heaterOn)
{
  HeaterControl_HeaterState = heaterOn;
  if (HeaterControl_HeaterState)
  {
    Serialprintln("Setting relay ON");
    digitalWrite(HEATER_PIN_ACTIVE_LOW, LOW);
    digitalWrite(HEATER_PIN_ACTIVE_HIGH, HIGH);
  }
  else
  {
    Serialprintln("Setting relay OFF");
    digitalWrite(HEATER_PIN_ACTIVE_LOW, HIGH);
    digitalWrite(HEATER_PIN_ACTIVE_HIGH, LOW);
  }

}
#endif
