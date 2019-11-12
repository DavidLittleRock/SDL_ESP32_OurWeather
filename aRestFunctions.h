//
//
//
// aRest Command functions
//
//


void updateDisplay(int displayMode);


int ledControl(String command) {

  // Get state from command
  int state = command.toInt();

  if (state == 0)  // ESP8266 inverts sense (HIGH = off, LOW = ON)
    state = 1;
  else
    state = 0;

  digitalWrite(blinkPin, state);
  return 1;
}

int setWeatherPlusIDControl(String command) {



  rest.set_id(command);
  return 1;
}

int resetWiFiAccessPoint(String command)
{
  Serial.print("Command =");
  Serial.println(command);




  if (command == adminPassword)
  {
    WiFi_psk = "";
    WiFi_SSID = "";
    writePreferences();
    return 1;
  }
  else
    return 0;


  //http://192.168.1.134/resetWiFiAccessPoint?params=adminpassword
  return 1;


}

int resetOurWeather(String command) {

  Serial.println("resetOurWeather - settings invalidated");
  Serial.print("Command =");
  Serial.println(command);
  if (command == adminPassword)
  {

    resetPreferences();

    system_restart();

    // qdelay(10000);

    return 1;
  }
  return 0;
}

int setAdminPassword(String command)
{
  Serial.print("Command =");
  Serial.println(command);


  String oldPassword;
  String newPassword;

  oldPassword = getValue(command, ',', 0);
  newPassword = getValue(command, ',', 1);

  if (oldPassword == adminPassword)
  {
    adminPassword = newPassword;
    writePreferences();
    return 1;
  }
  else
    return 0;


  //http://192.168.1.134/setAdminPassword?params=oldpassword,newpassword
  return 1;

}

int setWUSID(String command)
{
  Serial.print("Command =");
  Serial.println(command);


  String sentPassword;
  String WUSID;

  sentPassword = getValue(command, ',', 0);
  WUSID = getValue(command, ',', 1);

  Serial.print("adminPassword=");
  Serial.println(adminPassword);
  Serial.print("sentPassword=");
  Serial.println(sentPassword);

  if (sentPassword == adminPassword)
  {
    WeatherUnderground_StationID = WUSID;
    writePreferences();
    return 1;
  }
  else
    return 0;


  return 1;

}

int setWUKEY(String command)
{
  Serial.print("Command =");
  Serial.println(command);


  String sentPassword;
  String WUKEY;

  sentPassword = getValue(command, ',', 0);
  WUKEY = getValue(command, ',', 1);

  if (sentPassword == adminPassword)
  {
    WeatherUnderground_StationKey = WUKEY;
    writePreferences();

    return 1;
  }
  else
    return 0;


  return 1;

}

void startBlynk();
int setBAKEY(String command)
{
  Serial.print("Command =");
  Serial.println(command);


  String sentPassword;
  String BAKEY;

  sentPassword = getValue(command, ',', 0);
  BAKEY = getValue(command, ',', 1);

  if (sentPassword == adminPassword)
  {
    BlynkAuthCode = BAKEY;
    writePreferences();
    startBlynk();
    ESP.restart();
    return 1;
  }
  else
    return 0;


  return 1;

}

int rebootOurWeather(String command) {



  ESP.restart();
  // ESP.reset();
  delay(10000);

  return 1;
}

int setDateTime(String command)
{

  String _time;
  String _date;
  String _password;

  _password = getValue(command, ',', 0);
  _date = getValue(command, ',', 1);
  _time = getValue(command, ',', 2);

  Serial.println("command values:");
  Serial.println(_password);
  Serial.println(_date);
  Serial.println(_time);

  _date.replace("%20", " ");
  Serial.println(_password);
  Serial.println(_date);
  Serial.println(_time);

  if (_password == adminPassword)
  {
    if ((_time.length() > 0) && (_date.length() > 0))
    {
      //RtcDateTime compiled = RtcDateTime(_date.c_str(), _time.c_str());
      Serial.println("Updating DateTime in RTC");

    }
    else
    {
      Serial.println("Not updating DateTime in RTC");
      return 2;
    }
    return 1;
  }
  else
    return 0;
}


void writeToBlynkStatusTerminal(String statement);

// FOTA update commands


int updateOurWeather(String command)
{

  WiFiClient client;
  Serial.println("updateOurWeather");
  Serial.print("Command =");
  Serial.println(command);

  // grab the semaphore to stop reading and display
  xSemaphoreTake( xSemaphoreReadSensor, 10);

  String newFWVersion = "XX";

  if (command == adminPassword)
  {
    updateDisplay(DISPLAY_UPDATING);
    delay(5000);
    String fwVersionURL;
    fwVersionURL = "http://www.switchdoc.com/bin/CurrentFirmware.html";
    // Get the values for the update software

    HTTPClient httpClient;
    httpClient.begin( fwVersionURL );
    int httpCode = httpClient.GET();
    if ( httpCode == 200 ) {
      newFWVersion = httpClient.getString();

      Serial.print( "Current firmware version: " );
      Serial.println( WEATHERPLUSESP32VERSION );
      Serial.print( "Available firmware version: " );
      Serial.println( newFWVersion );

      if (UseBlynk)
      {
        writeToBlynkStatusTerminal("\nCurrent firmware version: ");
        writeToBlynkStatusTerminal(WEATHERPLUSESP32VERSION);
        writeToBlynkStatusTerminal("\nAvailable firmware version: ");
        writeToBlynkStatusTerminal(newFWVersion);
      }


      if ( newFWVersion != WEATHERPLUSESP32VERSION ) {
        Serial.println( "Preparing to update" );
        if (UseBlynk)
        {
          writeToBlynkStatusTerminal( "\nPreparing to update" );
        }


      }
      else
      { xSemaphoreGive( xSemaphoreRESTCommand);
        return 2;
      }
    }


    //t_httpUpdate_return ret = httpUpdate.update("www.switchdoc.com", 80, "/OurWeatherUpdater.php", WEATHERPLUSESP32VERSION);
    String fwImageURL;
    newFWVersion.trim();
    fwImageURL = "http://www.switchdoc.com/bin/" + newFWVersion + ".bin";
    Serial.println(fwImageURL);
    t_httpUpdate_return ret = httpUpdate.update(client, fwImageURL);
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.println("[update] Update failed.");
        updateDisplay(DISPLAY_NO_UPDATE_FAILED);
        delay(5000);
        xSemaphoreGive( xSemaphoreRESTCommand);
        return 1;

        break;
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("[update] Update no Updates.");
        updateDisplay(DISPLAY_NO_UPDATE_AVAILABLE);
        delay(5000);
        xSemaphoreGive( xSemaphoreRESTCommand);
        return 2;
        break;
      case HTTP_UPDATE_OK:

        Serial.println("[update] Update ok."); // may not called we reboot the ESP
        updateDisplay(DISPLAY_UPDATE_FINISHED);
        xSemaphoreGive( xSemaphoreRESTCommand);
        return 3;
        break;
    }



  }
  xSemaphoreGive( xSemaphoreRESTCommand);
  return 0;


}



// connection commands

int resetToDefaults(String command)
{


  return 1;

}

int enableCWOPControl(String command) {

  // Get state from command
  int state = command.toInt();


  return 1;
}

int enableTwitterControl(String command) {

  // Get state from command
  int state = command.toInt();

  return 1;
}


// Weather Controls

int weatherSmallControl(String command) {

  WeatherDisplayMode = DISPLAY_WEATHER_SMALL;
  writePreferences();

  return 1;
}


int weatherMediumControl(String command) {

  WeatherDisplayMode = DISPLAY_WEATHER_MEDIUM;
  writePreferences();
  return 1;
}



int weatherLargeControl(String command) {

  WeatherDisplayMode = DISPLAY_WEATHER_LARGE;
  writePreferences();
  return 1;
}



int weatherDemoControl(String command) {

  WeatherDisplayMode = DISPLAY_WEATHER_DEMO;
  writePreferences();
  return 1;
}

void writeToBlynkStatusTerminal(String statement);

int englishUnitControl(String command) {

  EnglishOrMetric = 0;
  writeToBlynkStatusTerminal("Units set to English");
  Blynk.virtualWrite(V8,  "English");
  writePreferences();
  return 1;
}


int metricUnitControl(String command) {

  EnglishOrMetric = 1;
  writeToBlynkStatusTerminal("Units set to Metric");
  Blynk.virtualWrite(V8,  "English");
  writePreferences();
  return 1;
}



// RasPiConnect

int jsonCmd(String command)
{

  Serial.println("in jsonCmd");
  return 1;
}
/*
  void jsonCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
  {

  currentObjectStructure parsedObject;
  initParsedObject(&parsedObject);

  #ifdef DEBUG
  Serial.println("------------------");
  #endif


  char myBuffer[1024];




  server.httpSuccess("application/json");

  if (type == WebServer::HEAD)
    return;

  int myChar;
  int count;
  myChar = server.read();
  count = 0;


  while (myChar > 0)
  {
    myBuffer[count] = (char) myChar;

    myChar = server.read();

    count++;

  }
  myBuffer[count] = '\0';

  delay(25);

  messageCount++;

  char returnJSON[500] = "";


  ExecuteCommandAndReturnJSONString(myBuffer, messageCount, md5str, &parsedObject, returnJSON, returnJSON);


  #ifdef DEBUG
  Serial.print("returnJSON =");
  Serial.println(returnJSON);
  #endif



  server << returnJSON;

  #ifdef DEBUG
  Serial.print("Mem1=");
  Serial.println(freeMemory());
  Serial.println("------------------");
  #endif
  }

*/
