/*
 * WifiNet.h library for methods to connect to Wifi for IoT applications
 * Created by Sachi Gerlitz
 * 
 * 29-XII-2024  ver 0.3.1 [add <getVersion>]
 * 27-VIII-2024 ver 0.2
 *
 */
#ifndef WifiNet_h
  #define WifiNet_h

  #include <ESPAsyncWebServer.h>    // https://github.com/me-no-dev/ESPAsyncWebServer
  #include  "WifiNetConfig.h"

  // pre defined macro check
  #if !defined(_WIFINTPON)
    #error "WifiNet.h error! _WIFINTPON is not defined (missing WifiNetConfig.h?)"
  #endif  //_WIFINTPON
  #ifndef _WIFINTPON                    // use of network time 
    #define _WIFINTPON  0
  #endif  //_WIFINTPON
  #ifndef _LOGGME                       // enable logging print
    #define _LOGGME     0
  #endif  //_LOGGME
  #ifndef _DEBUGON                      // endable debug prints
    #define _DEBUGON    0
  #endif  //_DEBUGON

  #include  "Arduino.h"
  
  #ifndef SSIDlength
    #define     SSIDlength  32          // maximum SSID length stored in EEPROM
  #endif  //SSIDlength
  #ifndef PASSlength
    #define     PASSlength  32          // maximum PW length stored in EEPROM
  #endif  //PASSlength
  struct  ManageWifi {
    uint8_t     WiFiStatus;             // status of WiFi connection:
    char        Ssid[SSIDlength+1];     // network ID
    char        Password[PASSlength+1]; // network password
    uint8_t     WiFiBSsid[6];           // BSSid
    uint8_t     WiFichannel;            // channel
    uint8_t     CredStat;               // status of credentials, values by <Codes4WiFi> enup in .cpp
    unsigned long TimeMeasured;         // keeps the time to connection
    uint8_t     ledIndicationCode;      // led indication management
    uint8_t     HowLongItTook;          // counter of connection retries
    uint8_t     activeTimeEvent;        // semaphore from library to time event <ManageWifiEvents>:
                                        // 0-no action; 1- WIFICONNECT-On ; 2-InitAppPostWiFi-On, WIFICONNECT-Off;
                                        // 3-WIFICONNECT-Off; 4-reset system; 5-TBD;
    bool        StaticDynamicIP;        // set for static IP, reset for DNS address
    char        DeviceIP[18];           // keeps actual IP
    bool        RefreshTimeSet;         // flag to init time refresh
    char        WhoAmI[18];             // platform ID
    char        Version[18];            // current SW version
    uint32_t    uploadedFileLen;        // length of OTA elegant Server uploaded file
    bool        uploadFileRady;         // OTA elegant Server uploaded file completed
  };

  class WifiNet {
    public:
      WifiNet(ManageWifi M);					  // constructor
      ManageWifi  begin(ManageWifi M);
      ManageWifi  startWiFi(TimePack  SysClock, ManageWifi M);
      ManageWifi  WiFiTimeOut(TimePack  SysClock, ManageWifi M);
      ManageWifi  IsWifiConnected(TimePack  SysClock, ManageWifi M);
      void        WiFiCodePrint(uint8_t Index);
      TimePack    GetWWWTime (TimePack  SysClock, ManageWifi M);
      ManageWifi  startOTAWifiServer(TimePack SysClock, ManageWifi M);
      void        whileWait4Wifi(ManageWifi M);
      ManageWifi  fetchCredFromEEPROM(TimePack _SysClock, ManageWifi M);
      ManageWifi  UpdateWifiCredentials(TimePack _SysClock, ManageWifi M);
      bool        ClearEEPROMwifiCredentials(TimePack  SysClock);
      bool        KeepCredentialsEEPROM (TimePack _SysClock, char* id, char* psw);
      bool        KeepChaBssidEEPROM (TimePack _SysClock, uint8_t Bssid[], uint8_t Channel);
      ManageWifi  ServiceOTACred(AsyncWebServerRequest *request, TimePack _SysClock, ManageWifi M);
      char*       SimpleUtilityPage(TimePack _SysClock, ManageWifi M, char* buf, uint8_t option, 
                       const char* PageTitleName, const char* FeedBack, const char* insert_action);
      bool        storeIPaddress(TimePack _SysClock, char* IPstring, uint16_t EEPaddress);
      char*       fetchIPaddress(char* buff, uint16_t EEPaddress);
      bool        CompareAndKeepIP (TimePack _SysClock,ManageWifi M);
      const char* getVersion();
    private:
      ManageWifi  _LM;

  };

#endif  //WifiNet_h