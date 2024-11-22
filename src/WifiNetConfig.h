/*
 * WifiNetConfig.h  is a configuraiton file for WifiNet library 
 * Created by Sachi Gerlitz
 * 
 * 22-IX-2024   ver 0.3   [redefine<CredSettingTrigger>]
 * 27-VIII-2024 ver 0.2
 * This is a workaround to pass configuration Macro values to the library
 * it contains:
 *  A - compilation flags
 *  B - setup variables and information for configuration
 *  C - Common definition for library and calling methods
 */

#ifndef WifiNetConfig_h
  #define WifiNetConfig_h

  //
  // Part A - Compilation flags
  //
  #ifndef _WIFINTPON
    #define _WIFINTPON    1       // enable use of network time
  #endif  //_WIFINTPON
  #ifndef _LOGGME
    #define _LOGGME       1       // enable progress of new log on serial monitor
  #endif  //_LOGGME
  #ifndef _DEBUGON
    #define _DEBUGON      200       // debug prints
  #endif  //_DEBUGON
  #ifndef _SETDEEPSLEEP
  //#define  _SETDEEPSLEEP  true
  #endif  //_SETDEEPSLEEP
  #ifndef _OTAWIFICONFIG
    #define _OTAWIFICONFIG  1     // enable wifi credential setting OTA
  #endif  //_OTAWIFICONFIG
  #ifndef _STATICIP
    #define _STATICIP     0       // enable static IP configuration
  #endif  //_STATICIP

  // the foloowing definitions need consideration
  //#define   CLEAREEPROM     true
    
  //
  // Part B - Configuration information
  //
  // setup parameters
  const  String      SoftAccPntSSID="ESP8266-OTA";              // SSID for the access point
  static const char CredSettingTrigger[] ="/setting";
  static const char SSID_Phrase[] = "SSID";             // SSID input field name
  static const char PSWD_Phrase[] = "Pass";             // password input field name
  static const char Per_SSID[] PROGMEM = "Explorers House Guests";
  static const char Per_Pass[] PROGMEM = "a21guest";
  static const char NO_IP_Set[] PROGMEM = "000.000.000.000  ";    //17 chars
  #ifndef NTPdelayAfterReset
    #define NTPdelayAfterReset  1500                            // delay[mS] after reset for 1st time NTP call
  #endif  //NTPdelayAfterReset
  #ifdef  _SETDEEPSLEEP
    const uint8_t   ConnTimeOutRep  = 60;  // 60 repeats ( 100*60= 6 seconds for deep-sleep)
  #else
    const uint8_t   ConnTimeOutRep  =120; // 12- repeats ( 100*120= 12 seconds for regular)
  #endif  //_SETDEEPSLEEP
  #define EEPROMipAddress 0x004B                                // EEPROM location of IP start record

  //
  // Part C - Common definition for library and calling methods
  //
  enum  Codes4WiFi  {
    Not_Connected=0,        // 0 - not connected
    Trying_Connect=1,       // 1 - trying to connect as station
    Connected=2,            // 2 - connected as station
    Configure_OTA=3,        // 3 - OTA server configured (waiting for client)
    Client_Connect_OTA=4,   // 4 - Client connected as AP (OTA)
    Connection_lost         // 5 - WiFi connection lost
  };
  enum  Codes4Watchlamp {   // codes used for watchdog LED
    LedWifiSearch=0,        // 0 - search for WiFi to connect as station
    LedAPSearch=1,          // 1 - search for client to connect as WiFi Access Point
    LedWifiLost=2,          // 2 - WiFi Network connection lost
    LedNTPfailure=3,        // 3 - Network time NOT set (NTP failure)
    LedSystemOK=4,          // 4 - Shades controller normal operation
    LedShadePause=5,        // 5 - Shades controller script paused operation
    LedSDfailure=6,         // 6 - Error opening SD file
    LedTBD7=7               // 7 - not in use
  };
  static const char _G3[] PROGMEM = " ";
  static const char _G4[] PROGMEM = "\n";
  static const char _G7[] PROGMEM = "Error";
  static const char _TextHTML[] = "text/html";
  static const char _DIGITS[18][2] PROGMEM ={"0","1","2","3","4","5","6","7","8","9"," ",":","_",".",",","-","/","="};
                                          // 0   1   2   3   4   5   6   7   8   9  10  11   12  13  14  15  16  17  18
  //
#endif  //WifiNetConfig_h