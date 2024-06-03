// ******************************************************

#if defined(DOOR_CONTROL)
// This is the main Door control module

//#define DOOR_CLOSE_PIN_ACTIVE_LOW 15
//#define DOOR_CLOSE_PIN_ACTIVE_HIGH 19
//#define DOOR_OPEN_PIN_ACTIVE_LOW 18
//#define DOOR_OPEN_PIN_ACTIVE_HIGH 5
//#define DOOR_CLOSED_SENSOR_PIN 12


void DoorControl_Initialise()
{
//    pinMode(Door_PIN_ACTIVE_LOW, OUTPUT);
//    digitalWrite(Door_PIN_ACTIVE_LOW, HIGH);
//    pinMode(Door_PIN_ACTIVE_HIGH, OUTPUT);
//    digitalWrite(Door_PIN_ACTIVE_HIGH, LOW);

  
  Serialprint("DoorControl: ManualEnabled: ");
}

inline void DoorControl_Loop(void)
{
}

inline void DoorControl_1msHandler(void)
{
}

inline void DoorControl_100msHandler(void)
{
}

inline void DoorControl_1000msHandler(void)
{
}

#endif
