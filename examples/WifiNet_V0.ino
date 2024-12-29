
/*
 * Unit test for <WifiNet> lib
 * Ver 0 29-V-2024
 *
 *  active commands:
 *                  not found (onNotFound)
 *                  root  / 
 *                        /erase        /erase                                    // clear credential from EEPROM
 *                        /ResetSystem  /ResetSystem                              // reset the platform
 *                  OTA   /cred                                                   // Wifi credential input page
 *                        /setting      /setting?SSID=Sachi&Pass=Kalisher46apt7   // set wifi credential
 *            ElegantOTA  /updtae       update bin file
 *
 *
 *  add: store Wifi and check
 *  add: simple web server
 */
static const char Version[] PROGMEM = "V0 29.V.2024";
//                                                  0           1                     2             3             4             5               6               7
static const char SWapplication[8][18] PROGMEM ={"Wifi Test", "Shades Controller", "Gray Water", "MQTTTester", "WebTester", "IR controller", "IRcontrol V1","Nixie Controller"};
#define   CurrentApp      0         // Email unit test

#define   BAUDRATE    115200
#define   LOGGME      true            // enable progress of new log on serial monitor
#define   DEBUGON     true            // debug prints
//#define   DONTEMAIL   true            // skip email send for debugging
#define   OTAWIFICONFIG true          // enable credential setting OTA
#define   OTAelegantServer  true      // enable SW update OTA
const int LEDPIN  = D4;               // LED on GPIO2

// Clock
#include <Clock.h>                    // self generated master clock lib
TimePack  SysClock ;
Clock     RunClock(SysClock);         // clock instance

// Utilities
#include <Utilities.h>                // self generated utilities lib
Utilities RunUtil(SysClock);          // Utilities instance

// Wifi
#include  <EEPROM.h>
const uint16_t EEPROMSIZE=3600;       // size of EEPROM to be emulated by library (<MaxEErecLength>*10+100)
#include  <ElegantOTA.h>              // https://docs.elegantota.pro/async-mode/
#include  <WifiNet.h>
ManageWifi  SysWifi;
WifiNet   RunWifi(SysWifi);           // Wifi instance

// simple async server
#define   IoTWEBserverPort   80       // WiFi server port
AsyncWebServer  IoTWEBserver(IoTWEBserverPort);
void  JustForCompilation(AsyncWebServerRequest *request){  }
#define   HTMLpageBufLen  7500        // length of HTMP page temp buffer

#ifndef DONTEMAIL
  // email
  #include  <WifiEmail.h>
  ManageEmail SysEmail;                 // instantiate email data
  WifiEmail RunEmail(SysEmail,SWapplication[CurrentApp]); // Email lib instance
#endif  DONTEMAIL

//****************************************************************************************/
void setup() {
  //
  // 1. set the environment
  //
  static const char Mname[] PROGMEM = "setup:";
  Serial.begin(BAUDRATE);                               // Serial monitor setup
  delay(3000);
  Serial.print("\n\n\nInit Unit test of WifiNet:\n\n");
  SysClock = RunClock.begin(SysClock);
  RunUtil.begin(LEDPIN);
  EEPROM.begin(EEPROMSIZE);                             //Initializing EEPROM
  delay(10);    
  SysWifi = RunWifi.begin(SysWifi);
  strcpy_P(SysWifi.Version,Version);                    // init SW version
  strcpy_P(SysWifi.WhoAmI,SWapplication[CurrentApp]);   // init app identification
  SysWifi = RunWifi.startWiFi(SysClock, SysWifi);       // init connection to Wifi
  RunUtil.InfoStamp(SysClock,Mname,"",1,0); Serial.print(F("Connecting to Wifi -END\n"));
  //
  // 1.1 init email - can skip this part by 
  //
    
  #ifndef DONTEMAIL
    uint8_t HowMany, dontEmail;
    HowMany = 3;
    dontEmail = true;                                     // only email simulator
    SysEmail = RunEmail.begin(SysEmail,HowMany,dontEmail);
    #define   UserAddress "sachi.gerlitz@gmail.com"
    strcpy(SysEmail.emailAddressToSend,UserAddress);      // load user address
  #endif  DONTEMAIL
  //
  // 2. init simple async server
  //
  AsyncSimpleServerHandlers();          // set async web server handlers
  //
  // 3. connect to network
  //
  while (1==1) {                        // connect to wifi
    SysWifi = RunWifi.WiFiTimeOut(SysClock,SysWifi);
    if ( SysWifi.activeTimeEvent==2 ) { // wifi connected
      break;
    } else {                            // continue to retry
      delay(200);
      Serial.print(F("."));
      SysWifi.activeTimeEvent = 0;      // clear semaphor post action
    }
  } // end of wait for wifi
  RunUtil.InfoStamp(SysClock,Mname,"",1,0); Serial.print(F(" SysWifi.activeTimeEvent ")); Serial.print(SysWifi.activeTimeEvent); Serial.print(F(" -END\n"));
  SysWifi.activeTimeEvent = 0;          // clear semaphor post completion
  //
  // 4. get network time
  //
  while (1==1) {                        // get network time
    SysClock = RunWifi.GetWWWTime(SysClock,SysWifi);
    if ( SysClock.IsTimeSet ){          // network time set
      break;
    } else {                            // continue to retry
      delay(500);
      SysWifi.activeTimeEvent = 0;      // clear semaphor post action
    }
  } // end wait for network time
  //
  // 5. check IP if new
  //
  bool  c;
  RunUtil.InfoStamp(SysClock,Mname,"",1,0); Serial.print(F("DeviceIP len=")); Serial.print(strlen(SysWifi.DeviceIP)); Serial.print(F(" -END\n"));
  RunUtil.InfoStamp(SysClock,Mname,"",1,0); Serial.print(F("DeviceIP ")); Serial.print(SysWifi.DeviceIP); Serial.print(F(" -END\n"));
  c = RunWifi.CompareAndKeepIP (SysClock,SysWifi);
  RunUtil.InfoStamp(SysClock,Mname,"",1,0);
  if ( c )  Serial.print(F("New address aquired -END\n"));
  else      Serial.print(F("Network IP is equal to kept address -END\n"));
  // change  stored address
  char  buf[16];
  RunWifi.fetchIPaddress(buf,EEPROMipAddress);
  buf[0] = '5';
  RunWifi.storeIPaddress(SysClock, buf, EEPROMipAddress);
  c = RunWifi.CompareAndKeepIP (SysClock,SysWifi);
  RunUtil.InfoStamp(SysClock,Mname,"",1,0);
  if ( c )  Serial.print(F("New address aquired -END\n"));
  else      Serial.print(F("Network IP is equal to kept address -END\n"));

  #ifndef DONTEMAIL
    //
    // 6. send email
    //
    char* subj = "test of subject";
    char* body = "test of email message<br>end of message";

    SysEmail = RunEmail.EmailWrapper(SysEmail,SysClock,subj,body);    // send email
    RunUtil.InfoStamp(SysClock,Mname,G3,1,0); Serial.print(F(" return code ")); Serial.print(SysEmail.RC); Serial.print(F(" -END\n"));
  #endif DONTEMAIL
  //
  RunUtil.InfoStamp(SysClock,Mname,"",1,0); Serial.print(F("Unit test of WifiNet ended -END\n"));
} // end of setup

//****************************************************************************************/
void loop() {
  static const char Mname[] PROGMEM = "Loop:";
  #ifdef  OTAelegantServer
    ElegantOTA.loop();                  // for over the air firmware updates
  #endif  OTAelegantServer
  if (SysWifi.activeTimeEvent==4) {     // Asyc command to reset the system
    delay(3000);
    ESP.restart();                      // https://techtutorialsx.com/2017/12/29/esp8266-arduino-software-restart/
  }   // end of reset system check
} // end of loop

//****************************************************************************************/
//****************************************************************************************/