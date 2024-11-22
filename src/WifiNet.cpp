/*
 * WifiNet.cpp library for methods to connect to Wifi for IoT applications
 * Created by Sachi Gerlitz
 * 22-IX-2024   ver 0.3   [redefine<CredSettingTrigger>]
 * 26-VIII-2024 ver 0.2   [sequance of printing for <GetWWWTime>,<NTPserver>]
 *                        [needs re-writing: #ifdef OLEDON]
 *
 * constructor:   WifiNet
 * methods:       begin; startWiFi; WiFiTimeOut; IsWifiConnected; WiFiCodePrint; GetWWWTime; 
 *                startOTAWifiServer; whileWait4Wifi; fetchCredFromEEPROM; UpdateWifiCredentials; 
 *                ClearEEPROMwifiCredentials; KeepCredentialsEEPROM; KeepChaBssidEEPROM; ServiceOTACred;
 *                SimpleUtilityPage; storeIPaddress; fetchIPaddress; CompareAndKeepIP;
 *                
 * EEPROM allocation
 * 
 * 0000 -> 0074 0x0000 -> 0x004A    credentials record
 * 0075 -> 0092 0x004B -> 0x005A    IP string (15 chars+0x00) referred by <EEPROMipAddress>
 * 0092 -> 0099 0x005B -> 0x0063    spare
 * 0100 ->      0x0064 ->           Application's records
 *
 */

#include  "Arduino.h"
#include  "Clock.h"
#include  "Utilities.h"
#include  "WifiNet.h"
#include <ESP8266WiFi.h>          // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html
                                  // https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFi.h
#include <ESPAsyncTCP.h>
#include <EEPROM.h>

TimePack  _SysClock;                // clock data
Clock     _RunClock(_SysClock);     // clock instance
Utilities _RunUtil(_SysClock);      // Utilities instance

#ifdef  SETDEEPSLEEP
  #define   ConnTimeOutRep  60      // 60 repeats ( 100*60= 6 seconds for deep-sleep)
#else
  #define   ConnTimeOutRep  120     // 120 repeats ( 100*120= 12 seconds for regular)
#endif  //SETDEEPSLEEP

// Local WiFi credentials
const uint8_t LipA=192, LipB=168, LipC=7, LipD=60;      // local static IP

IPAddress   PreGateway(LipA,LipB,1,1);
IPAddress   PreSubnet(255,255,255,0);
IPAddress   Pre_local_IP(LipA,LipB,LipC,LipD);      // local IP
IPAddress   Gateway, Subnet;  //, primaryDNS, seconderyDNS;

//****************************************************************************************/
WifiNet::WifiNet(ManageWifi M) {
    _LM = M;
}     // end of WifiNet 

//****************************************************************************************/
ManageWifi WifiNet::begin(ManageWifi M){
  ManageWifi _M=M;

  Gateway       = PreGateway;
  Subnet        = PreSubnet;
  //IPAddress PPprimaryDNS(8,8,8,8);
  //IPAddress PPseconderyDNS(8,8,4,4);
  //primaryDNS    = PPprimaryDNS;
  //seconderyDNS  = PPseconderyDNS;
  strcpy_P(_M.Ssid,Per_SSID);           // set default credentials for EEPROM not configured
  strcpy_P(_M.Password,Per_Pass);
  _M.CredStat = Not_Connected;
  _M.WiFiStatus = Not_Connected;        // WiFi not connected
  _M.ledIndicationCode = LedWifiSearch; // indicate search for WiFi network
  _M.activeTimeEvent = 0;
  _M.HowLongItTook = 0;
  _M.RefreshTimeSet = false;
  strcpy_P(&_M.DeviceIP[0],NO_IP_Set);
  _M.uploadedFileLen = 0;               // init length of OTA elegant Server uploaded file
  _M.uploadFileRady = false;            // init OTA elegant Server uploaded file complete flag
  return  _M;
  //
}     // end of begin

//****************************************************************************************/
ManageWifi  WifiNet::startWiFi(TimePack  _SysClock, ManageWifi M) {
  /*
    * Procedure to connect to the WiFi network as a station by variety of credentials options
    */
  static const char Mname[] PROGMEM = "startWiFi:";
  static const char L0[] PROGMEM = "Connecting to";
  static const char L1[] PROGMEM = "WiFichannel=";
  static const char L3[] PROGMEM = "Not configured!";
  static const char L4[] PROGMEM = "Partially configured.";
  static const char L5[] PROGMEM = "Fully configured.";
  //static const char E0[] PROGMEM = "ERROR CredStat=";
  static const char E1[] PROGMEM = "ERROR failed to configure static IP required";
  ManageWifi _M=M;

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  _M.WiFiStatus = Not_Connected;        // WiFi not connected
  _M.ledIndicationCode = LedWifiSearch; // indicate search for WiFi network
  delay(100);

  //KeepCredentialsEEPROM("Sachi","Kalisher46apt7");
  // fetch credentials
  _M = fetchCredFromEEPROM(_SysClock,_M); // get credentials from EEPROM
  #if _LOGGME==1
    _RunUtil.InfoStamp(_SysClock,Mname,L0,1,0); Serial.print(_M.Ssid); Serial.print(F(" (Password ")); Serial.print(_M.Password); 
    Serial.print(F(") Credenial status ")); 
    switch (_M.CredStat) {
      case  0:  Serial.print(L3); break;
      case  1:  Serial.print(L4); break;
      case  2:  Serial.print(L5); break;
      default:  break;            // error
    }
    Serial.print(F(" ("));Serial.print(_M.CredStat); Serial.print(F(") -END\n"));
  #endif  //_LOGGME
  
  // connect
  _M.TimeMeasured = _RunClock.StartStopwatch();
  _M.WiFiStatus=Trying_Connect;         // trying to connect to WiFi 
  switch ( _M.CredStat ) {
    case 0:                             // credentials are not set - dummy call or default
      WiFi.begin(_M.Ssid, _M.Password);
      break;
    case 1:                             // partial credential exists
      WiFi.begin(_M.Ssid, _M.Password);
      break;
    case 2:                             // full credentials exists
      #if _LOGGME==1
        _RunUtil.InfoStamp(_SysClock,Mname,L1,1,0);  Serial.print(_M.WiFichannel); Serial.print(F(" WiFiBSsid="));
        for ( int i=0; i<6; i++ ){ Serial.print(_M.WiFiBSsid[i],HEX); Serial.print(F(":")); } Serial.print(F(" -END\n"));
      #endif  //_LOGGME
      WiFi.begin(_M.Ssid, _M.Password, _M.WiFichannel, _M.WiFiBSsid, true );  // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/station-class.html#begin 
      break;
    default:
      // error
      _RunUtil.InfoStamp(_SysClock,Mname,E1,1,0); Serial.print(_M.CredStat); Serial.print(F(" -END\n"));
      _M.WiFiStatus=Not_Connected;                       // WiFi not connected
      break;
  }   // end of begin switch

  // establish connection
  _M.activeTimeEvent = 1;                    // set connection timer - renewable <WIFICONNECT>
  return  _M;
} // end of startWiFi

//****************************************************************************************/
ManageWifi  WifiNet::WiFiTimeOut(TimePack  _SysClock, ManageWifi M){
  /*
    * Method to address end of wifi connect time out
    */
  static const char Mname[] PROGMEM = "WiFiTimeOut:";
  static const char L0[] PROGMEM = "Connection timeout.";
  static const char L1[] PROGMEM = "Setting up Soft Access Point.";
  static const char L2[] PROGMEM = "WiFi connection lost. Trying more";
  static const char E0[] PROGMEM = "Error! Wrong wifi status code=";
  ManageWifi _M=M;
  #ifdef OLEDON
    char  OLEDbuf[10];
    char  OLEDbuf1[10];
    static const char OLEDmsg0[] PROGMEM = "WiFi Conn";
    static const char OLEDmsg1[] PROGMEM = "Start OTA";
    static const char OLEDmsg2[] PROGMEM = "TimOut";
  #endif //OLEDON
    
  // check if WiFi connected (skip if waiting for credentials, all is async)
  if ( _M.WiFiStatus != Configure_OTA ) _M = IsWifiConnected(_SysClock,_M); 
  whileWait4Wifi(_M);                                 // print while waiting

  switch ( _M.WiFiStatus ) {
    case  Connected:                                  // WiFi connected
      _M.ledIndicationCode = LedSystemOK;
      _M.activeTimeEvent = 2;                         // stop connection timer <WIFICONNECT>, set <InitAppPostWiFi>
                                                      // <ledIndicationCode> will be set @application by <StartApplicationAfterWiFi> 
      #ifdef OLEDON
        strcpy_P(OLEDbuf,OLEDmsg0);
        printOLED (OLEDbuf,Ssid,2);
      #endif //OLEDON
      break;

    case  Connection_lost:                            // WiFi accidential lost - wait
      _M.HowLongItTook = 1;                           // keep timeout timer
      _M.activeTimeEvent = 1;                         // set connection timer for renew
      _RunUtil.InfoStamp(_SysClock,Mname,L2,1,1); 
      _M.WiFiStatus  = Trying_Connect;
      _M.ledIndicationCode = LedWifiLost;             // indicate connection lost
      break;

    case  Trying_Connect:                             // Not connected - check timeout
      _M.ledIndicationCode = LedWifiSearch;
      if ( _M.HowLongItTook >= ConnTimeOutRep ) {     // apply timeout for connection
        #if  _OTAWIFICONFIG==1
          _M.activeTimeEvent = 1;                     // set connection timer for renew
          #if _LOGGME==1
            Serial.println();     // terminate wait line
            _RunUtil.InfoStamp(_SysClock,Mname,L0,1,0); Serial.print(_M.HowLongItTook); Serial.print(F(" tries.-END\n"));
            _RunUtil.InfoStamp(_SysClock,Mname,L1,1,1); 
          #endif  //_LOGGME
          #ifdef OLEDON
            printOLED ( "Start OTA","TimOut",2);
          #endif //OLEDON
          _M.ledIndicationCode = LedAPSearch;         // indicate AP mode seaarch for client
          _M = startOTAWifiServer(_SysClock, _M);     // start soft access point
        #endif  //_OTAWIFICONFIG
        _M.HowLongItTook = 0;                         // reset counter to avoid overflow
      }                                               // end of tries check
      break;

    case  Configure_OTA:                              // waiting for connection while OTA
      _M.ledIndicationCode = LedAPSearch;             // indicate AP mode seaarch for client
      _M.activeTimeEvent = 1;                         // set connection timer for renew
      break;

    case  Client_Connect_OTA:                         // TBD
    default:                                          // on program error (can't be other value)
      _M.ledIndicationCode = LedSDfailure;            // indicate error
      _RunUtil.InfoStamp(_SysClock,Mname,E0,1,0); Serial.print(_M.WiFiStatus); Serial.print(F("! -END\n"));
      break;
  }   // end of connection switch
    
  return  _M;
}     // end of WiFiTimeOut

//****************************************************************************************/
ManageWifi WifiNet::IsWifiConnected(TimePack  _SysClock, ManageWifi M){
  /*
    * method to check until connected to WiFi network, it is called after <WIFICONNECT> event is due
    *  - if not connected, activate timeout for OTA credenial setting
    *  - when connected, establish all connection mechanisms: local IP, update credenials
    *    time setting by network was moved to calling method, though might be effected when setting static IP
    */
  static const char Mname[] PROGMEM = "IsWifiConnected:";
  #if  (_STATICIP==1) && (_LOGGME==1)
    static const char E0[] PROGMEM = "ERROR failed to configure static IP required";
  #endif  //(_STATICIP==1) && (_LOGGME==1)
  //static const char L1[] PROGMEM = "Waiting for connection";
  static const char L2[] PROGMEM = "Connected to network.";
  static const char G2[] PROGMEM = "IP Address:";
  ManageWifi _M=M;
  // https://www.arduino.cc/en/Reference/WiFiStatus
  if (WiFi.status() != WL_CONNECTED) {      // continue the wait period, reneu the timer
                                            //------------------------------------------
    _M.activeTimeEvent = 1;                 // re start connection timer <WIFICONNECT>
    if ( WiFi.status()==WL_CONNECTION_LOST ) _M.WiFiStatus=Connection_lost;
    _M.HowLongItTook++;
    
  } else {                                  // successul connection to WiFi 
                                            //--------------------------------------------
    #if _LOGGME==1
      _RunUtil.InfoStamp(_SysClock,Mname,L2,1,1); 
    #endif  //_LOGGME
                                            // configure network
    #if  _STATICIP==1                       // for static IP configuration
      if (!WiFi.config(Pre_local_IP,PreGateway,PreSubnet/*,primaryDNS,seconderyDNS*/)) {
                                            // failure to set static IP
        _M.StaticDynamicIP = false;         // revert to DNS supplied IP
        #if _LOGGME==1
          _RunUtil.InfoStamp(_SysClock,Mname,E0,1,0); Serial.print(Pre_local_IP); Serial.print(F(" Gateway ")); Serial.print(PreGateway); 
          Serial.print(F(" Subnet ")); Serial.print(PreSubnet); Serial.print(F(" Time to connect (staticIP fail)="));
          Serial.print(_RunClock.ElapseStopwatch(_M.TimeMeasured)); Serial.print(F("mS - END\n"));
        #endif  //_LOGGME
      } else {
        // successful Static IP
        _M.StaticDynamicIP = true;
      }   // end of IP configuration
    #endif  //_STATICIP

    #if _LOGGME==1
      _RunUtil.InfoStamp(_SysClock,Mname,G2,1,0); Serial.print(WiFi.localIP()); Serial.print(F(" Gateway ")); Serial.print(PreGateway); 
      Serial.print(F(" Subnet ")); Serial.print(PreSubnet); 
      Serial.print(F(" Time to connect: ")); Serial.print(_RunClock.ElapseStopwatch(_M.TimeMeasured)); Serial.print(F("mS - END\n"));
    #endif  //_LOGGME
                                                // update credentials
    memcpy( _M.WiFiBSsid, WiFi.BSSID(), 6 );    // keep 6 bytes of BSSID (AP's MAC address)
    _M.WiFichannel=WiFi.channel();              // keep channel
    _M = UpdateWifiCredentials(_SysClock,_M);
                                                // connection status
    _M.WiFiStatus = Connected;                  // WiFi connected
    WiFi.localIP().toString().toCharArray(&_M.DeviceIP[0], 17);   // keep char version of IP
    #if   _DEBUGON==1
      _RunUtil.InfoStamp(_SysClock,Mname,G2,0,0); Serial.print(WiFi.localIP()); Serial.print(F(" Actual IP ")); Serial.print(_M.DeviceIP); 
      if (_M.StaticDynamicIP) Serial.print(F(" Dynamic IP"));
      else                    Serial.print(F(" Static IP"));
      Serial.print(F(" - END\n"));
    #endif  //_DEBUGON
    _M.HowLongItTook = 0;                        // clear retry counter
    _M.TimeMeasured = _RunClock.StartStopwatch();// start measuring for NTP
  }   // end of check for connection

  return  _M;
}     // end of IsWifiConnected

//****************************************************************************************//
void  WifiNet::WiFiCodePrint(uint8_t Index) {
  /*
    * method to convert WiFi status code to string and print it
    */
  static const char   code_str_0[] PROGMEM = "not connected";
  static const char   code_str_1[] PROGMEM = "trying to connect as station";
  static const char   code_str_2[] PROGMEM = "connected as station";
  static const char   code_str_3[] PROGMEM = "OTA server configured (waiting for client)";
  static const char   code_str_4[] PROGMEM = "Client connected as AP (OTA)";
  static const char   code_str_5[] PROGMEM = "WiFi connection lost";
  static const char*  code_str_tab[] PROGMEM = { code_str_0, code_str_1, code_str_2, code_str_3, code_str_4, code_str_5 };
  char* buf=new char[80]; // Temp buffer
  strcpy_P(buf,(char*)pgm_read_dword(&(code_str_tab[Index])));
  Serial.print(buf);
  delete [] buf;              // release buffer
}   // end of WiFiCodePrint

#if  _WIFINTPON==1
  //****************************************************************************************/
  TimePack  WifiNet::GetWWWTime (TimePack  SysClock, ManageWifi M) {
    /*
      * https://github.com/arduino-libraries/NTPClient
      * https://www.timeanddate.com/worldclock/linking.html
      * https://lastminuteengineers.com/esp8266-ntp-server-date-time-tutorial/
      * https://johnwargo.com/internet-of-things-iot/using-the-arduino-ntpclient-library.html
      * https://randomnerdtutorials.com/esp32-ntp-client-date-time-arduino-ide/
      * https://randomnerdtutorials.com/esp32-ntp-timezones-daylight-saving/
      * Israel Standard Time (IST) and Israel Daylight Time (IDT) are 2 hours ahead of the prime meridian in winter, springing forward an hour on 
      * March’s fourth Thursday at 26:00 (i.e., 02:00 on the first Friday on or after March 23), and falling back on October’s last Sunday at 02:00.
      * IST-2IDT,M3.4.4/26,M10.5.0 (https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html )
      * list of timezones https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
      * 
      * check if to use https://www.pjrc.com/teensy/td_libs_DateTime.html
      * Unix time (Epoch) starts on Jan 1 1970
      * UTC time starts Jan 1 1900
      * Difference 70 years = 2208988800UL = (17*366+53*365) * 24 * 60 * 60 sec
      * one day in sec's: 86400L=24*60*60
      * sec's today from 00:00am =(epoch%86400)
      * today's hour =(epoch%86400L)/3600 
      * return 1 - for time set
      *        0 - for time not set
      * 
      */
    #include <time.h>                       // https://mikaelpatel.github.io/Arduino-RTC/d8/d5a/structtm.html
    static const char Mname[] PROGMEM = "GetWWWTime:";
    #if _LOGGME==200
      static const char G0[] PROGMEM = "Starts";
    #endif  //_LOGGME
    static const char E0[] PROGMEM = "Failed to update time.";
    static const char L0[] PROGMEM = "GMT time=";
    static const char L1[] PROGMEM = "Local time=";
    static const char L2[] PROGMEM = "Time to acquire network time is ";
    const char *NTPserver1="pool.ntp.org";
    const char *NTPserver2="time.nist.gov";
    const char *NTPserver3="time.google.com";
    struct tm timeinfo;
    TimePack  _SysClock = SysClock;
    
    _SysClock.IsTimeSet = true;             // temporary flag
    #if _LOGGME==200
      _RunUtil.InfoStamp(_SysClock,Mname,G0,1,1); 
    #endif  //_LOGGME
    if ( _SysClock.NTPbeginOnce ) {         // perform only after reset
      #if _DEBUGON==200
        _RunUtil.InfoStamp(_SysClock,Mname,nullptr,0,0); Serial.print("1st entry _SysClock.NTPbeginOnce="); Serial.print(_SysClock.NTPbeginOnce); 
        Serial.print(" _SysClock.IsTimeSet="); Serial.print(_SysClock.IsTimeSet); Serial.print(F(" -END\n"));
      #endif  //_DEBUGON
      // This need to be performed only once connect to NTP server, with 0 TZ offset
      delay(NTPdelayAfterReset);
      configTime(0, 0, NTPserver1, NTPserver2, NTPserver3); // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      _SysClock.NTPbeginOnce=false;         // first time update at GMT TZ
      if(!getLocalTime(&timeinfo)){
        _SysClock.IsTimeSet = false;        // 1st time failure
        #if _LOGGME==1
          _RunUtil.InfoStamp(_SysClock,Mname,E0,1,0); Serial.print(F(" (1st) WiFiStatus: ")); WiFiCodePrint(M.WiFiStatus); 
          Serial.print(F(" (")); Serial.print(M.WiFiStatus); Serial.print(F(") - END\n"));
        #endif  //_LOGGME
      }   // end of time test 1
      #if _DEBUGON==200
        _RunUtil.InfoStamp(_SysClock,Mname,nullptr,0,0); Serial.print("After 1st entry _SysClock.IsTimeSet="); Serial.print(_SysClock.IsTimeSet); Serial.print(F(" -END\n"));
      #endif  //_DEBUGON
      if (!_SysClock.IsTimeSet) return _SysClock; // failure on test 1
      #if _LOGGME==1
        _RunUtil.InfoStamp(_SysClock,Mname,L0,1,0); Serial.print(timeinfo.tm_hour); Serial.print(":"); Serial.print(timeinfo.tm_min); Serial.print(":"); 
        Serial.print(timeinfo.tm_sec); Serial.print(" "); Serial.print(timeinfo.tm_mday); Serial.print("/"); Serial.print(timeinfo.tm_mon+1); Serial.print("/"); 
        Serial.print(timeinfo.tm_year-100); Serial.print(" DST="); Serial.print(timeinfo.tm_isdst); Serial.print(F(" -END\n"));
      #endif  //_LOGGME
    }     // end of NTPbeginOnce
    
                                            // change TZ to IST
    String  timezone="IST-2IDT,M3.4.4/26,M10.5.0";
    setenv("TZ",timezone.c_str(),1);        // set timezone
    tzset();
    if(!getLocalTime(&timeinfo)){           // seconf time update at IST TZ
      #if _LOGGME==1
        _RunUtil.InfoStamp(_SysClock,Mname,E0,1,0); Serial.print(F(" (2nd) WiFiStatus: ")); WiFiCodePrint(M.WiFiStatus); 
        Serial.print(F(" (")); Serial.print(M.WiFiStatus); Serial.print(F(") - END\n"));
      #endif  //_LOGGME
      _SysClock.IsTimeSet = false;
    }   // end of time test 2
    if (!_SysClock.IsTimeSet) return _SysClock; // failure on test 2
    
    #if _LOGGME==1
      static const char daysOfTheWeek[7][12] PROGMEM = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
      _RunUtil.InfoStamp(_SysClock,Mname,L1,1,0); Serial.print(timeinfo.tm_hour); Serial.print(":"); Serial.print(timeinfo.tm_min); Serial.print(":"); 
      Serial.print(timeinfo.tm_sec); Serial.print(" "); Serial.print(timeinfo.tm_mday); Serial.print("/"); Serial.print(timeinfo.tm_mon+1); Serial.print("/"); 
      Serial.print(timeinfo.tm_year-100); Serial.print(" "); Serial.print(daysOfTheWeek[timeinfo.tm_wday]); Serial.print("("); Serial.print(timeinfo.tm_wday); 
      Serial.print(") DST="); Serial.print(timeinfo.tm_isdst); 
      Serial.print(F(" -END\n"));
      _RunUtil.InfoStamp(_SysClock,Mname,L2,1,0); Serial.print(_RunClock.ElapseStopwatch(M.TimeMeasured)); Serial.print(F("mS - END\n"));
    #endif  //_LOGGME
                                                // time zone updated - load clock information
    _SysClock.clockHour = timeinfo.tm_hour;
    _SysClock.clockMin  = timeinfo.tm_min;
    _SysClock.clockSec  = timeinfo.tm_sec;
    _SysClock.clockYear = timeinfo.tm_year-100; 
    _SysClock.clockMonth = timeinfo.tm_mon+1;
    _SysClock.clockDay  = timeinfo.tm_mday;
    _SysClock.clockWeekDay = timeinfo.tm_wday;  // Sunday=0... Saturday=6
    
        return _SysClock;

  }     // end of GetWWWTime
#endif  //_WIFINTPON

//****************************************************************************************/
ManageWifi  WifiNet::startOTAWifiServer(TimePack  _SysClock, ManageWifi M){
  /*
    * method to initiate OTA Async web server over SAP to obtaine network credentials
    */
  static const char Mname[] PROGMEM = "startOTAWifiServer:";
  static const char L0[] PROGMEM = "Soft AP started=";
  ManageWifi _M=M;

  // set sot access point, initiate timer and wait for client to connect to SAP and provide configuration
  // https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/soft-access-point-class.rst
  bool  SAP=WiFi.softAP(SoftAccPntSSID);

  #if _LOGGME==1
    _RunUtil.InfoStamp(_SysClock,Mname,L0,1,0); Serial.print(SAP); Serial.print(F(" Soft Access Point IP=")); Serial.print(WiFi.softAPIP()); 
    Serial.print(F(" -END\n"));
  #endif  //_LOGGME
  _M.WiFiStatus=Configure_OTA;              // AP (OTA)configured as server, waiting for client to connect
  return  _M;
}   // end of startOTAWifiServer
  
//****************************************************************************************/
void  WifiNet::whileWait4Wifi(ManageWifi M){
  /*
    * method to indicate wait period once waiting for client toconnect to the wifi network
    * it counts to 20 (aprox 2 Sec) then prints wait pattern by status
    */
  static  uint8_t liner=0;
  static  uint8_t counter=0;  
  static  bool    flagger=0;
  uint8_t         endOfLine=100;
  
  counter++;
  if (counter>19) {                               // 2 sec
    #if _LOGGME==1
      flagger = !flagger;
      switch  (M.WiFiStatus) {
        case  Connected:                          // connected (do nothing)
          break;
        case  Trying_Connect:                     //wait for connection
          if ( flagger )  Serial.print(F("."));
          else            Serial.print(F("."));
          endOfLine=100;
          break;
        case  Configure_OTA:                      // wait for OTA credentials
          if ( flagger )  Serial.print(F("."));
          else            Serial.print(F("+"));
          endOfLine=70;
          break;
        case  Client_Connect_OTA:                 // client connected to OTA server
          if ( flagger )  Serial.print(F(":"));
          else            Serial.print(F("x"));
          break;
        case  Not_Connected:
        case  Connection_lost:
          endOfLine=100;
          break;
        default:                                  // other status - Error
          if ( flagger )  Serial.print(F("?"));
          else            Serial.print(F("&"));
          endOfLine=100;
          break;
      }   // end of print switch
    #endif  //_LOGGME
    liner++;    
    if ( liner==endOfLine ) {                     // start new line
      liner=0;
      counter=0;
      flagger=0;
      #if _LOGGME==1
        Serial.print(F("\n"));
        if ( M.WiFiStatus == Configure_OTA || M.WiFiStatus == Client_Connect_OTA ) {
          Serial.print(F("Connect to SSID: ")); Serial.print(SoftAccPntSSID); Serial.print(F(" at IP: ")); 
          Serial.print(WiFi.softAPIP()); Serial.print(F(" ")); 
        }
      #endif  //_LOGGME
    } // end of wait end-of-line

  }   // end 2 sec edge
}     // end of whileWait4Wifi

//****************************************************************************************/
ManageWifi   WifiNet::fetchCredFromEEPROM(TimePack _SysClock, ManageWifi M){
  /*
    * Procedure to fetch credentials from EEPROM and load <ssid> <password> <bssid> <channel>
    * <M.CredStat>  2 for pre programmed EEPROM with all parameters
    *               1 for only SSID and password pre programmed
    *               0 for EEPROM not programmed (credentials are NOT overridden)
    * 
      * EEPROM Memory structure:
      * 0 1 SSIDlength PASSlength        6      1
      * +--+----------+-------------+----------+--+
      * | ?| SSID     | Password    | BSSID(6) |Ch|
      * +--+----------+-------------+----------+--+
      * First byte is '?' - not set
      *               '+' - SSID and password set, BSSID and channel not set 
      *               '*' - all set
    */
  static const char Mname[] PROGMEM = "fetchCredFromEEPROM:";
  static const char L0[] PROGMEM = "EEPROM status:";
  static const char L1[] PROGMEM = "EEPROM SSID:";
  static const char L2[] PROGMEM = "EEPROM BSSID:";
  static const char L3[] PROGMEM = "Not configured!";
  static const char L4[] PROGMEM = "Partially configured.";
  static const char L5[] PROGMEM = "Fully configured.";
  ManageWifi _M=M;
  
  // 1. reset EEPROM to config by firmware code
  #ifdef  CLEAREEPROM                
    ClearEEPROMwifiCredentials();
  #endif  //CLEAREEPROM
  // 2. check integrity of previous write to EEPROM
  if ( char(EEPROM.read(0))=='*' )      { _M.CredStat=2;      // for fully programmed EEPROM
  } else if (char(EEPROM.read(0))=='+') { _M.CredStat=1;      // for partially programmed EEPROM
  } else                                { _M.CredStat=0;      // EEPROM was NOT pre-programmed
  } // end integrity check
  #if _LOGGME==1
    _RunUtil.InfoStamp(_SysClock,Mname,L0,1,0); 
    switch (_M.CredStat) {
      case  0:  Serial.print(L3); break;
      case  1:  Serial.print(L4); break;
      case  2:  Serial.print(L5); break;
      default:  break;            // error
    }
    Serial.print(F(" -END\n"));
  #endif  //_LOGGME

  if ( _M.CredStat !=0 ) {                // fetch credenial (if EEPROM programmed)
    // 3. read SSID configuration from EEPROM: byte[1] to [SSIDlength]
    for (uint8_t i=1; i <= SSIDlength; ++i) { 
      _M.Ssid[i-1]=char(EEPROM.read(i)); 
      _M.Ssid[i]='\0';      // set termination
      //Serial.print(F(" i ")); Serial.print(i);Serial.print(F(" S(i-1) ")); Serial.print(Ssid[i-1],HEX); Serial.print(F(" i ")); Serial.println(Ssid[i],HEX);
      }
    // 4. read PASSWORD configuration from EEPROM: byte[SSIDlength+1] to [SSIDlength+PASSlength]
    for (uint8_t i=0; i < PASSlength; ++i) { 
      _M.Password[i] = char(EEPROM.read(SSIDlength+i)); 
      _M.Password[i+1]='\0'; 
      }
    #if _LOGGME==1
      _RunUtil.InfoStamp(_SysClock,Mname,L1,1,0); Serial.print(_M.Ssid); Serial.print(F(" PW:")); Serial.print(_M.Password); Serial.print(F(" -END\n"));
    #endif  //_LOGGME
    // 5. read BSSID and channel configuration from EEPROM: byte[SSIDlength+PASSlength+1] to [SSIDlength+PASSlength+7] and [SSIDlength+PASSlength+8]
    if ( _M.CredStat==2 ) {
      #if _LOGGME==1
        _RunUtil.InfoStamp(_SysClock,Mname,L2,1,0);
      #endif  //_LOGGME
      for (uint8_t i = 0; i < 6 ; ++i) { 
        _M.WiFiBSsid[i] = byte(EEPROM.read(SSIDlength+PASSlength+1+i)); 
        #if _LOGGME==1
          Serial.print(_M.WiFiBSsid[i],HEX); Serial.print(F(":"));
        #endif  //_LOGGME
      }
      _M.WiFichannel = byte(EEPROM.read(SSIDlength+PASSlength+1+6+1));
      #if _LOGGME==1
        Serial.print(F(" EEPROM Ch:")); Serial.print(_M.WiFichannel); Serial.print(F(" -END\n"));
      #endif  //_LOGGME
    } // end of BSSID and Ch fetch
  }   // end of fetch

  return  _M;
}   // end of fetchCredFromEEPROM
  
//****************************************************************************************/
  ManageWifi  WifiNet::UpdateWifiCredentials(TimePack _SysClock, ManageWifi M){
  /*
    * metod to store credentials in EEPROM by current credential status (avoid accessive rewrites)
    */
  //static const char Mname[] PROGMEM = "UpdateWifiCredentials:";
  ManageWifi _M=M;
  switch  (_M.CredStat) {
    case  0:        // credentials not set at all
      KeepCredentialsEEPROM( _SysClock,_M.Ssid,_M.Password );
      KeepChaBssidEEPROM( _SysClock,_M.WiFiBSsid,_M.WiFichannel );
      break;
    case  1:        // complete missing credentials
      KeepChaBssidEEPROM( _SysClock,_M.WiFiBSsid,_M.WiFichannel );
      break;
    case  2:        // all set - do nothing
      break;
    default:        // error
      break;
  } // end of switch
  return  _M;
}   // end of UpdateWifiCredentials
        
//*********************************************************************************/
bool  WifiNet::ClearEEPROMwifiCredentials(TimePack _SysClock) {
  /*
    * Method to clear EEPROM credentials
    */
  static const char Mname[] PROGMEM = "ClearEEPROMwifiCredentials:";
  static const char L0[] PROGMEM = "Writing to EEPROM completed:";
  for (uint8_t i = 0; i < SSIDlength+PASSlength+6+1+2; ++i) EEPROM.write(i, '?');
  EEPROM.commit();
  #if _LOGGME==1
    _RunUtil.InfoStamp(_SysClock,Mname,L0,1,0); 
    for (uint8_t i = 0; i < SSIDlength+PASSlength+1 ; ++i) { Serial.print(char(EEPROM.read(i))); }
    Serial.print(F(" -END\n"));
  #endif  //_LOGGME
  return  true;       // indictae (and report) that EEPROM was cleared
}     // end of ClearEEPROMwifiCredentials
  
//****************************************************************************************/
bool  WifiNet::KeepCredentialsEEPROM ( TimePack  _SysClock, char* id, char* psw ){
  static const char Mname[] PROGMEM = "KeepCredentialsEEPROM:";
  static const char L0[] PROGMEM = "Received credentials SSID:";
  static const char L1[] PROGMEM = "EEPROM cleared, SSID:";
  /*
    * Procedure to store <id. and <ps> credentials in EEPROM
    *      returns 1 - stored OK
    *              0 - wrong lengths of parameters
    *
    * EEPROM Memory structure:
    * 0 1 SSIDlength PASSlength        6      1
    * +--+----------+-------------+----------+--+
    * | ?| SSID     | Password    | BSSID(6) |Ch|
    * +--+----------+-------------+----------+--+
    * First byte is '?' - not set
    *               '+' - SSID and password set, BSSID and channel not set 
    *               '*' - all set
    */
  bool    returnFlag;
  char*   pntr;

  // 0. check input
  if ( strlen(id) > 0 && strlen(psw) > 0) {
    #if _LOGGME==1
      _RunUtil.InfoStamp(_SysClock,Mname,L0,1,0); Serial.print(id); Serial.print(F(" Pass: ")); Serial.print(psw); Serial.print(F(" -END\n"));
    #endif //_LOGGME
    for (uint8_t i = 0; i < SSIDlength+PASSlength+1; ++i) // clear EEPROM record
      { EEPROM.write(i,0x00); }
    
    // 1. store ssid
    pntr = id;
    #if _LOGGME==1
      _RunUtil.InfoStamp(_SysClock,Mname,L1,1,0); Serial.print(pntr);
    #endif //_LOGGME
    for (uint8_t i = 0; i < strlen(id); ++i) {EEPROM.write( i+1, *pntr++); } // end of writing SSID
      
    // 2. store passowd
    pntr = psw;
    #if _LOGGME==1
      Serial.print(F(" Pass: ")); Serial.print(psw);
    #endif  //_LOGGME
    for (uint8_t i = 0; i <strlen(psw); ++i) {EEPROM.write( SSIDlength+i, *pntr++ );} // end of writing pw
    
    // 3. indicate store completed
    EEPROM.write( 0, '+' );
    EEPROM.commit();
    #if _LOGGME==1
      Serial.print(F(" -END\n"));
    #endif  //_LOGGME

    returnFlag=true;
  } else {
    // wrong logical checks (length)
    returnFlag=false;
  }
  
  return  returnFlag;
}     // end of KeepCredentialsEEPROM
  
//****************************************************************************************/
bool  WifiNet::KeepChaBssidEEPROM (TimePack _SysClock, uint8_t Bssid[], uint8_t Channel){
  /*
    * Procedure to store credentials <bssid> and <Channel> in EEPROM and complete the setup
    *      returns 1 - stored OK
    *              0 - wrong lengths of parameters
    *
    * EEPROM Memory structure:
    * 0 1 SSIDlength PASSlength        6      1
    * +--+----------+-------------+----------+--+
    * | ?| SSID     | Password    | BSSID(6) |Ch|
    * +--+----------+-------------+----------+--+
    * First byte is '?' - not set
    *               '+' - SSID and password set, BSSID and channel not set 
    *               '*' - all set
    */
  static const char Mname[] PROGMEM = "KeepChaBssidEEPROM:";
  static const char L0[] PROGMEM = "writing EEPROM BSSID:";
  bool    returnFlag;
  
  // 1. store Bssid
  #if _LOGGME==1
    _RunUtil.InfoStamp(_SysClock,Mname,L0,1,0); 
  #endif //_LOGGME
  for (uint8_t i = 0; i < 6 ; ++i ) {
    EEPROM.write( SSIDlength+PASSlength+1+i, byte(Bssid[i]));
    #if _LOGGME==1
      Serial.print(Bssid[i],HEX);Serial.print(F(":"));
    #endif  //_LOGGME
  } // end of writing BSSID
    
  // 2. store channel
  #if LOGGME==1
    Serial.print(F(" Channel: "));
  #endif  //_LOGGME
  EEPROM.write( SSIDlength+PASSlength+1+6+1 , byte(Channel) );
  #if _LOGGME==1
    Serial.print(Channel);
  #endif  //_LOGGME
        
  // 3. indicate store completed
  EEPROM.write( 0, '*' );
  EEPROM.commit();
  #if _LOGGME==1
    Serial.print(F(" -END\n"));
  #endif  //_LOGGME

  returnFlag=true;
  return  returnFlag;

}     // end of KeepChaBssidEEPROM

//****************************************************************************************/
ManageWifi WifiNet::ServiceOTACred(AsyncWebServerRequest *request, TimePack _SysClock, ManageWifi M) {
  /*
    * Async server handler to deal with credential inputs (called from <IoTWEBserver.on>)
    * if both SSID and password are set (<OTACredStat> should be 3):
    * - respond to client
    * - store the SSID and password
    * - set <M.activeTimeEvent> to 4, to reset the system by calling method
    */
  static const char  Mname[] PROGMEM ="ServiceCred/setting:";  // setting of credenials (input)
  static const char L0[] PROGMEM = "Credentials received <";
  static const char L1[] PROGMEM = "Credential store complete.<br>Resetting platform. Bye bye";
  static const char L2[] PROGMEM = "Credential input form";
  static const char L3[] PROGMEM = "Input is incomplete.<br> Try again!";
  static const char E0[] PROGMEM = "Credential input is incomplete. OTACredStat=";
  static const char E1[] PROGMEM = "Program error. OTACredStat=";
  ManageWifi _M = M;
  
  uint8_t OTACredStat=0;
  uint8_t option;

  if (request->hasParam(SSID_Phrase)) {             // check for SSID field
      request->getParam(SSID_Phrase)->value().toCharArray(_M.Ssid,SSIDlength);
      if ( _M.Ssid[0] != 0x00 ) OTACredStat+=1;     // check for empty parameter
  } // end SSID
  
  if (request->hasParam(PSWD_Phrase)) {             // check for pasword field
      request->getParam(PSWD_Phrase)->value().toCharArray(_M.Password,PASSlength);;
      if ( _M.Password[0] != 0x00 ) OTACredStat+=2; // check for empty parameter
  } // end password

  // respond to client
  char* buf = new char[1000];                         // Temp buffer
  switch ( OTACredStat ) {
    case  0:                                          // no input at all
    case  1:                                          // only SSID
    case  2:                                          // only PW
      #if _LOGGME==1
        _RunUtil.InfoStamp(_SysClock,Mname,E0,1,0); Serial.print(OTACredStat); Serial.print(F(" -END\n"));
      #endif //_LOGGME
      option = 2;                                     // for feedback form
      buf=SimpleUtilityPage(_SysClock,_M,buf,option,L2,L3,nullptr);
      request->send(200,_TextHTML,buf);
      break;
    case  3:                                          // input complete
      #if _LOGGME==1
        _RunUtil.InfoStamp(_SysClock,Mname,L0,1,0); Serial.print(_M.Ssid); Serial.print(F("> -<")); 
        Serial.print(_M.Password); Serial.print(F("> (OTACredStat=")); Serial.print(OTACredStat); Serial.print(F(") -END\n"));
      #endif //_LOGGME
                                                      // store credential in EEPROM
      KeepCredentialsEEPROM( _SysClock,_M.Ssid,_M.Password );
      _M = fetchCredFromEEPROM(_SysClock, _M);        // test read EEPROM
      _M.activeTimeEvent = 4;                         // set event to reset the platform
      #if _LOGGME==1
        _RunUtil.InfoStamp(_SysClock,Mname,L1,1,1);
      #endif //_LOGGME
      option = 2;                                     // for feedback form
      buf=SimpleUtilityPage(_SysClock,_M,buf,option,L2,L1,nullptr);
      request->send(200,_TextHTML,buf);
      break;
    default:                                          // program error
      _RunUtil.InfoStamp(_SysClock,Mname,E1,1,0); Serial.print(OTACredStat); Serial.print(F(") -END\n"));
      break;

  }   // end of cred status switch  
  delete [] buf;                                      // release buffer

    return  _M;
}   // end of ServiceOTACred

//****************************************************************************************/
char*  WifiNet::SimpleUtilityPage(TimePack _SysClock, ManageWifi M, char* buf, uint8_t option, 
                        const char* PageTitleName, const char* FeedBack, const char* insert_action){
  /*
    * method to create htmp page to be used in a simple applications, set by <option>
    * <option>  - 0 default root page
    *           - 1 Error 404
    *           - 2 display feedback
    *           - 3 Credentials input form
    *           - 4 Credential input acknowledge
    *           - 5 TBD
    *
    * format:
    * line 0:     whoamI+version+00:00:00   (current time)
    * line 1:     page title                <PageTitleName>
    * line 2:     - Feedback                <FeedBack>
    *             - credential form
    *                 to collect network credentials
    *                 <insert_action> defines the action (without /)
    *                 <SSID_Phrase> <PSWD_Phrase> define the field names for the input
    *                 http://192.168.4.1/setting?SSID=Sachi&Pass=Kalisher46apt7 (eg. setting, SSID, Pass)
    */
  // color picker https://htmlcolorcodes.com/color-picker/
  //static const char Mname[] PROGMEM = "CreateCredForm:";
  static const char S_Title[] PROGMEM = "<html><head><title>ESP8266 Utilities</title>";
  static const char S_Style[] PROGMEM = "<style>body {background-color: #9DCDF4; font-family: Arial, Helvetica, Sans-Serif; Color: blue;}</style></head>\n"; //#E6E6FA #33A4FF
  static const char Body_H[]  PROGMEM = "<body>";
  static const char Body_E[]  PROGMEM = "</body></html>";
  static const char S_H1[]    PROGMEM = "<h1>";
  static const char S_H1E[]   PROGMEM = "</h1>";
  static const char S_H2[]    PROGMEM = "<h2>";
  static const char S_H2E[]   PROGMEM = "</h2>";
  static const char S_Form[]  PROGMEM = "<form method='get' action='";
  static const char S_Form1[] PROGMEM = "'>&nbsp;&nbsp;&nbsp;&nbsp;Network SSID:<input type='text' name='";
  static const char S_Form2[] PROGMEM = "'><BR><BR>Network password:<input type='text' name='";
  static const char S_Form3[] PROGMEM = "'>&nbsp;<input type='submit' value='Enter'></form>";
  static const char Msg1[]    PROGMEM = "<h1>SSID and code obtaine are:</h1>";
  static const char Msg2[]    PROGMEM = "<h1>System is about to boot. Bye bye</h1>";

  char*  actionWOslash;
  char* action = new char[15];                          // Temp buffer to hold action and time stamp
  
  strcpy_P(buf,S_Title);                                // title and style
  strcat_P(buf,S_Style);
  strcat_P(buf,Body_H);                                 // body

  strcat_P(buf,S_H2);                                   // Line 0 
  strcat  (buf,M.WhoAmI);                               // platform name
  strcat_P(buf,_G3);                                    // ' '
  strcat  (buf,M.Version);                              // version
  strcat_P(buf,_G3);
  strcat  (buf,_RunUtil.TimestampToString(_SysClock,action));// get current time
  strcat_P(buf,S_H2E);

  strcat_P(buf,S_H1);                                   // Line 1 
  strcat_P(buf,PageTitleName);                          // page title name
  strcat_P(buf,S_H1E);

  strcpy (action,CredSettingTrigger);
  actionWOslash=action+1;                               // temp hold action w/o'/'

  switch ( option ) {                                   // line 2
    case  0:                                            // default root
    case  1:                                            // error 404
    case  2:                                            // message from application
      strcat_P(buf,S_H1);
      strcat_P(buf,FeedBack);                           // message from application
      strcat_P(buf,S_H1E);
      break;
    case  3:                                            // credential input form
      strcat_P(buf,S_Form);
      strcat_P(buf,actionWOslash);                      // insert 'action' field
      strcat_P(buf,S_Form1);
      strcat_P(buf,SSID_Phrase);                        // insert SSID field name
      strcat_P(buf,S_Form2);
      strcat_P(buf,PSWD_Phrase);                        // insert pasword field name
      strcat_P(buf,S_Form3);
      break;
    case  4:                                            // credential input acknowledge
      strcat_P(buf,Msg1);                               // SSID and PW header
      strcat_P(buf,S_H1);
      strcat  (buf,M.Ssid);                             // print SSID
      strcat_P(buf,_G3);
      strcat  (buf,M.Password);                         // print passcode
      strcat_P(buf,S_H1E);
      strcat_P(buf,Msg2);                               // bye bye
      break;
    case  5:                                            // do nothing
    default:
      break;
  }
  strcat_P(buf,Body_E);
  #if _DEBUGON==250
    _RunUtil.InfoStamp(_SysClock,insert_action,nullptr,0,0); Serial.print(buf); Serial.print(F(" - END\n"));
  #endif //_DEBUGON
      
  delete [] action;                                     // release buffer
  return  buf;
}     // end of SimpleUtilityPage

//****************************************************************************************/
bool  WifiNet::storeIPaddress(TimePack _SysClock, char* IPstring, uint16_t EEPaddress){
  /*
   * method to store IP address <IPstring> (up to 15 chars) at EEPROM starting address <EEPaddress>
   * returns  0 for write error or wrong input length
   *          1 stored OK
   */
  static const char Mname[] PROGMEM = "storeIPaddress:";
  uint16_t  Address = EEPaddress;
  char*     pntr = IPstring;
  if ( strlen(IPstring)>15 ) {                  // error, input too long
    #ifdef _LOGGME
      _RunUtil.InfoStamp(_SysClock,Mname,_G7,1,0); Serial.print(F("IP string too long="));
      Serial.print(strlen(IPstring)); Serial.print(F(" - END\n"));
    #endif //_LOGGME
    return  false;
  }   // end of length check
  
  for ( uint8_t ii=0; ii<strlen(IPstring); ii++ ) EEPROM.write(Address++, *pntr++);
  EEPROM.write(Address, 0x00);      // termination

  if ( EEPROM.commit() ) {          // write OK
    return  true;
  } else {                          // write bad
    #ifdef _LOGGME
      _RunUtil.InfoStamp(_SysClock,Mname,_G7,1,0); Serial.print(F("IP string could not be written to EEPROM - END\n"));
    #endif //_LOGGME
    return  false;
  } // end of commit
}     //end of storeIPaddress

//****************************************************************************************/
char*  WifiNet::fetchIPaddress(char* buff, uint16_t EEPaddress){
  /*
   * method to fetch IP address (up to 15 chars) from EEPROM starting address <EEPaddress>
   * and loads it to <buff> (must be at least 16 chars long)
   * returns a pointer to the IP string. For error returns null string
   */
  //static const char Mname[] PROGMEM = "fetchIPaddress:";
  uint16_t  Address = EEPaddress;
  char*     pbuf = buff;

  for ( uint8_t ii=0; ii<15; ii++ ) {
    *pbuf = EEPROM.read(Address++);
    if ( *pbuf == 0x00 ) break;         // end of string
    pbuf++;
    *pbuf = 0x00;                       // last char NULL
  }     // end of loop

    return  buff;
}     // end of fetchIPaddress

//****************************************************************************************/
bool    WifiNet::CompareAndKeepIP (TimePack _SysClock,ManageWifi M) {
  /*
   * method to compare EEPROM kept IP address to current network IP address.
   * if equal     - returns   0
   * if different - returns   1 and store current network IP in EEPROM
   * note: the method does not check for successful EEPROM store
   */
  
  #if _DEBUGON==1
    static const char Mname[] PROGMEM = "CompareAndKeepIP:";
    static const char L0[] PROGMEM = "Stored IP:";
  #endif //_DEBUGON
  char  buf[16];
  bool  comp;
  
  fetchIPaddress(buf,EEPROMipAddress);                            // fetch IP from EEPROM
  comp = strcmp(buf,M.DeviceIP);                            // compare to current
  #if _DEBUGON==1
    _RunUtil.InfoStamp(_SysClock,Mname,L0,0,0); Serial.print(buf); Serial.print(F(" network IP:")); Serial.print(M.DeviceIP); 
    if ( comp )   Serial.print(F(" addresses are different."));
    else          Serial.print(F(" addresses are the same."));
    Serial.print(F(" -END\n"));
  #endif //_DEBUGON
  if ( comp ) {                                                   // keep the new address
    storeIPaddress(_SysClock, M.DeviceIP, EEPROMipAddress);
    return    true;                                               // alert for change
  } else {
    return    false;                                              // all OK
  }   // end of compare check 
}     // end of CompareAndKeepIP

//****************************************************************************************/