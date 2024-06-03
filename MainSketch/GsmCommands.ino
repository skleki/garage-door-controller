#ifdef GSM_MODEM

#include <functional> 

#define GSM_RECEIVE_TIMEOUT_SEC 120

String OK_RESPONSE = "\r\nOK\r\n";
String ERROR_RESPONSE = "ERROR";
String GET_CREDIT_FINAL_RESPONSE = "\r\n+CUSD: 2\r\n";
String SMS_NOTIFICATION_START = "\r\n+CMT: \"";
String RING_NOTIFICATION_START = "\r\nRING\r\n\r\n+CLIP: \"";
String SIGNAL_STRENGTH_RESPONSE_START = "\r\n+CSQ: ";
String SMS_SEND_RESPONSE_START = "\r\n+CMGS:";
String BUSY_RESPONSE = "\r\nBUSY\r\n";
String NOCARRIER_RESPONSE = "\r\nNO CARRIER\r\n";

String GET_OPERATOR_CREDIT_COMMAND ="AT+CUSD=1,\"*100#\"\r";

int INIT_COMMANDS_LENGTH = 7;
String INIT_COMMANDS[] = {
"AT\r",
"AT+CMEE=2\r",
"AT+CNMI=2,2,0,1,0\r",
"AT+CMGF=1\r",
"AT+CPMS=\"SM\",\"SM\",\"SM\"\r",
"AT+CMGD=1,4\r",
"AT&W\r"
};
int GsmCommandsCurrentInitCommandIndex;


int TIME_COMMANDS_LENGTH = 7;
String TIME_COMMANDS[] = {
"AT\r",
"AT+CGATT=1\r",
"AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r",
"AT+SAPBR=3,1,\"APN\",\"\"\r",
"AT+SAPBR=3,1,\"USER\",\"\"\r",
"AT+SAPBR=3,1,\"PWD\",\"\"\r",
"AT+SAPBR=1,1\r"
};
int GsmCommandsCurrentTimeCommandIndex;

String READ_GSM_TIME_COMMAND = "AT+CIPGSMLOC=2,1\r";
String GET_SIGNAL_STRENGTH_COMMAND = "AT+CSQ\r";
String DELETE_ALL_STORED_SMS_COMMAND = "AT+CMGD=1,4\r";
String HANGUP_CALL_COMMAND = "ATH\r";


String _newArrivedData;
String _dataToProcess;
String _lastCommandSent;
int GsmReceiveTimeoutCounter = 0;


typedef std::function<void(String, struct tm, String)> TReceivedSmsNotificationFunction;
TReceivedSmsNotificationFunction _receivedSmsNotificationFunction;

typedef std::function<void(String)> TReceivedRingNotificationFunction;
TReceivedRingNotificationFunction _receivedRingNotificationFunction;

typedef std::function<void(float)> TReceivedOperatorCreditFunction;
TReceivedOperatorCreditFunction _receivedOperatorCreditFunction;

typedef std::function<void(struct tm)> TReceivedTimeFunction;
TReceivedTimeFunction _receivedGsmTimeFunction;

typedef std::function<void(int)> TReceivedSignalStrengthFunction;
TReceivedSignalStrengthFunction _receivedSignalStrengthFunction;


typedef std::function<bool(bool)> TReceivedDataParserFunction;
TReceivedDataParserFunction _currentParserFunction;
unsigned long  _currentParserTimeoutMs;
unsigned long  _currentParserStartTimestamp;

#define GsmParserOK 0
#define GsmParserERROR 1
#define GsmParserTimeout 2
typedef std::function<void(int)> TParserCallbackNotification;
TParserCallbackNotification _parserCallbackNotification;


// Message prototypes
inline bool FindSmsNotification(String& input, int& foundEndIndex, String& callerNumber, struct tm& smsTime, String& smsContent);
inline bool FindRingNotification(String& input, int& foundEndIndex, String& callerNumber);
inline void SetupNonIdleParser(TReceivedDataParserFunction parserFunction, int timeout);
inline void SetupIdleParser();
void SendGsmCommand(String command);
void ResetGsmModem();


#define SIM800L_RX_PIN 16
#define SIM800L_TX_PIN 17
#define SIM800L_RESET_PIN 4


// ****************************************************************************************
void GsmCommands_Initialise(TParserCallbackNotification parserCallbackNotification,
                            TReceivedSmsNotificationFunction receivedSmsNotificationFunction,
                            TReceivedRingNotificationFunction receivedRingNotificationFunction,
                            TReceivedOperatorCreditFunction receivedOperatorCreditFunction,
                            TReceivedTimeFunction receivedGsmTimeFunction,
                            TReceivedSignalStrengthFunction receivedSignalStrengthFunction)
{
  _newArrivedData = "";
  _lastCommandSent = "";
  SetupIdleParser();

  _parserCallbackNotification = parserCallbackNotification;
  _receivedSmsNotificationFunction = receivedSmsNotificationFunction;
  _receivedRingNotificationFunction = receivedRingNotificationFunction;
  _receivedOperatorCreditFunction = receivedOperatorCreditFunction;
  _receivedGsmTimeFunction = receivedGsmTimeFunction;
  _receivedSignalStrengthFunction = receivedSignalStrengthFunction;

  ResetPinLow();
  pinMode(SIM800L_RESET_PIN, OUTPUT);

  ResetGsmModem();
  Serial2.begin(9600, SERIAL_8N1, SIM800L_RX_PIN, SIM800L_TX_PIN);
}

void ResetPinHigh()
{
  digitalWrite(SIM800L_RESET_PIN, HIGH);
}
void ResetPinLow()
{
  digitalWrite(SIM800L_RESET_PIN, LOW);
}
void ResetGsmModem()
{
  ResetPinHigh();
  delay(1000);
  ResetPinLow();
}

void GsmCommands_loop(void)
{
  _newArrivedData = "";
  while (Serial2.available())
  {
    _newArrivedData += char(Serial2.read());
  }
  
  if (_newArrivedData.length() > 0)
  {
    Serialprint(millis());
    Serialprint(": ");
    Serialprint("Received Serial port DATA=[");
    Serialprint(_newArrivedData);
    Serialprintln("]");
  
    GsmReceiveTimeoutCounter = 0;
    _dataToProcess += _newArrivedData;
    _newArrivedData = "";

    _currentParserFunction(false);
  }
}

void GsmCommands_100msHandler(void)
{
  if (_currentParserTimeoutMs > 0)
  {
    unsigned long elapsedTime = millis() - _currentParserStartTimestamp;
    if (elapsedTime > _currentParserTimeoutMs)
    {
//      Serialprint(millis());
//      Serialprint(": ");
//      Serialprint("GsmCommands_100msHandler => GsmParserTimeout _currentParserStartTimestamp: ");
//      Serialprint(_currentParserStartTimestamp);
//      Serialprint(", _currentParserTimeoutMs: ");
//      Serialprint(_currentParserTimeoutMs);
//      Serialprint(", elapsedTime: ");
//      Serialprint(elapsedTime);
//      Serialprint(", millis()");
//      Serialprintln(millis());

      Serialprint(millis());
      Serialprint(": ");
      Serialprint("Timeouted on command: ");
      Serialprintln(_lastCommandSent);
      
      _currentParserFunction(true);
      _parserCallbackNotification(GsmParserTimeout);
    }
  }
}

void GsmCommands_1sHandler(void)
{
  GsmReceiveTimeoutCounter++;
  if (GsmReceiveTimeoutCounter > GSM_RECEIVE_TIMEOUT_SEC && _dataToProcess != "")
  {
    Serialprint("Deleting receive buffer after 120 sec. Buffer:[");
    Serialprint(_dataToProcess);
    Serialprintln("]");
    GsmReceiveTimeoutCounter = 0;
    _dataToProcess = "";
  }
}

void SendGsmCommand(String command)
{
  Serialprint(millis());
  Serialprint(": ");
  Serialprint("Sending GsmCommand: [");  
  Serialprint(command);  
  Serialprintln("]");  
  Serial2.print(command);
  _lastCommandSent = command;
}



inline void SetupIdleParser()
{
  _currentParserFunction = ParseDataOnIdleFunction;
  _currentParserTimeoutMs = 0;
}

inline void SetupNonIdleParser(TReceivedDataParserFunction parserFunction, int timeout)
{
  _currentParserFunction = parserFunction;
  _currentParserTimeoutMs = timeout;
  _currentParserStartTimestamp = millis();

//  Serialprint(millis());
//  Serialprint(": ");
//  Serialprint("SetupNonIdleParser => _currentParserStartTimestamp: ");
//  Serialprint(_currentParserStartTimestamp);
//  Serialprint(", _currentParserTimeoutMs: ");
//  Serialprintln(_currentParserTimeoutMs);
}

inline bool CanSendAtCommand()
{
  // Only in IDLE mode, we do not expect any command response and can send a new command
  return _currentParserTimeoutMs == 0;
}

inline void InitSendingSimpleInitCommands()
{
  GsmCommandsCurrentInitCommandIndex = 0;
}

inline bool SendSimpleInitCommand()
{
  if (GsmCommandsCurrentInitCommandIndex >= INIT_COMMANDS_LENGTH) return false;
  SetupNonIdleParser(ParseCommandWithSimpleResponseFunction, 5000);
  SendGsmCommand(INIT_COMMANDS[GsmCommandsCurrentInitCommandIndex]);
  GsmCommandsCurrentInitCommandIndex++;
  return true;
}

inline void InitSendingSimpleTimeCommands()
{
  GsmCommandsCurrentTimeCommandIndex = 0;
}

inline bool SendSimpleTimeCommand()
{
  if (GsmCommandsCurrentTimeCommandIndex >= TIME_COMMANDS_LENGTH) return false;
  SetupNonIdleParser(ParseCommandWithSimpleResponseFunction, 60000);
  SendGsmCommand(TIME_COMMANDS[GsmCommandsCurrentTimeCommandIndex]);
  GsmCommandsCurrentTimeCommandIndex++;
  return true;
}


// IDLE parser function, can not timeout
bool ParseDataOnIdleFunction(bool isTimeout)
{
//  Serialprint("Received Parser STARTING to proces data, remained data:[");
//  Serialprint(_dataToProcess);
//  Serialprintln("]");

  int foundEndIndex;
  String callerNumber;
  struct tm smsTime;
  String smsContent;
  if (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent))
  {
    _receivedSmsNotificationFunction(callerNumber, smsTime, smsContent);
    _dataToProcess = _dataToProcess.substring(foundEndIndex);
  }
  if (FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
  {
    _receivedRingNotificationFunction(callerNumber);
    _dataToProcess = _dataToProcess.substring(foundEndIndex);
  }

//  Serialprint(millis());
//  Serialprint(": ");
//  Serialprint("Received Parser finished processing data, remained data:[");
//  Serialprint(_dataToProcess);
//  Serialprintln("]");
  
  return false;
}

// Simple response parser function
bool ParseCommandWithSimpleResponseFunction(bool isTimeout)
{
  int foundEndIndex;
  String callerNumber;
  struct tm smsTime;
  String smsContent;

  if (isTimeout)
  {
    while (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent) ||
           FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
    {
      if (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent))
      {
        _receivedSmsNotificationFunction(callerNumber, smsTime, smsContent);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
      if (FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
      {
        _receivedRingNotificationFunction(callerNumber);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
    }
    _dataToProcess = "";
    return false;
  }
  
  int foundStartIndex;
  if (FindString(_dataToProcess, OK_RESPONSE, foundStartIndex))
  {
    _parserCallbackNotification(GsmParserOK);
    _dataToProcess = _dataToProcess.substring(foundStartIndex + OK_RESPONSE.length());
  }
  else if (FindString(_dataToProcess, ERROR_RESPONSE, foundStartIndex))
  {
    _parserCallbackNotification(GsmParserERROR);
    _dataToProcess = _dataToProcess.substring(foundStartIndex + ERROR_RESPONSE.length());
  }

  return false;
}


bool ParseGsmTimeResponseFunction(bool isTimeout)
{
  int foundEndIndex;
  String callerNumber;
  struct tm smsTime;
  String smsContent;

  if (isTimeout)
  {
    while (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent) ||
           FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
    {
      if (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent))
      {
        _receivedSmsNotificationFunction(callerNumber, smsTime, smsContent);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
      if (FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
      {
        _receivedRingNotificationFunction(callerNumber);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
    }
    _dataToProcess = "";
    return false;
  }
  
  // Respoonse(after 2 sec approx): 
  // {0D}{0A}+CIPGSMLOC: 0,2021/12/01,19:34:55{0D}{0A}{0D}{0A}OK{0D}{0A}
  String responseStart = "\r\n+CIPGSMLOC: ";

  int foundStartIndex;
  if (FindString(_dataToProcess, OK_RESPONSE, foundStartIndex))
  {
    int startGsmDateTimeResponse = _dataToProcess.indexOf(responseStart);
    int firstCommaIndex = _dataToProcess.indexOf(',', startGsmDateTimeResponse + 1);
    if (firstCommaIndex != -1)
    {
      String statusNumberAsString = _dataToProcess.substring(startGsmDateTimeResponse + responseStart.length(), firstCommaIndex);
      int statusNumber = statusNumberAsString.toInt();
      if (statusNumber != 0)
      {
        Serialprint(millis());
        Serialprint(": ");
        Serialprint("Command AT+CIPGSMLOC=2,1 returned error: ");
        Serialprintln(statusNumberAsString);
        _parserCallbackNotification(GsmParserERROR);
        return false;
      }
    }
   
    int secondCommaIndex = _dataToProcess.indexOf(',', firstCommaIndex + 1);
    int timeEndIndex = _dataToProcess.indexOf('\r', secondCommaIndex + 1);
    if (secondCommaIndex == -1 || timeEndIndex == -1)
    {
      Serialprint(millis());
      Serialprint(": ");
      Serialprint("FAILED to extract Date and Time from buffer: ");
      Serialprintln(_dataToProcess);
      return false;
    }
    
    String _date = _dataToProcess.substring(firstCommaIndex + 1, secondCommaIndex);
    String _time = _dataToProcess.substring(secondCommaIndex + 1, timeEndIndex);
//    Serialprint(millis());
//    Serialprint(": ");
//    Serialprint("%%%%%%%%%%%%%% Extracted Date: ");
//    Serialprint(_date);
//    Serialprint("Extracted Time: ");
//    Serialprintln(_time);
    
    int firstSlash = _date.indexOf('/');    
    int secondSlash = _date.indexOf('/', firstSlash + 1);
    if (firstSlash == -1 || secondSlash== -1)
    {
      Serialprint(millis());
      Serialprint(": ");
      Serialprint("FAILED to extract Date from: ");
      Serialprintln(_date);
      _parserCallbackNotification(GsmParserERROR);
      _dataToProcess = _dataToProcess.substring(foundStartIndex + OK_RESPONSE.length());
      return false;     
    }
    int _year = _date.substring(0, firstSlash).toInt();
    int _month = _date.substring(firstSlash + 1, secondSlash).toInt();
    int _day = _date.substring(secondSlash + 1).toInt();
//    Serialprint(millis());
//    Serialprint(": ");
//    Serialprint("%%%%%%%%%%%%%% Extracted Date: Day:");
//    Serialprint(_day);
//    Serialprint(", Month: ");
//    Serialprint(_month);
//    Serialprint(", Year: ");
//    Serialprintln(_year);

    // 19:34:55
    int firstColon = _time.indexOf(':');
    int secondColon = _time.indexOf(':', firstColon + 1);
    if (firstColon == -1 || secondColon == -1)
    {
      Serialprint(millis());
      Serialprint(": ");
      Serialprint("FAILED to extract Time from: ");
      Serialprintln(_time);
      _parserCallbackNotification(GsmParserERROR);
      _dataToProcess = _dataToProcess.substring(foundStartIndex + OK_RESPONSE.length());
      return false;     
    }
    int _hour = _time.substring(0, firstColon).toInt();    
    int _min = _time.substring(firstColon + 1, secondColon).toInt();    
    int _sec = _time.substring(secondColon + 1).toInt();    
//    Serialprint(millis());
//    Serialprint(": ");
//    Serialprint("%%%%%%%%%%%%%% Extracted Time: ");
//    Serialprint(_hour);
//    Serialprint(":");
//    Serialprint(_min);
//    Serialprint(":");
//    Serialprintln(_sec);

    struct tm receivedTime;
    receivedTime.tm_sec = _sec;
    receivedTime.tm_min = _min;
    receivedTime.tm_hour = _hour;
    receivedTime.tm_mday = _day;
    receivedTime.tm_mon = _month;
    receivedTime.tm_year = _year - 1900;
    //tm_sec  int seconds after the minute  0-61*
    //tm_min  int minutes after the hour  0-59
    //tm_hour int hours since midnight  0-23
    //tm_mday int day of the month  1-31
    //tm_mon  int months since January  0-11
    //tm_year int years since 1900
    _receivedGsmTimeFunction(receivedTime);

    while (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent) ||
           FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
    {
      if (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent))
      {
        _receivedSmsNotificationFunction(callerNumber, smsTime, smsContent);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
      if (FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
      {
        _receivedRingNotificationFunction(callerNumber);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
    }

    _parserCallbackNotification(GsmParserOK);
    _dataToProcess = "";
  }
  else if (FindString(_dataToProcess, ERROR_RESPONSE, foundStartIndex))
  {
    _parserCallbackNotification(GsmParserERROR);
    _dataToProcess = _dataToProcess.substring(foundStartIndex + ERROR_RESPONSE.length());
  }

  return false;
}

inline void SendGetGsmTimeCommand()
{
  SetupNonIdleParser(ParseGsmTimeResponseFunction, 60000);
  SendGsmCommand(READ_GSM_TIME_COMMAND);
}



bool ParseGetOperatorCreditResponseFunction(bool isTimeout)
{
  int foundEndIndex;
  String callerNumber;
  struct tm smsTime;
  String smsContent;
  String creditStartMarker = "Kredit:";

  if (isTimeout)
  {
    // Try to extract Kredit
    int startCreditIndex = _dataToProcess.indexOf(creditStartMarker);
    int endCreditIndex = _dataToProcess.indexOf("din");
    if (startCreditIndex == -1 || endCreditIndex == -1)
    {
//      Serialprint("ParseGetOperatorCreditResponseFunction UNABLE to find Kredit value: ");
//      Serialprintln(_dataToProcess);
//      Serialprint("startCreditIndex: ");
//      Serialprint(startCreditIndex);
//      Serialprint(", endCreditIndex: ");
//      Serialprintln(endCreditIndex);
      return false;
    }   

    String creditAsString = _dataToProcess.substring(startCreditIndex + creditStartMarker.length() + 1, endCreditIndex);
    float credit = creditAsString.toFloat();  
//    Serialprint("ParseGetOperatorCreditResponseFunction extracting Kredit value: ");
//    Serialprintln(creditAsString);
//    Serialprint("ParseGetOperatorCreditResponseFunction EXTRACTED Kredit value: ");
//    Serialprintln(credit);
    _receivedOperatorCreditFunction(credit);

    while (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent) ||
           FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
    {
      if (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent))
      {
        _receivedSmsNotificationFunction(callerNumber, smsTime, smsContent);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
      if (FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
      {
        _receivedRingNotificationFunction(callerNumber);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
    }
    _dataToProcess = "";
    return false;
  }

// Response je u formatu: Response: {0D}{0A}OK{0D}{0A}{0D}{0A}+CUSD: 1,"Kredit: 352.6 din.{0A}{0A}{0A}1. Bonus dobrodoslice{0A}2. KOMBO{0A}3. MIX{0A}4. Internet{0A}5. Drustvene mreze{0A}6. Roming{0A}7. Dodatno{0A}8. Stanje{0A}", 15{0D}{0A}{0D}{0A}+CUSD: 2{0D}{0A}
// Pri cemu se {0D}{0A}+CUSD: 2{0D}{0A} javlja posle nekih 40-tak sekundi i treba da se saceka ali za to vreme moze da stize RING i SMS Notifications
  int foundStartIndex;
  if (FindString(_dataToProcess, GET_CREDIT_FINAL_RESPONSE, foundStartIndex))
  {
    int startCreditIndex = _dataToProcess.indexOf(creditStartMarker);
    int endCreditIndex = _dataToProcess.indexOf("din");
    if (startCreditIndex == -1 || endCreditIndex == -1)
    {
//      Serialprint("ParseGetOperatorCreditResponseFunction UNABLE to find Kredit value: ");
//      Serialprintln(_dataToProcess);
//      Serialprint("startCreditIndex: ");
//      Serialprint(startCreditIndex);
//      Serialprint(", endCreditIndex: ");
//      Serialprintln(endCreditIndex);
      return false;
    }   

    String creditAsString = _dataToProcess.substring(startCreditIndex + creditStartMarker.length() + 1, endCreditIndex);
    float credit = creditAsString.toFloat();  
//    Serialprint("ParseGetOperatorCreditResponseFunction extracting Kredit value: ");
//    Serialprintln(creditAsString);
//    Serialprint("ParseGetOperatorCreditResponseFunction EXTRACTED Kredit value: ");
//    Serialprintln(credit);
    _receivedOperatorCreditFunction(credit);

    // since this is a lengthy operqation, check for SMS and RING notifications once finished
    while (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent) ||
           FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
    {
      if (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent))
      {
        _receivedSmsNotificationFunction(callerNumber, smsTime, smsContent);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
      if (FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
      {
        _receivedRingNotificationFunction(callerNumber);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
    }
    
    _parserCallbackNotification(GsmParserOK);
    _dataToProcess = "";
  }  
  else if (FindString(_dataToProcess, ERROR_RESPONSE, foundStartIndex))
  {
    _parserCallbackNotification(GsmParserERROR);
    _dataToProcess = _dataToProcess.substring(foundStartIndex + ERROR_RESPONSE.length());
  }
//  else
//  {
//    Serialprint("ParseGetOperatorCreditResponseFunction UNABLE to get final response from: ");
//    Serialprintln(_dataToProcess);
//  }
  return false;
}

inline void SendGetOperatorCreditCommand()
{
  SetupNonIdleParser(ParseGetOperatorCreditResponseFunction, 120000);
  SendGsmCommand(GET_OPERATOR_CREDIT_COMMAND);
}


bool ParseGetSignalStrengthResponseFunction(bool isTimeout)
{
//    +CSQ: <rssi>,<ber>Parameters
//    <rssi>
//    0 -115 dBm or less
//    1 -111 dBm
//    2...30 -110... -54 dBm
//    31 -52 dBm or greater
//    99 not known or not detectable
//    <ber> (in percent):
//    0...7 As RXQUAL values in the table in GSM 05.08 [20]
//    subclause 7.2.4
//    99 Not known or not detectable
//    */
//      Response: {0D}{0A}+CSQ: 26,0{0D}{0A}{0D}{0A}OK{0D}{0A}
  
  int foundEndIndex;
  String callerNumber;
  struct tm smsTime;
  String smsContent;
  if (isTimeout)
  {
    while (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent) ||
           FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
    {
      if (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent))
      {
        _receivedSmsNotificationFunction(callerNumber, smsTime, smsContent);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
      if (FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
      {
        _receivedRingNotificationFunction(callerNumber);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
    }
    _dataToProcess = "";
    return false;
  }

  int foundStartIndex;
  if (FindString(_dataToProcess, OK_RESPONSE, foundStartIndex))
  {
    int startIndex = _dataToProcess.indexOf(SIGNAL_STRENGTH_RESPONSE_START);
    int commaIndex = _dataToProcess.indexOf(",");
    if (startIndex == -1 || commaIndex == -1)
    {
      Serialprint("ParseGetSignalStrengthResponseFunction UNABLE to find signal strength value: ");
      Serialprintln(_dataToProcess);
      Serialprint("startIndex: ");
      Serialprint(startIndex);
      Serialprint(", commaIndex: ");
      Serialprintln(commaIndex);
      _dataToProcess = _dataToProcess.substring(foundStartIndex + OK_RESPONSE.length());
      return false;
    }   

    String signalStrengthAsString = _dataToProcess.substring(startIndex + SIGNAL_STRENGTH_RESPONSE_START.length(), commaIndex);
    int signalStrengthExtracted = signalStrengthAsString.toFloat();  
    Serialprint("ParseGetSignalStrengthResponseFunction extracting Signal strength value: ");
    Serialprintln(signalStrengthAsString);
    Serialprint("ParseGetSignalStrengthResponseFunction EXTRACTED value: ");
    Serialprintln(signalStrengthExtracted);

//    2...30 -110... -54 dBm
//    31 -52 dBm or greater
//    99 not known or not detectable
    int signalStrength = -115;
    if (signalStrengthExtracted == 1) 
    {
      signalStrength = -111;
    }
    else if (signalStrengthExtracted >= 2 and signalStrengthExtracted <= 30)
    {
      signalStrength = -110 + (110 - 54) * (signalStrengthExtracted - 2) / (30 - 2) ;
    }
    else if (signalStrengthExtracted == 31)
    {
      signalStrength = -52;
    }
    else if (signalStrengthExtracted == 99)
    {
      signalStrength = 0;
    }

    _receivedSignalStrengthFunction(signalStrength);
    _parserCallbackNotification(GsmParserOK);
    _dataToProcess = "";
  }  
  else if (FindString(_dataToProcess, ERROR_RESPONSE, foundStartIndex))
  {
    _parserCallbackNotification(GsmParserERROR);
    _dataToProcess = _dataToProcess.substring(foundStartIndex + ERROR_RESPONSE.length());
  }
  return false;
}

inline void SendGetSignalStrengthCommand()
{
  SetupNonIdleParser(ParseGetSignalStrengthResponseFunction, 5000);
  SendGsmCommand(GET_SIGNAL_STRENGTH_COMMAND);
}

inline void SendDeleteAllStoredSmsCommand()
{
  SetupNonIdleParser(ParseCommandWithSimpleResponseFunction, 5000);
  SendGsmCommand(DELETE_ALL_STORED_SMS_COMMAND);
}

// This command is not waiting for any response
inline void SendHangupCallCommand()
{
  SendGsmCommand(HANGUP_CALL_COMMAND);
}


bool ParseSendSmsResponseFunction(bool isTimeout)
{
  int foundEndIndex;
  String callerNumber;
  struct tm smsTime;
  String smsContent;
  if (isTimeout)
  {
    while (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent) ||
           FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
    {
      if (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent))
      {
        _receivedSmsNotificationFunction(callerNumber, smsTime, smsContent);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
      if (FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
      {
        _receivedRingNotificationFunction(callerNumber);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
    }
    _dataToProcess = "";
    return false;
  }

  // Response je u formatu: Response: {0D}{0A}+CMGS: 7{0D}{0A}{0D}{0A}OK{0D}{0A}
  int foundStartIndex;
  int foundOKresponseStartIndex;
  if (FindString(_dataToProcess, SMS_SEND_RESPONSE_START, foundStartIndex) &&
      FindString(_dataToProcess, OK_RESPONSE, foundOKresponseStartIndex))
  {
    // since this is a lengthy operqation, check for SMS and RING notifications once finished
    while (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent) ||
           FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
    {
      if (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent))
      {
        _receivedSmsNotificationFunction(callerNumber, smsTime, smsContent);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
      if (FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
      {
        _receivedRingNotificationFunction(callerNumber);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
    }
    
    _parserCallbackNotification(GsmParserOK);
    _dataToProcess = "";
  }  
  else if (FindString(_dataToProcess, ERROR_RESPONSE, foundStartIndex))
  {
    _parserCallbackNotification(GsmParserERROR);
    _dataToProcess = _dataToProcess.substring(foundStartIndex + ERROR_RESPONSE.length());
  }
  return false;
}

inline void SendSmsCommand(String receiverNumber, String content)
{
  SetupNonIdleParser(ParseSendSmsResponseFunction, 60000);
  String smsSendCommand = "AT+CMGS=\"" + receiverNumber + "\"\r";
  SendGsmCommand(smsSendCommand);
  delay(500);
  Serial2.print(content);
  Serialprint(millis());
  Serialprint(": ");
  Serialprint("Sending SMS content: [");  
  Serialprint(content);  
  Serialprintln("]");  
  Serial2.print((String)((char)0x1A));
  Serialprint(millis());
  Serialprint(": ");
  Serialprint("Sending SMS terminator Ctrl+Z: [");  
  Serialprint((String)((char)0x1A));  
  Serialprintln("]");  
}







bool ParseCallingResponseFunction(bool isTimeout)
{
  int foundEndIndex;
  String callerNumber;
  struct tm smsTime;
  String smsContent;
  if (isTimeout)
  {
    while (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent) ||
           FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
    {
      if (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent))
      {
        _receivedSmsNotificationFunction(callerNumber, smsTime, smsContent);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
      if (FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
      {
        _receivedRingNotificationFunction(callerNumber);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
    }
    _dataToProcess = "";
    return false;
  }

  //String BUSY_RESPONSE = "\r\nBUSY\r\n";
  //String NOCARRIER_RESPONSE = "\r\nNO CARRIER\r\n";
  //  Response kad se odbije poziv: {0D}{0D}{0A}OK{0D}{0A} <i kad korisnik prekine> {0D}{0A}BUSY{0D}{0A}
  //  Response kad se prihvati pa prekine poziv: {0D}{0D}{0A}OK{0D}{0A} <onda se korisnik javi i kad prekine> {0D}{0A}NO CARRIER{0D}{0A}
  int foundStartIndex;
  int foundOKresponseStartIndex;
  if (FindString(_dataToProcess, OK_RESPONSE, foundOKresponseStartIndex) &&
      (FindString(_dataToProcess, BUSY_RESPONSE, foundStartIndex) || FindString(_dataToProcess, NOCARRIER_RESPONSE, foundStartIndex)))
  {
    // since this is a lengthy operqation, check for SMS and RING notifications once finished
    while (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent) ||
           FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
    {
      if (FindSmsNotification(_dataToProcess, foundEndIndex, callerNumber, smsTime, smsContent))
      {
        _receivedSmsNotificationFunction(callerNumber, smsTime, smsContent);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
      if (FindRingNotification(_dataToProcess, foundEndIndex, callerNumber))
      {
        _receivedRingNotificationFunction(callerNumber);
        _dataToProcess = _dataToProcess.substring(foundEndIndex);
      }
    }
    
    _parserCallbackNotification(GsmParserOK);
    _dataToProcess = "";
  }  
  else if (FindString(_dataToProcess, ERROR_RESPONSE, foundStartIndex))
  {
    _parserCallbackNotification(GsmParserERROR);
    _dataToProcess = _dataToProcess.substring(foundStartIndex + ERROR_RESPONSE.length());
  }
  return false;
}

inline void SendCallCommand(String receiverNumber)
{
  SetupNonIdleParser(ParseCallingResponseFunction, 60000);
  String callCommand = "ATD" + receiverNumber + ";\r";
  SendGsmCommand(callCommand);
}







inline bool FindString(String& input, String& pattern, int& foundStartIndex)
{
  return (foundStartIndex = input.indexOf(pattern)) != -1;
}

inline bool FindString(String& input, String& pattern, int& foundStartIndex, int searchFromIndex)
{
  return (foundStartIndex = input.indexOf(pattern, searchFromIndex)) != -1;
}

inline bool FindSmsNotification(String& input, int& foundEndIndex, String& callerNumber, struct tm& smsTime, String& smsContent)
{
  // This settings use "AT+CNMI=2,2,0,1,0\r" configuration for SMS notifications which spill entire 
  // SMS into the Terminal in format:
  // {0D}{0A}+CMT: "+381642023960","","21/12/04,13:21:48+04"{0D}{0A}Proba slanja poruke, red1{0A}Red2{0A}Red3{0A}{0A}Red5{0A}.{0D}{0A}

  
// SMS_NOTIFICATION_START = "\r\n+CMT: \"";  
  int smsNotificationStartIndex = input.indexOf(SMS_NOTIFICATION_START);
  if (smsNotificationStartIndex == -1) return false;
  int smsNotificationFirst0D0A = input.indexOf("\r\n", smsNotificationStartIndex + SMS_NOTIFICATION_START.length());
  if (smsNotificationFirst0D0A == -1) return false;
  int smsNotificationSecond0D0A = input.indexOf("\r\n", smsNotificationFirst0D0A + 2);
  if (smsNotificationSecond0D0A == -1) return false;
  // At this point, the entire SMS notification should be received
//  String oneSmsNotification = input.substring(smsNotificationStartIndex, smsNotificationSecond0D0A + 2);
//  Serialprint("!!!!!!!!!!!!!!!!!!!!!!! Received SMS Notification: ");
//  Serialprintln(oneSmsNotification);
  int smsNotificationCallerNumberEndQuoteIndex = input.indexOf('\"', smsNotificationStartIndex + SMS_NOTIFICATION_START.length() + 1);
  if (smsNotificationCallerNumberEndQuoteIndex == -1) return false;
  int smsNotificationFirstCommaIndex = input.indexOf(',', smsNotificationStartIndex + SMS_NOTIFICATION_START.length() + 1);
  if (smsNotificationFirstCommaIndex == -1) return false;
  int smsNotificationSecondCommaIndex = input.indexOf(',', smsNotificationFirstCommaIndex + 1);
  if (smsNotificationSecondCommaIndex == -1) return false;
  String callerNumberExtracted = input.substring(smsNotificationStartIndex + SMS_NOTIFICATION_START.length(), smsNotificationCallerNumberEndQuoteIndex);
  String smsDateTime = input.substring(smsNotificationSecondCommaIndex+2, smsNotificationFirst0D0A-1);
  String smsContentExtracted = input.substring(smsNotificationFirst0D0A + 2, smsNotificationSecond0D0A);
//  Serialprint("!!!!!!!!!!!!!!!!!!!!!!! Received SMS calleNumber: ");
//  Serialprintln(callerNumberExtracted);
//  Serialprint("!!!!!!!!!!!!!!!!!!!!!!! Received SMS DateTime: ");
//  Serialprintln(smsDateTime);
//  Serialprint("!!!!!!!!!!!!!!!!!!!!!!! Received SMS Content: ");
//  Serialprintln(smsContentExtracted);
  foundEndIndex = smsNotificationSecond0D0A + 2;  // 2 is for "\r\n";

  // smsDateTime is now in format: "21/12/04,13:21:48+04"
  int commaSeparatorIndex = smsDateTime.indexOf(',');
  if (commaSeparatorIndex == -1)
  {
      Serialprint(millis());
      Serialprint(": ");
      Serialprint("Unable to extract date portion from the smsNotification date time: ");
      Serialprintln(smsDateTime);
  }
  String _date = smsDateTime.substring(0, commaSeparatorIndex);
  String _time = smsDateTime.substring(commaSeparatorIndex + 1);
  
  int firstSlash = _date.indexOf('/');    
  int secondSlash = _date.indexOf('/', firstSlash + 1);
  if (firstSlash == -1 || secondSlash== -1)
  {
    Serialprint(millis());
    Serialprint(": ");
    Serialprint("FAILED to extract Date from: ");
    Serialprintln(_date);
  }
  int _year = _date.substring(0, firstSlash).toInt();
  int _month = _date.substring(firstSlash + 1, secondSlash).toInt();
  int _day = _date.substring(secondSlash + 1).toInt();
//  Serialprint(millis());
//  Serialprint(": ");
//  Serialprint("%%%%%%%%%%%%%% Extracted Date: Day:");
//  Serialprint(_day);
//  Serialprint(", Month: ");
//  Serialprint(_month);
//  Serialprint(", Year: ");
//  Serialprintln(_year);

  // 13:21:48+04
  int firstColon = _time.indexOf(':');
  int secondColon = _time.indexOf(':', firstColon + 1);
  int plusSign = _time.indexOf('+', secondColon + 1);
  int minusSign = _time.indexOf('-', secondColon + 1);
  if (firstColon == -1 || secondColon == -1 || (plusSign == -1 && minusSign == -1))
  {
    Serialprint(millis());
    Serialprint(": ");
    Serialprint("FAILED to extract Time from: ");
    Serialprintln(_time);
  }
  int _hour = _time.substring(0, firstColon).toInt();    
  int _min = _time.substring(firstColon + 1, secondColon).toInt();
  int secEndIndex = plusSign != -1 ? plusSign : minusSign;
  int _sec = _time.substring(secondColon + 1, secEndIndex).toInt();    
//  Serialprint(millis());
//  Serialprint(": ");
//  Serialprint("%%%%%%%%%%%%%% Extracted Time: ");
//  Serialprint(_hour);
//  Serialprint(":");
//  Serialprint(_min);
//  Serialprint(":");
//  Serialprintln(_sec);
  
  callerNumber = callerNumberExtracted;
  smsTime.tm_sec = _sec;
  smsTime.tm_min = _min;
  smsTime.tm_hour = _hour;
  smsTime.tm_mday = _day;
  smsTime.tm_mon = _month;
  smsTime.tm_year = 2000 + _year - 1900;
//tm_sec  int seconds after the minute  0-61*
//tm_min  int minutes after the hour  0-59
//tm_hour int hours since midnight  0-23
//tm_mday int day of the month  1-31
//tm_mon  int months since January  0-11
//tm_year int years since 1900
  smsContent = smsContentExtracted;
  return true;
}

inline bool FindRingNotification(String& input, int& foundEndIndex, String& callerNumber)
{
  // RING notification has fallowing format: \r\nRING\r\n\r\n+CLIP: \"+381642023960\",145,\"\",0,\"\",0\r\n
  int ringNotificationStartIndex = input.indexOf(RING_NOTIFICATION_START);
  if (ringNotificationStartIndex == -1) return false;
  int numberEndPos = input.indexOf("\"", ringNotificationStartIndex + RING_NOTIFICATION_START.length() + 1);
  int ringNotificationEndIndex = input.indexOf("\r\n", ringNotificationStartIndex + RING_NOTIFICATION_START.length() + 1);
  if ((numberEndPos == -1) && (ringNotificationEndIndex != -1))
  {
    return false;
  }
  callerNumber = input.substring(ringNotificationStartIndex + RING_NOTIFICATION_START.length(), numberEndPos);
  foundEndIndex = ringNotificationEndIndex + 2;  // 2 is for "\r\n";
  return true;

// String RING_NOTIFICATION_START = "\r\nRING\r\n\r\n+CLIP: \"";
}


#endif
