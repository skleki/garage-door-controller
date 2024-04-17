//#if defined (ESP8266)
//  #include <ESP8266WiFi.h>
//#elif defined (ESP32)
//  #include <WiFi.h>
//#endif
//
//#include <WiFiUDP.h>
//#include <Ethernet.h> 
//#include <HttpRequest.h> 

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <uri/UriRegex.h>

extern inline bool HeaterControl_IsHeaterOnATM();
extern inline float HeaterControl_RoomTemperature();
extern inline float HeaterControl_WaterTemperature();
extern String GsmStatusPart0();
extern String GsmStatusPart0();
extern inline void HeaterControl_ManualEnable(float desiredTemp);
extern inline void HeaterControl_ManualDisable();
extern inline void HeaterControl_AutomaticDisable();
extern inline void HeaterControl_AutomaticEnable(float desiredTemp, byte startHour, byte startMin, byte endHour, byte endMin);

String WiFiStatusPart0();
String WiFiStatusPart1();

void ParseHttpRequestAndDoActiona(WebServer& server);


#define RECONNECT_RETRY_INTERVAL 60

unsigned char LAN_MAC_Address[6];

#define SERVER_PORT 80
WebServer webServer(SERVER_PORT);

int WiFiMode = -1;
int MyWiFi_ReconnectCounter = 0;
String MyWiFi_Status_Part0;
String MyWiFi_Status_Part1;

DNSServer dnsServer;
const char* ssid = "GsmThermostat";  // Enter SSID here
const char* password = "lara05102008";  //Enter Password here
/* Put IP Address details */
IPAddress local_ip(192,168,244,1);
IPAddress gateway(192,168,244,1);
IPAddress subnet(255,255,255,0);
const byte DNS_PORT = 53;
const char* dnsName = "www.skleki.com";

IPAddress setup_ip(EEPROMDataRegular.Server_IP[0], EEPROMDataRegular.Server_IP[1], EEPROMDataRegular.Server_IP[2], EEPROMDataRegular.Server_IP[3]);
IPAddress setup_gw(EEPROMDataRegular.Server_IP[0], EEPROMDataRegular.Server_IP[1], EEPROMDataRegular.Server_IP[2], 1);
IPAddress setup_dns(EEPROMDataRegular.Server_IP[0], EEPROMDataRegular.Server_IP[1], EEPROMDataRegular.Server_IP[2], 1);
IPAddress setup_dnsSec(8, 8, 8, 8);
IPAddress setup_subnet(255, 255, 255, 0);

void PrintHttpRequestDetails(WebServer& server);

inline void MyWiFi_Initialise(void) 
{ 
  MyWiFi_Status_Part0 = "Initializing...";
  MyWiFi_Status_Part1 = "";
  
  ScanAndPrintSSIDs();
  Serialprintln();

  Serialprint("Configured WLAN_SSID: ");
  Serialprintln((const char *)EEPROMDataWiFiAP.WLAN_SSID);
  Serialprint("WLAN_PASS: ");
  Serialprintln((const char *)EEPROMDataWiFiAP.WLAN_PASS);
  Serialprint("WLAN_SECURITY: ");
  Serialprintln(EEPROMDataWiFiAP.WLAN_SECURITY);
  String clientMac = "";

  unsigned char MyWiFi_MAC_Address[6];
  WiFi.macAddress(MyWiFi_MAC_Address);	// gets MAC address into buffer
  memcpy(LAN_MAC_Address, MyWiFi_MAC_Address, 6);
  clientMac += macToStr(MyWiFi_MAC_Address);
  clientMac.toUpperCase();
  Serialprint("MAC Address: ");
  Serialprintln(clientMac);

  WIFI_Connect(WiFiMode);

  webServer.on(UriRegex("[\\s\\S]+"), []() 
  {
    PrintHttpRequestDetails(webServer);
    ParseHttpRequestAndDoActiona(webServer);

    bool hcManualEnabled;
    float hcManualDesiredTemp;
    bool hcAutoEnabled;
    float hcAutoDesiredTemp;
    byte hcAutoStartHour, hcAutoStartMin, hcAutoEndHour, hcAutoEndMin;
    HeaterControl_GetManualEnabled(&hcManualEnabled, &hcManualDesiredTemp);
    HeaterControl_GetAutomaticEnabled(&hcAutoEnabled, &hcAutoDesiredTemp, &hcAutoStartHour, &hcAutoStartMin, &hcAutoEndHour, &hcAutoEndMin);

    String myPage = generate_html_page(GsmStatusPart0(),
                                       GsmStatusPart1(),
                                       WiFiStatusPart0(),
                                       WiFiStatusPart1(),
                                       (EEPROMDataThermostat.ManualEnabled || EEPROMDataThermostat.AutomaticEnabled) ? "ENABLED" : "DISABLED",
                                       hcManualDesiredTemp,  /* desired temp, ATM the same for both Manual and Auto modes*/
                                       HeaterControl_RoomTemperature(),  /* curent temp*/
                                       HeaterControl_IsHeaterOnATM(),  /* heater is ON*/
                                       HeaterControl_WaterTemperature(),  /* waterTemp*/
                                       hcManualEnabled, /* Manual mode is enabled*/
                                       hcAutoEnabled, /* Automatic mode is enabled*/
                                       hcAutoStartHour, hcAutoStartMin,
                                       hcAutoEndHour, hcAutoEndMin);
    
    webServer.send(200, "text/html", myPage.c_str());
  });

  webServer.begin();
  Serialprint("Server listening is at port ");
  Serialprintln(SERVER_PORT);
}

inline void MyWiFi_1sHandler(void)
{
  MyWiFi_ReconnectCounter++;
}

inline void MyWiFi_Loop(void) 
{
  const long timeoutTime = 500;
  unsigned long currentTime = millis();
  unsigned long previousTime = 0; 

  if (WiFiMode == WIFI_STA && WiFi.status() != WL_CONNECTED && MyWiFi_ReconnectCounter > RECONNECT_RETRY_INTERVAL)
  {
    WDT_Reset();
    Serialprintln("Wifi not connected. reconnecting...");
    WIFI_Connect(WiFiMode);
    MyWiFi_ReconnectCounter = 0;
    return;
  }

  webServer.handleClient();
  delay(2);//allow the cpu to switch to other tasks
}


void ParseHttpRequestAndDoActiona(WebServer& server)
{
// Manual ON request:
//Args count: 1
//   arg 0: [ManualDesiredTemperatureSliderId]:[23.2]

// Manual OFF request:
//Args count: 1
//   arg 0: [MANUAL_OFF]:[1]

// AUTO ON request:
//Args count: 3
//   arg 0: [AutomaticDesiredTemperatureSliderId]:[30]
//   arg 1: [AutomaticStart]:[07:08]
//   arg 2: [AutomaticEnd]:[09:10]

// AUTO OFF request:
//Args count: 1
//   arg 0: [AUTOMATIC_OFF]:[1]

  if (server.args() == 1)
  {
    if (server.argName(0) == "ManualDesiredTemperatureSliderId")
    {
      float desiredTemp = server.arg(0).toFloat();
      HeaterControl_ManualEnable(desiredTemp);
    }
    else if (server.argName(0) == "MANUAL_OFF")
    {
      HeaterControl_ManualDisable();
    }
    else if (server.argName(0) == "AUTOMATIC_OFF")
    {
      HeaterControl_AutomaticDisable();
    }
  }
  else if (server.args() == 3)
  {
    if (server.argName(0) == "AutomaticDesiredTemperatureSliderId")
    {
      float desiredTemp = server.arg(0).toFloat();
      String startTimeAsString = server.arg(1);
      String endTimeAsString = server.arg(2);
      byte startHour = startTimeAsString.substring(0,2).toInt();
      byte startMin = startTimeAsString.substring(3,5).toInt();
      byte endHour = endTimeAsString.substring(0,2).toInt();
      byte endMin = endTimeAsString.substring(3,5).toInt();
      HeaterControl_AutomaticEnable(desiredTemp, startHour, startMin, endHour, endMin);
    }
  }
  else
  {
    Serialprint("INAVLID request, URI: ");
    Serialprintln(server.uri());
  }
}

void PrintHttpRequestDetails(WebServer& server)
{
  Serialprint("Method: ");
  Serialprintln(server.method());
  Serialprint("Uri: ");
  Serialprintln(server.uri());

  Serialprint("Args count: ");
  Serialprintln(server.args());
  for (int i = 0; i < server.args(); i++)
  {
    Serialprint("   arg ");
    Serialprint(i);
    Serialprint(": [");
    Serialprint(server.argName(i));
    Serialprint("]:[");
    Serialprint(server.arg(i));
    Serialprintln("]");
  }

  Serialprint("Headers count: ");
  Serialprintln(server.headers());
  for (int i = 0; i < server.headers(); i++)
  {
    Serialprint("   header ");
    Serialprint(i);
    Serialprint(": [");
    Serialprint(server.headerName(i));
    Serialprint("]:[");
    Serialprint(server.header(i));
    Serialprintln("]");
  }

  Serialprint("HOST Header: ");
  Serialprintln(server.hostHeader());
}

String WiFiStatusPart0()
{
  return MyWiFi_Status_Part0;
}

String WiFiStatusPart1()
{
  return MyWiFi_Status_Part1;
}

void ScanAndPrintSSIDs()
{
  Serialprintln("WiFI SCAN start...");
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();

  Serialprintln("WiFI SCAN done!");
  if (n == 0)
  {
    Serialprintln("no networks found");
  }
  else
  {
    Serialprint(n);
    Serialprintln(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serialprint("   ");
      Serialprint(i + 1);
      Serialprint(": ");
      Serialprint(WiFi.SSID(i));
      Serialprint(" (");
      Serialprint(WiFi.RSSI(i));
      Serialprintln(")");
      delay(10);
    }
  }
  Serialprintln("");
}

inline void WIFI_Connect(int currentWiFiMode)
{
  bool skipSettingAP = true;

  if (currentWiFiMode == WIFI_AP)
  {
    Serialprintln("WIFI_Connect while WiFiMode == WIFI_AP. This is a bug");
    return;
  }
  else if (currentWiFiMode != WIFI_STA)
  {
    skipSettingAP = false;
  }
  
  WiFi.disconnect();
  // setting up Station IP statically

  // setting up Station mode
  WiFi.mode(WIFI_STA);
  WiFiMode = WIFI_STA;

  // Uninitialised EEPROM has all 0xFFs
  if (setup_ip[0] != 255 && setup_ip[1] != 255 && setup_ip[2] != 255 && setup_ip[3] != 255)
  {
  #if defined (ESP8266)
    WiFi.config(setup_ip, setup_dns, setup_gw, setup_subnet);
  #elif defined (ESP32)
    if (!WiFi.config(setup_ip, setup_gw, setup_subnet, setup_dns, setup_dnsSec))
    {
      Serialprintln("STA Failed to configure");
    }
  #endif
  }
  
  MyWiFi_Status_Part0 = "Connect. to";
  MyWiFi_Status_Part1 = String((const char *)EEPROMDataWiFiAP.WLAN_SSID);

  WiFi.begin((const char *)EEPROMDataWiFiAP.WLAN_SSID, (const char *)EEPROMDataWiFiAP.WLAN_PASS);

  // Wait for connect to AP
  Serialprint("[Connecting]");
  Serialprint((const char *)EEPROMDataWiFiAP.WLAN_SSID);
  int tries=0;
  while (WiFi.status() != WL_CONNECTED) 
  {
    WDT_Reset();
    delay(500);
    Serialprint(".");
    tries++;
    if (tries > 30)
    {
      break;
    }
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serialprintln("Unable to connest to WiFi network");
    Serialprintln();

    if (!skipSettingAP)
    {
      SetupAP();
    }
    
    return;
  }
  
  Serialprintln("Connected to wifi");
  Serialprintln();
  printWifiStatus();
  MyWiFi_Status_Part0 = WiFi.SSID();
  MyWiFi_Status_Part1 = WiFi.localIP().toString ();
}

void SetupAP()
{
  MyWiFi_Status_Part0 = "Conf. AP";
  MyWiFi_Status_Part1 = String((const char *)ssid);

  Serialprint("Wi-Fi mode set to WIFI_AP ");
  Serialprintln(WiFi.mode(WIFI_AP) ? "" : "Failed!");
  WiFiMode = WIFI_AP;

  Serialprint("Setting soft-AP ... ");
  Serialprintln(WiFi.softAP(ssid, password) ? "Ready" : "Failed!");
  Serialprintln("Wait 100 ms for AP_START...");
  delay(100);

  Serialprint("Setting soft-AP configuration ... ");
  Serialprintln(WiFi.softAPConfig(local_ip, gateway, subnet) ? "Ready" : "Failed!");

  Serialprint("Soft-AP IP address = ");
  Serialprintln(WiFi.softAPIP());

  MyWiFi_Status_Part0 = String((const char *)ssid);
  MyWiFi_Status_Part1 = String(WiFi.softAPIP()[0]) + "." + String(WiFi.softAPIP()[1]) + "." + String(WiFi.softAPIP()[2]) + "." + String(WiFi.softAPIP()[3]);

  // modify TTL associated  with the domain name (in seconds)
  // default is 60 seconds
  dnsServer.setTTL(300);
  // set which return code will be used for all other domains (e.g. sending
  // ServerFailure instead of NonExistentDomain will reduce number of queries
  // sent by clients)
  // default is DNSReplyCode::NonExistentDomain
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);

  // start DNS server for a specific domain name
  dnsServer.start(DNS_PORT, dnsName, local_ip);

  delay(100);
}

inline long SignalStrength()
{
  if (WiFi.status() != WL_CONNECTED) 
  {
    Serialprintln("Couldn't get a wifi connection");
    return 0;
  }
  else
  {
    long rssi = WiFi.RSSI();
    Serialprint("RSSI:");
    Serialprintln(rssi);
    return rssi;
  }
}

inline void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serialprint("SSID: ");
  Serialprintln(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serialprint("Module IP Address: ");
  Serialprintln(ip);
}

inline String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) 
  {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}
