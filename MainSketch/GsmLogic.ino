#ifdef GSM_MODEM

#include "cppQueue.h"

#define GSM_TIME_ZONE 1
#define GCM_CREDIT_WARNING_LIMIT 300.0

#define ADMIN_NUMBER "+381642023960"
#define REGISTRED_NUMBERS_COUNT 2
const String REGISTRED_NUMBERS[REGISTRED_NUMBERS_COUNT] = {
    "+381642023960",
    "+381642042030"};

#define SEND_LOW_CREDIT_NOTIFICATION_NUMBER 5

//// For test, smaller values
//#define GET_OPERATOR_CREDIT_ATTEMPT_INTERVAL_SEC 3600
//#define GSM_OPERATOR_CREDIT_CHECK_INTERVAL 24*60*1000
//#define GSM_SIGNAL_STRENGTH_CHECK_INTERVAL 10*60*1000

#define GET_OPERATOR_CREDIT_ATTEMPT_INTERVAL_SEC 3600
#define GSM_OPERATOR_CREDIT_CHECK_INTERVAL 24*(60 - 3)*60*1000
#define GSM_SIGNAL_STRENGTH_CHECK_INTERVAL 10*60*1000

#define GLSM_S_INIT 0
#define GLSM_S_ERROR 1
#define GLSM_S_GSM_RESET_WAIT 2
#define GLSM_S_SIMPLE_INIT_AT_SENT 3
#define GLSM_S_SIMPLE_INIT_AT_PAUSE 4
#define GLSM_S_GET_CREDIT 9
#define GLSM_S_GET_SIGNAL_STRENGTH 10
#define GLSM_S_WAIT_SMS_DELETE 11
#define GLSM_S_WAIT_SMS_SEND_CONFIRMATION 12
#define GLSM_S_IDLE 15

#define GLSM_A_2sTick 0
#define GLSM_A_ParserOK 1
#define GLSM_A_ParserError 2
#define GLSM_A_ParserTimeout 3

int GsmLogicState;
bool SignalStrengthAcquired;
long SignalStrengthTimeAcquired;
long GsmSM_counter = 0;
//long getGsmTimeAttempCounterSec = GET_GSM_TIME_ATTEMPT_INTERVAL_SEC;
long getOperatorCreditAttempCounterSec = GET_OPERATOR_CREDIT_ATTEMPT_INTERVAL_SEC;
void GsmLogic_ParseSms(String& callerNumber, struct tm& smsTime, String& smsContent);

bool GsmCurrentCreditAcquired;
bool ResetSmsSent;
float GsmCurrentCredit;
bool GsmSignalStrengthDbAcquired;
int GsmSignalStrengthDb;
int GsmSentLowCreditWarningNumber;

bool SmsIsReceived;

struct SmsForSending {
  String receiverNumber;
  String smsContent;
};

#define SMS_MAX_RECEIVER_NUMBER_LENGTH 20
#define SMS_MAX_CONTENT_LENGTH 500
struct SmsForSendingInternal {
  char receiverNumber[SMS_MAX_RECEIVER_NUMBER_LENGTH];
  int receiverNumberLength;
  char smsContent[SMS_MAX_CONTENT_LENGTH];
  int smsContentLength;
};
struct SmsForSendingInternal smsForQueue;

cppQueue SmsQueue(sizeof(struct SmsForSendingInternal), 5, FIFO, false);  

bool OperatorsCreditAcquired;
unsigned long GsmLogic_OperatorsCreditTimeAcquired;

int TwoSecondsTimerCounter = 0;

// Function prototypes
void GsmLogic_InternalInit();
void GsmCommands_Initialise(TParserCallbackNotification parserCallbackNotification,
                            TReceivedSmsNotificationFunction receivedSmsNotificationFunction,
                            TReceivedRingNotificationFunction receivedRingNotificationFunction,
                            TReceivedOperatorCreditFunction receivedOperatorCreditFunction,
                            TReceivedTimeFunction receivedGsmTimeFunction,
                            TReceivedSignalStrengthFunction receivedSignalStrengthFunction);
void GsmLogicStateMachine(int action);
inline bool CanSendAtCommand();
inline void SetupIdleParser();
inline void InitSendingSimpleInitCommands();
inline bool SendSimpleInitCommand();
inline void InitSendingSimpleTimeCommands();
inline bool SendSimpleTimeCommand();
inline void SendGetGsmTimeCommand();
void ResetGsmModem();
bool PushSmsIntoSendingQueue(struct SmsForSending* sms);
bool PeekSmsFromSendingQueue(struct SmsForSending* sms);
bool PopSmsFromSendingQueue(struct SmsForSending* sms);


void ReceivedSmsNotificationFunction(String callerNumber, struct tm smsTime, String smsContent);
void ReceivedRingNotificationFunction(String callerNumber);
void ReceivedOperatorCreditFunction(float credit);
void ReceivedGsmTimeFunction(struct tm gsmTime);
void ReceivedGsmSignalStrengthFunction(int signalStrengthDb);


void GsmLogic_Initialise(void)
{
  GsmLogic_InternalInit();
  
  GsmLogicState = GLSM_S_ERROR;
	GsmCommands_Initialise(ParserCallbackNotification, 
	                       ReceivedSmsNotificationFunction, 
	                       ReceivedRingNotificationFunction,
	                       ReceivedOperatorCreditFunction,
	                       ReceivedGsmTimeFunction,
	                       ReceivedGsmSignalStrengthFunction);
}

void GsmLogic_InternalInit()
{
  SmsIsReceived = false;
  OperatorsCreditAcquired = false;
  GsmLogic_OperatorsCreditTimeAcquired = 0;
  SignalStrengthAcquired = false;
  SignalStrengthTimeAcquired = - GSM_SIGNAL_STRENGTH_CHECK_INTERVAL;
  getOperatorCreditAttempCounterSec = GET_OPERATOR_CREDIT_ATTEMPT_INTERVAL_SEC;
  GsmCurrentCreditAcquired = false;
  ResetSmsSent = false;
  GsmSignalStrengthDbAcquired = false;
  GsmSentLowCreditWarningNumber = 0;
}

void GsmLogic_loop(void)
{
	GsmCommands_loop();
}

void GsmLogic_100msHandler(void)
{
	GsmCommands_100msHandler();
}

void GsmLogic_1sHandler(void)
{
	GsmCommands_1sHandler();

  TwoSecondsTimerCounter++;
  if (TwoSecondsTimerCounter >= 2)
  {
    TwoSecondsTimerCounter = 0;
    GsmLogicStateMachine(GLSM_A_2sTick);
  }

  static int waitForSendingResetSms = 0;
  waitForSendingResetSms++;
  if (!ResetSmsSent &&
     (waitForSendingResetSms > 600 || GsmCurrentCreditAcquired))
  {
    ResetSmsSent = true;
    struct SmsForSending resetSms;
    resetSms.receiverNumber = ADMIN_NUMBER;
    resetSms.smsContent = "RESET OCCURED\r\n" + GsmLogic_StatusReport();
    
    // TODO: uncoment this for production
    // PushSmsIntoSendingQueue(&resetSms);
  }
}

String GsmStatusPart0()
{
  if (!GsmCurrentCreditAcquired)
  {
    return "unavailable";
  }
  else
  {
    String gsmStatus = "Cred: " + String(GsmCurrentCredit);
    return gsmStatus;
  }
}

String GsmStatusPart1()
{
  if (!GsmSignalStrengthDbAcquired)
  {
    return "unavailable";
  }
  else
  {
    String gsmStatus = "Sig: " + String(GsmSignalStrengthDb) + "db";
    return gsmStatus;
  }
}

void ParserCallbackNotification(int parserEvent)
{
  if (parserEvent == GsmParserOK)
  {
    GsmLogicStateMachine(GLSM_A_ParserOK);
  }
  else if (parserEvent == GsmParserERROR)
  {
    GsmLogicStateMachine(GLSM_A_ParserError);
  }
  else if (parserEvent == GsmParserTimeout)
  {
    GsmLogicStateMachine(GLSM_A_ParserTimeout);
  }
}

void ReceivedSmsNotificationFunction(String callerNumber, struct tm smsTime, String smsContent)
{
  Serialprint("###########   SMS Received Notification callerNumber: ");
  Serialprintln(callerNumber);
  Serialprint("###########   SMS Received Notification Time: ");
  Serialprint(smsTime.tm_hour);
  Serialprint(":");
  Serialprint(smsTime.tm_min);
  Serialprint(":");
  Serialprint(smsTime.tm_sec);
  Serialprint(" ");
  Serialprint(smsTime.tm_mday);
  Serialprint(".");
  Serialprint(smsTime.tm_mon);
  Serialprint(".");
  Serialprintln(smsTime.tm_year + 1900);
//tm_sec  int seconds after the minute  0-61*
//tm_min  int minutes after the hour  0-59
//tm_hour int hours since midnight  0-23
//tm_mday int day of the month  1-31
//tm_mon  int months since January  0-11
//tm_year int years since 1900
  Serialprint("###########   SMS Received Notification content: ");
  Serialprintln(smsContent);

  SmsIsReceived = true;

  if (GsmLogic_IsSmsSenderAuthorized(callerNumber))
  {
    GsmLogic_ParseSms(callerNumber, smsTime, smsContent);
  }
  else
  {
    struct SmsForSending smsReceivedNotificationToAdmin;
    smsReceivedNotificationToAdmin.receiverNumber = ADMIN_NUMBER;
    smsReceivedNotificationToAdmin.smsContent = "SMS from unregisterd: " + callerNumber + ", content:[" + smsContent +"]";
    PushSmsIntoSendingQueue(&smsReceivedNotificationToAdmin);
  }

}

void ReceivedRingNotificationFunction(String callerNumber)
{
  // TODO: ovde dodaj poziv funkcije koja ce da obradi ovo (nije primenljivo za Thermostat)
  Serialprint("###########   RING number: ");
  Serialprintln(callerNumber);
  SendHangupCallCommand();
}

void ReceivedOperatorCreditFunction(float credit)
{
  GsmCurrentCreditAcquired = true;
  GsmCurrentCredit = credit;
  Serialprint("###########   Operator CREDIT: ");
  Serialprintln(credit);

  GsmSentLowCreditWarningNumber++;
  if (GsmSentLowCreditWarningNumber <= SEND_LOW_CREDIT_NOTIFICATION_NUMBER && credit < GCM_CREDIT_WARNING_LIMIT)
  {
    struct SmsForSending smsReceivedNotificationToAdmin;
    smsReceivedNotificationToAdmin.receiverNumber = ADMIN_NUMBER;
    smsReceivedNotificationToAdmin.smsContent = "LOW CREDIT\r\n" + GsmLogic_StatusReport();
    PushSmsIntoSendingQueue(&smsReceivedNotificationToAdmin);
  }
  else if (credit >= GCM_CREDIT_WARNING_LIMIT)
  {
    GsmSentLowCreditWarningNumber = 0;
  }
}

void ReceivedGsmTimeFunction(struct tm gsmTime)
{
  // This function will never get called
  Serialprint("###########   Received GSM (converted to loacl time zone) Time: ");
  Serialprint(gsmTime.tm_hour + GSM_TIME_ZONE);
  Serialprint(":");
  Serialprint(gsmTime.tm_min);
  Serialprint(":");
  Serialprint(gsmTime.tm_sec);
  Serialprint(" ");
  Serialprint(gsmTime.tm_mday);
  Serialprint(".");
  Serialprint(gsmTime.tm_mon);
  Serialprint(".");
  Serialprintln(gsmTime.tm_year + 1900);
}

void ReceivedGsmSignalStrengthFunction(int signalStrengthDb)
{
  GsmSignalStrengthDbAcquired = true;
  GsmSignalStrengthDb = signalStrengthDb;

  Serialprint("###########   Signal strength: ");
  Serialprint(signalStrengthDb);
  Serialprintln(" db");
}


void GsmLogicStateMachine(int action)
{
  Serialprint(millis());
  Serialprint(": ");
  Serialprint("GsmSM enter S: ");
  Serialprint(GsmLogicState);
  Serialprint(", A: ");
  Serialprintln(action);
  
  bool hasMoreCommands;
  unsigned long elapsedTime;
  struct SmsForSending smsToPop;
  struct SmsForSending smsForSending;

  switch(GsmLogicState)
  {
    case GLSM_S_INIT:
      switch(action)
      {
        case GLSM_A_2sTick:
          if (CanSendAtCommand())
          {
            InitSendingSimpleInitCommands();
            SendSimpleInitCommand();
            GsmLogicState = GLSM_S_SIMPLE_INIT_AT_SENT;
          }
        break;
      }
    break;

    
    case GLSM_S_ERROR:
      switch(action)
      {
        case GLSM_A_2sTick:
          ResetGsmModem();
          GsmLogic_InternalInit();
          GsmLogicState = GLSM_S_GSM_RESET_WAIT;
          GsmSM_counter = 0;
          SetupIdleParser();
        break;
      }
    break;

    
    case GLSM_S_GSM_RESET_WAIT:
      switch(action)
      {
        case GLSM_A_2sTick:
          GsmSM_counter++;
          if (GsmSM_counter > 10)
          {
            GsmLogicState = GLSM_S_INIT;
            SetupIdleParser();
          }
        break;
      }
    break;

    
    case GLSM_S_SIMPLE_INIT_AT_SENT:
      switch(action)
      {
        case GLSM_A_ParserOK:
          GsmLogicState = GLSM_S_SIMPLE_INIT_AT_PAUSE;
          SetupIdleParser();
          TwoSecondsTimerCounter = 0;        
        break;
        case GLSM_A_ParserError:
        case GLSM_A_ParserTimeout:
          GsmLogicState = GLSM_S_ERROR;
          SetupIdleParser();
        break;
      }
      break;


    
    case GLSM_S_SIMPLE_INIT_AT_PAUSE:
      switch(action)
      {
        case GLSM_A_2sTick:
          if (CanSendAtCommand())
          {
            GsmLogicState = GLSM_S_SIMPLE_INIT_AT_SENT;
            TwoSecondsTimerCounter = 0;
            hasMoreCommands = SendSimpleInitCommand();
            if (!hasMoreCommands)
            {
              GsmLogicState = GLSM_S_IDLE;
              Serialprintln("%%%%%% SM: Entered IDLE");
              SetupIdleParser();
            }
          }
        break;
      }
      break;


    case GLSM_S_GET_CREDIT:
      switch(action)
      {
        case GLSM_A_ParserOK:
          GsmLogicState = GLSM_S_IDLE;
          OperatorsCreditAcquired = true;
          GsmLogic_OperatorsCreditTimeAcquired = millis();
          getOperatorCreditAttempCounterSec = 0;
          SetupIdleParser();
        break;
        case GLSM_A_ParserError:
        case GLSM_A_ParserTimeout:

          GsmLogicState = GLSM_S_IDLE;
          SetupIdleParser();
        break;
      }
    break;


    
    case GLSM_S_GET_SIGNAL_STRENGTH:
      switch(action)
      {
        case GLSM_A_ParserOK:
          GsmLogicState = GLSM_S_IDLE;
          SignalStrengthTimeAcquired = millis();
          SignalStrengthAcquired = true;
          SetupIdleParser();
        break;
        case GLSM_A_ParserError:
        case GLSM_A_ParserTimeout:
          GsmLogicState = GLSM_S_IDLE;
          SetupIdleParser();
        break;
      }
    break;



    case GLSM_S_WAIT_SMS_DELETE:
      switch(action)
      {
        case GLSM_A_ParserOK:
          GsmLogicState = GLSM_S_IDLE;
          SetupIdleParser();
          SmsIsReceived = false;
        break;
        case GLSM_A_ParserError:
        case GLSM_A_ParserTimeout:
          GsmLogicState = GLSM_S_IDLE;
          SetupIdleParser();
        break;
      }
    break;


    
    case GLSM_S_WAIT_SMS_SEND_CONFIRMATION:
      switch(action)
      {
        case GLSM_A_ParserOK:
          GsmLogicState = GLSM_S_IDLE;
          PopSmsFromSendingQueue(&smsToPop);
          SetupIdleParser();
        break;
        case GLSM_A_ParserError:
        case GLSM_A_ParserTimeout:
          GsmLogicState = GLSM_S_IDLE;
          SetupIdleParser();
        break;
      }
    break;




    case GLSM_S_IDLE:
    break;



//    case GLSM_S_:
//      switch(action)
//      {
//        case :
//        break;
//      }
//    break;
    
  }


  // Performing various actions in IDLE state 
  if (GsmLogicState == GLSM_S_IDLE && action == GLSM_A_2sTick && SmsIsReceived && CanSendAtCommand())
  {
    SendDeleteAllStoredSmsCommand();
    GsmLogicState = GLSM_S_WAIT_SMS_DELETE;
  }
  if (GsmLogicState == GLSM_S_IDLE && action == GLSM_A_2sTick && !OperatorsCreditAcquired &&
      getOperatorCreditAttempCounterSec >= GET_OPERATOR_CREDIT_ATTEMPT_INTERVAL_SEC && CanSendAtCommand())
  {
    SendGetOperatorCreditCommand();
    getOperatorCreditAttempCounterSec = 0;
    GsmLogicState = GLSM_S_GET_CREDIT;
  }
  else
  {
    getOperatorCreditAttempCounterSec += 2;
  }
  if (GsmLogicState == GLSM_S_IDLE && action == GLSM_A_2sTick && !SignalStrengthAcquired && CanSendAtCommand())
  {
    SendGetSignalStrengthCommand();
    GsmLogicState = GLSM_S_GET_SIGNAL_STRENGTH;
  }

  if (GsmLogicState == GLSM_S_IDLE && action == GLSM_A_2sTick && CanSendAtCommand() && PeekSmsFromSendingQueue(&smsForSending))
  {
    SendSmsCommand(smsForSending.receiverNumber, smsForSending.smsContent);
    GsmLogicState = GLSM_S_WAIT_SMS_SEND_CONFIRMATION;
  }
  

  elapsedTime = millis() - GsmLogic_OperatorsCreditTimeAcquired;
  if (elapsedTime > GSM_OPERATOR_CREDIT_CHECK_INTERVAL && OperatorsCreditAcquired)  // 12 hours
  {
    OperatorsCreditAcquired = false;
    getOperatorCreditAttempCounterSec = GET_OPERATOR_CREDIT_ATTEMPT_INTERVAL_SEC+1;
  }

  elapsedTime = millis() - SignalStrengthTimeAcquired;
  if (elapsedTime > GSM_SIGNAL_STRENGTH_CHECK_INTERVAL && !SignalStrengthAcquired)  // 1 hour
  {
    SignalStrengthAcquired = false;
  }

  Serialprint(millis());
  Serialprint(": ");
  Serialprint("GsmSM exit S: ");
  Serialprintln(GsmLogicState);
}


bool PushSmsIntoSendingQueue(struct SmsForSending* sms)
{
  for (int i = 0; i < min(sms->receiverNumber.length(), (unsigned int)SMS_MAX_RECEIVER_NUMBER_LENGTH); i++)
  {
    smsForQueue.receiverNumber[i] = sms->receiverNumber[i];
  }
  smsForQueue.receiverNumberLength = min(sms->receiverNumber.length(), (unsigned int)SMS_MAX_RECEIVER_NUMBER_LENGTH);
  for (int i = 0; i < min(sms->smsContent.length(), (unsigned int)SMS_MAX_CONTENT_LENGTH); i++)
  {
    smsForQueue.smsContent[i] = sms->smsContent[i];
  }
  smsForQueue.smsContentLength = min(sms->smsContent.length(), (unsigned int)SMS_MAX_CONTENT_LENGTH);

  return SmsQueue.push(&smsForQueue);
}

bool PeekSmsFromSendingQueue(struct SmsForSending* sms)
{
  bool retValue = SmsQueue.peek(&smsForQueue);
  if (!retValue) return retValue;

  sms->receiverNumber = "";
  for (int i = 0; i < smsForQueue.receiverNumberLength; i++)
  {
    sms->receiverNumber += smsForQueue.receiverNumber[i];
  }
  sms->smsContent = "";
  for (int i = 0; i < smsForQueue.smsContentLength; i++)
  {
    sms->smsContent += smsForQueue.smsContent[i];
  }
  return retValue;
}

bool PopSmsFromSendingQueue(struct SmsForSending* sms)
{
  bool retValue = SmsQueue.pop(&smsForQueue);
  if (!retValue) return retValue;

  sms->receiverNumber = "";
  for (int i = 0; i < smsForQueue.receiverNumberLength; i++)
  {
    sms->receiverNumber += smsForQueue.receiverNumber[i];
  }
  sms->smsContent = "";
  for (int i = 0; i < smsForQueue.smsContentLength; i++)
  {
    sms->smsContent += smsForQueue.smsContent[i];
  }
  return retValue;
}






// SMS parser and report generator code:

bool GsmLogic_IsSmsSenderAuthorized(String callerNumber)
{
  for (int i = 0; i < REGISTRED_NUMBERS_COUNT; i++)
  {
    if (REGISTRED_NUMBERS[i] == callerNumber)
    {
      return true;
    }
  }
  return false;
}


#define COMMAND_STATUS "STATUS"
#define COMMAND_OPEN "OPEN"
#define COMMAND_CLOSE "CLOSE"
#define COMMAND_SEND_SMS "SENDSMS"
#define COMMAND_RESET_MODULE "RESET"

//#define COMMANDS_COUNT 4
//const String COMMANDS[COMMANDS_COUNT] = {
//    COMMAND_STATUS,
//    COMMAND_OPEN,
//    COMMAND_CLOSE,
//    COMMAND_SEND_SMS
//};


String GsmLogic_CorrectCommandFormat()
{
  return "Invalid SMS, Correct command format: \r\n" \
        "status\r\n" \
        "open\r\n" \
        "close\r\n" \
        "sendsms";
}

const String GsmLogic_StatusTemplate = 
  "Current status:\r\n"\
  "Door: %DOOR_OPENED_CLOSED%\r\n"\
  "Current temp 1: %CURRENT_TEMP_SENSOR_1% C\r\n"\
  "Current temp 2: %CURRENT_TEMP_SENSOR_2% C\r\n"\
  "Current temp 3: %CURRENT_TEMP_SENSOR_3%";

String GsmLogic_StatusReport()
{
  String smsContent = GsmLogic_StatusTemplate;

//  smsContent.replace(WIFI_STATUS_FIELD_0, WiFiStatusPart0());
//  smsContent.replace(WIFI_STATUS_FIELD_1, WiFiStatusPart1());
//  smsContent.replace(HEATING_FIELD, (hcManualEnabled || hcAutoEnabled) ? "ENABLED" : "DISABLED");

  return smsContent;
}

void GsmLogic_ParseStatusCommand(String& callerNumber, struct tm& smsTime, String& smsContent)
{
  struct SmsForSending smsToAdmin;
  smsToAdmin.receiverNumber = callerNumber;
  smsToAdmin.smsContent = GsmLogic_StatusReport();
  PushSmsIntoSendingQueue(&smsToAdmin);
}

void GsmLogic_ParseSendSmsCommand(String& callerNumber, struct tm& smsTime, String& smsContent)
{
//    SendSms,<receiverNo>,<smsContent>     -> send sms, like sendsms,+381642023960,proba za slanje sms
  struct SmsForSending smsToAdmin;
  int firstCommaIndex = smsContent.indexOf(',');
  int secondCommaIndex = smsContent.indexOf(',', firstCommaIndex + 1);
  if (firstCommaIndex == -1 || secondCommaIndex == -1)
  {
    smsToAdmin.receiverNumber = callerNumber;
    smsToAdmin.smsContent = GsmLogic_CorrectCommandFormat();
    PushSmsIntoSendingQueue(&smsToAdmin);
    return;
  }

  String smsReceiverNumber = smsContent.substring(firstCommaIndex + 1, secondCommaIndex);
  String smsContentToSend = smsContent.substring(secondCommaIndex + 1);
  smsToAdmin.receiverNumber = smsReceiverNumber;
  smsToAdmin.smsContent = smsContentToSend;
  PushSmsIntoSendingQueue(&smsToAdmin);
}

void GsmLogic_ParseResetModuleCommandCommand(String& callerNumber, struct tm& smsTime, String& smsContent)
{
  while(1);
  // ESP32 will restart in 16 seconds on WDT reset
}

void GsmLogic_ParseSms(String& callerNumber, struct tm& smsTime, String& smsContent)
{
  int firstCommaIndex = smsContent.indexOf(',');
  String command = smsContent.substring(0, firstCommaIndex);
  command.toUpperCase();
  if (command == COMMAND_STATUS)
  {
    GsmLogic_ParseStatusCommand(callerNumber, smsTime, smsContent);
  }
  else if (command == COMMAND_OPEN)
  {
    //GsmLogic_ParseManualOnCommand(callerNumber, smsTime, smsContent);
  }
  else if (command == COMMAND_CLOSE)
  {
    //GsmLogic_ParseManualOffCommand(callerNumber, smsTime, smsContent);
  }
  else if (command == COMMAND_SEND_SMS)
  {
    GsmLogic_ParseSendSmsCommand(callerNumber, smsTime, smsContent);
  }
  else if (command == COMMAND_RESET_MODULE)
  {
    GsmLogic_ParseResetModuleCommandCommand(callerNumber, smsTime, smsContent);
  }
  else
  {
    struct SmsForSending smsToAdmin;
    smsToAdmin.receiverNumber = callerNumber;
    smsToAdmin.smsContent = GsmLogic_CorrectCommandFormat();
    PushSmsIntoSendingQueue(&smsToAdmin);
  }
  
}


#endif
