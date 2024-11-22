/****************************************************************************************/
void  AsyncSimpleServerHandlers(){
  /*
    * method to define Async WEB server handlers
    */
  static const char Mname[] PROGMEM = "v:";
  static const char L0[] PROGMEM = "IoTserver initiated";
  // declare handlers
  //......................................................................................./
  //for request not found
  IoTWEBserver.onNotFound([] (AsyncWebServerRequest *request)
    {                                                         // Process Not Found
    static const char Mname[] PROGMEM = "onNotFound:";
    static const char E0[] PROGMEM = "Request not found (404) url:";
    static const char E1[] PROGMEM = "Param(";
    static const char E404Title[] PROGMEM = "Page does not exist";
    static const char E404FeedBack[] PROGMEM = "Error 404, please try again";
    char* buf = new char[HTMLpageBufLen];     // Temp buffer
    #ifdef LOGGME
      RunUtil.InfoStamp(SysClock,Mname,E0,1,0); Serial.print(request->url()); Serial.print(F(" -END\n"));
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        RunUtil.InfoStamp(SysClock,Mname,E1,1,0); Serial.print(i);        
        if      (p->isFile()) { Serial.printf(" FILE[%s]: %s, size: %u", p->name().c_str(), p->value().c_str(), p->size()); } 
        else if (p->isPost()) { Serial.printf(") POST[%s]: %s", p->name().c_str(), p->value().c_str());} 
        else                  { Serial.printf(") GET[%s]: %s", p->name().c_str(), p->value().c_str()); }
        Serial.print(F(" -END\n"));
      } // end of param loop
    #endif LOGGME
    request->send(404, TextHTML, RunWifi.SimpleUtilityPage(SysClock,SysWifi,buf,1,E404Title,E404FeedBack,Mname));
    delete [] buf;                                // release buffer
  });
  //......................................................................................./
  // for root
  IoTWEBserver.on("/", HTTP_GET, [] (AsyncWebServerRequest *request)
    { ServiceRoot(request);    });                           // Process '/'
  //......................................................................................./ 
  // for erase EEPROM credentials
  IoTWEBserver.on("/erase", HTTP_GET, [] (AsyncWebServerRequest *request)
    { ServiceClrEEPROM(request);    });                       // Process '/erase'
  //......................................................................................./ 
  // to reset the system
  IoTWEBserver.on("/ResetSystem", HTTP_GET, [] (AsyncWebServerRequest *request)
    { ServiceResetSystem(request);    });                     // Process '/ResetSystem' 
  //......................................................................................./ 
  // to test the system
  IoTWEBserver.on("/test", HTTP_GET, [] (AsyncWebServerRequest *request)
    {                                                         // Process '/test' 
        static const char Mname[] PROGMEM = "Testing simple page  (/test):";
        static const char CredTitle[] PROGMEM = "Credenitial input form";
        static const char CredFeedBack[] PROGMEM = "-";
        char* buf = new char[HTMLpageBufLen];                 // Temp buffer
        request->send(200, TextHTML, RunWifi.SimpleUtilityPage(SysClock,SysWifi,buf,3,CredTitle,"",CredSettingTrigger));
        #ifdef LOGGME
          RunUtil.InfoStamp(SysClock,Mname,G1,1,0); Serial.print(buf); Serial.print(F(" - END\n"));
        #endif LOGGME
        delete [] buf;                                // release buffer
  });
  //......................................................................................./ 
  //......................................................................................./
  #ifdef  OTAWIFICONFIG
    //......................................................................................./
    // for credentials input form for OTA
    IoTWEBserver.on("/cred", HTTP_GET, [] (AsyncWebServerRequest *request) {  // Process '/cred' 
        static const char Mname[] PROGMEM = "Credential input form (/cred):";
        static const char CredTitle[] PROGMEM = "Credenitial input form";
        char* buf = new char[HTMLpageBufLen];                 // Temp buffer
        request->send(200, TextHTML, RunWifi.SimpleUtilityPage(SysClock,SysWifi,buf,3,CredTitle,"",CredSettingTrigger));
        #ifdef LOGGME
          RunUtil.InfoStamp(SysClock,Mname,G1,1,0); Serial.print(buf); Serial.print(F(" - END\n"));
        #endif LOGGME
        delete [] buf;                                // release buffer
    });
    //......................................................................................./
    // for setting of credenials      
    IoTWEBserver.on(CredSettingTrigger, HTTP_GET, [] (AsyncWebServerRequest *request) { // process '/setting'
        SysWifi = RunWifi.ServiceOTACred(request,SysClock,SysWifi);   
    }); 
  #endif  OTAWIFICONFIG

    //......................................................................................./    
  #ifdef  OTAelegantServer
    ElegantOTA.begin(&IoTWEBserver);                                              // Start ElegantOTA
                                                                                  // ElegantOTA callbacks
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);
    ElegantOTA.setAutoReboot(false);                                              // Disable Auto Reboot post update
  #endif  OTAelegantServer
  
  IoTWEBserver.begin();                                                           // initiate Async WEB server
  #ifdef LOGGME
    RunUtil.InfoStamp(SysClock,Mname,L0,1,1); 
  #endif LOGGME     
}     // end of AsyncSimpleServerHandlers 

/****************************************************************************************/
#ifdef  OTAelegantServer
  unsigned long ota_progress_millis = 0;              // progress counter
  static const char HOTAname[] PROGMEM = "ElegantOTA handler:";
  //......................................................................................./
    void onOTAStart() {                               // Log when OTA has started
    #ifdef LOGGME
      RunUtil.InfoStamp(SysClock,HOTAname,G0,1,0); Serial.print(F("OTA update started! - END\n"));
    #endif LOGGME
    SysWifi.uploadedFileLen = 0;                      // init file length
    // <Add more code here>
  }   // end of onOTAStart
  //......................................................................................./
  void onOTAProgress(size_t current, size_t final) {  // OTA progress: Log every 1 second
    if (millis() - ota_progress_millis > 1000) {
      ota_progress_millis = millis();
      #ifdef LOGGME
        RunUtil.InfoStamp(SysClock,HOTAname,G0,1,0); Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes", current, final); Serial.print(F(" - END\n"));
      #endif LOGGME
    }
    SysWifi.uploadedFileLen = final;                // keep file length
  }   // end of onOTAProgress
  //......................................................................................./
  void onOTAEnd(bool success) {                     // Log when OTA has finished
    static const char M0[] PROGMEM = "OTA file upload length ";
    static const char M1[] PROGMEM = "Message end";
    if (success) {
      SysWifi.uploadFileRady = true;                              // indicate file upload completed
      #ifdef LOGGME
        RunUtil.InfoStamp(SysClock,HOTAname,G2,1,0); Serial.print(F("OTA update ended successfully! File length "));
          Serial.print(SysWifi.uploadedFileLen); Serial.print(F(" - END\n"));
      #endif LOGGME
    } else {
      SysWifi.uploadFileRady = false;                             // indicate file upload did not completed
      #ifdef LOGGME
        RunUtil.InfoStamp(SysClock,HOTAname,G2,1,0); Serial.print(F("OTA update ended with an error during OTA update! - END\n"));
      #endif LOGGME
    }
    // post upload
  }     // end of onOTAEnd
  //......................................................................................./
#endif  OTAelegantServer
  
/****************************************************************************************/
void ServiceRoot(AsyncWebServerRequest *request){
/*
  * method to service HTML GET command for root '/'
  */
  static const char Mname[] PROGMEM = "ServiceRoot(/):";
  static const char D0[] PROGMEM = "\n\n\nDefault page in test mode\n";
  char* buf = new char[HTMLpageBufLen];                 // Temp buffer
  if ( SysWifi.WiFiStatus==Configure_OTA ) {
    #ifdef  OTAWIFICONFIG                               // for credentials input form for OTA      
      static const char CredTitle[] PROGMEM = "Credenitial input form";
      request->send(200, TextHTML, RunWifi.SimpleUtilityPage(SysClock,SysWifi,buf,3,CredTitle,"",CredSettingTrigger));
    #endif  OTAWIFICONFIG
  } else {
    static const char RootTitle[] PROGMEM = "Default Root page in test mode";
    static const char RootFeedBack[] PROGMEM = "Try another input";
    request->send(200, TextHTML, RunWifi.SimpleUtilityPage(SysClock,SysWifi,buf,0,RootTitle,RootFeedBack,Mname));
  }
  #ifdef DEBUGON1
    RunUtil.InfoStamp(SysClock,Mname,G1,0,0); Serial.print(buf); Serial.print(F(" - END\n"));
  #endif DEBUGON1
  delete [] buf;                                        // release buffer
}     // end of ServiceRoot

/****************************************************************************************/
void ServiceClrEEPROM(AsyncWebServerRequest *request){
  /*
   * method to service '/erease' to clear EEPROM credentials
   */
  static const char Mname[] PROGMEM = "ServiceClrEEPROM(/erase):";
  static const char D0[] PROGMEM = "EEPROM cleared successfully<br>Go back to root";
  static const char E0[] PROGMEM = "Error while EEPROM clearing!!!!<br>Go back to root";
  char* buf = new char[HTMLpageBufLen];                 // Temp buffer
  if ( RunWifi.ClearEEPROMwifiCredentials(SysClock) ) {         // successful erase
    request->send(200, TextHTML, RunWifi.SimpleUtilityPage(SysClock,SysWifi,buf,2,Mname,D0,Mname));
    #ifdef LOGGME
      RunUtil.InfoStamp(SysClock,Mname,G1,1,0); Serial.print(F("EEPROM cleared successfully - END\n"));
    #endif LOGGME
  } else  {                                                     // for error
    request->send(200, TextHTML, RunWifi.SimpleUtilityPage(SysClock,SysWifi,buf,2,Mname,E0,Mname));
    #ifdef LOGGME
      RunUtil.InfoStamp(SysClock,Mname,G1,1,0); Serial.print(F("Error while EEPROM clearing!!!! - END\n"));
    #endif LOGGME
  }
  delete [] buf;                                        // release buffer
}     // end of ServiceClrEEPROM

/****************************************************************************************/
void ServiceResetSystem(AsyncWebServerRequest *request){
  /*
   * method to service '/ResetSystem' to reset the system
   */
  static const char Mname[] PROGMEM = "ServiceResetSystem(/ResetSystem):";
  static const char D0[] PROGMEM = "Received reset system commnad<br>Wait util system reset.<br>Bye bye";
  char* buf = new char[HTMLpageBufLen];                 // Temp buffer
  request->send(200, TextHTML, RunWifi.SimpleUtilityPage(SysClock,SysWifi,buf,2,Mname,D0,Mname));
  #ifdef LOGGME
    RunUtil.InfoStamp(SysClock,Mname,G1,1,0); Serial.print(F("Received reset system commnad - END\n"));
  #endif LOGGME
  SysWifi.activeTimeEvent = 4;                // semaphore to reset the system
  delete [] buf;                                        // release buffer
}     // end of ServiceResetSystem

/****************************************************************************************/

 