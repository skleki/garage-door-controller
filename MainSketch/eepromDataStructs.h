#ifndef __EEPROMDATASTRUCTS_H
  #define __EEPROMDATASTRUCTS_H 
  
struct MyEepromDataRegular{
  unsigned long MagicNumber;
  unsigned long CommsType;      // Not Used with Fast UDP Comms
  byte RF24_ChAdd0[8];          // Not Used with Fast UDP Comms
  byte RF24_ChAdd1[8];          // Not Used with Fast UDP Comms
  unsigned long RetransmitCount; 
  unsigned long PingTimeoutSeconds;
  byte ARDUINO_MACadd[8];       // Not Used with Fast UDP Comms //{0x08, 0x00, 0x28, 0x57, 0xAC, 0x96}
  byte Server_IP[4];           //(192.168.88.205)
  unsigned long Server_UDP_Port;   //byte  (30231)
  byte ARDUINO_SubNetMask[4];   // Not Used with Fast UDP Comms //(255, 255, 255, 0)
  byte ARDUINO_dfGW[4];         // Not Used with Fast UDP Comms//(192.168.0.1)
  byte ARDUINO_DNSServer[4];    // Not Used with Fast UDP Comms//(192.168.0.1)
  unsigned long LoggingEnabled;
  unsigned long CRC;
}; 

struct MyEepromDataWiFiAP{
  unsigned long MagicNumber;
  byte WLAN_SSID[32];  //("NasStancic")        // cannot be longer than 32 characters!
  byte WLAN_PASS[32];  //("lara05102008")
  unsigned long WLAN_SECURITY;  /* (#define    WLAN_SEC_UNSEC    (0)
                                  #define    WLAN_SEC_WEP      (1)
                                  #define    WLAN_SEC_WPA      (2)
                                  #define    WLAN_SEC_WPA2     (3) */
  unsigned long CRC;
}; 

struct MyEepromDataThermostat{
  unsigned long MagicNumber;

  bool ManualEnabled;
  float DesiredTemp;
  bool AutomaticEnabled;
  byte AutomaticStartHour; 
  byte AutomaticStartMin; 
  byte AutomaticEndHour; 
  byte AutomaticEndMin; 
 
  unsigned long CRC;
}; 

#endif
