// ******************************************************

#if defined(DOOR_CONTROL)
// This is the main Door control module

//#define DOOR_CLOSE_PIN_ACTIVE_LOW 15
//#define DOOR_CLOSE_PIN_ACTIVE_HIGH 19
//#define DOOR_OPEN_PIN_ACTIVE_LOW 18
//#define DOOR_OPEN_PIN_ACTIVE_HIGH 5
//#define DOOR_CLOSED_SENSOR_PIN 12

#define DOOR_CONTROL_PULSE_LEN_MS 300
typedef enum DoorControlState {
  Idle,
  ClosingSignalActive,
  OpeningSignalActive,
} DoorControlStateEnum;

DoorControlStateEnum DoorControl_State = Idle;
int DoorControl_PuseCounter = 0;

void DoorControl_Initialise()
{
  pinMode(DOOR_CLOSE_PIN_ACTIVE_LOW, OUTPUT);
  digitalWrite(DOOR_CLOSE_PIN_ACTIVE_LOW, HIGH);
  pinMode(DOOR_CLOSE_PIN_ACTIVE_HIGH, OUTPUT);
  digitalWrite(DOOR_CLOSE_PIN_ACTIVE_LOW, LOW);

  pinMode(DOOR_OPEN_PIN_ACTIVE_LOW, OUTPUT);
  digitalWrite(DOOR_OPEN_PIN_ACTIVE_LOW, HIGH);
  pinMode(DOOR_OPEN_PIN_ACTIVE_HIGH, OUTPUT);
  digitalWrite(DOOR_OPEN_PIN_ACTIVE_HIGH, LOW);

  pinMode(DOOR_CLOSED_SENSOR_PIN, INPUT_PULLUP);

  delay(50);
  Serialprint("DoorControl: Currently ");
  if(DoorControl_IsDoorClosed())
  {
    Serialprintln("CLOSED");
  }
  else
  {
    Serialprintln("OPENED");
  }
}

inline void DoorControl_Loop(void)
{
}

inline void DoorControl_1msHandler(void)
{
}

inline void DoorControl_10msHandler(void)
{
  if(DoorControl_State != Idle)
  {
    DoorControl_PuseCounter++;
    if(DoorControl_PuseCounter > (DOOR_CONTROL_PULSE_LEN_MS / 10))
    {
      if(DoorControl_State == OpeningSignalActive)
      {
        DoorControl_DeactivateOpenCircuit();
      }
      else
      {
        DoorControl_DeactivateCloseCircuit();
      }

      DoorControl_State = Idle;
      DoorControl_PuseCounter = 0;
    }
  }
}

inline void DoorControl_1000msHandler(void)
{
//  // this is for testing only, open and close on 5 seconds
//  static int cc = 0;
//  cc++;
//  if(cc == 5)
//  {
//    DoorControl_Open();
//  }
//  else if(cc == 10)
//  {
//    DoorControl_Close();
//    cc = 0;
//  }
}

void DoorControl_Open(void)
{
  Serialprintln("DoorControl: Received OPEN command");
  DoorControl_PuseCounter = 0;
  DoorControl_State = OpeningSignalActive;
  DoorControl_DeactivateCloseCircuit();
  DoorControl_ActivateOpenCircuit();
}

void DoorControl_Close(void)
{
  Serialprintln("DoorControl: Received CLOSE command");
  DoorControl_PuseCounter = 0;
  DoorControl_State = ClosingSignalActive;
  DoorControl_DeactivateOpenCircuit();
  DoorControl_ActivateCloseCircuit();
}

bool DoorControl_IsDoorClosed(void)
{
  return digitalRead(DOOR_CLOSED_SENSOR_PIN) == LOW;
}

void DoorControl_ActivateOpenCircuit(void)
{
  digitalWrite(DOOR_OPEN_PIN_ACTIVE_LOW, LOW);
  digitalWrite(DOOR_OPEN_PIN_ACTIVE_HIGH, HIGH);
}

void DoorControl_DeactivateOpenCircuit(void)
{
  digitalWrite(DOOR_OPEN_PIN_ACTIVE_LOW, HIGH);
  digitalWrite(DOOR_OPEN_PIN_ACTIVE_HIGH, LOW);
}

void DoorControl_ActivateCloseCircuit(void)
{
  digitalWrite(DOOR_CLOSE_PIN_ACTIVE_LOW, LOW);
  digitalWrite(DOOR_CLOSE_PIN_ACTIVE_HIGH, HIGH);
}

void DoorControl_DeactivateCloseCircuit(void)
{
  digitalWrite(DOOR_CLOSE_PIN_ACTIVE_LOW, HIGH);
  digitalWrite(DOOR_CLOSE_PIN_ACTIVE_HIGH, LOW);
}


#endif
