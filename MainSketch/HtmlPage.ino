const char html_page_template[] = R"=====(
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
      background-color: rgb(161, 202, 248);
    }
    .container {
      background-color: rgb(240, 240, 240);
      max-width: 600px;
      padding: 5px;
      box-sizing: border-box;
    }

    h1 {
      margin: 0;
      padding-bottom: 10px;
      font-size: 200%;
    }

    .myText {
      font-size: 20px;
      margin: 2px;
    }

    .myButton {
      width: 160px;
      height: 55px;
      margin: 5px 10px;
      background-color: rgb(25, 24, 121); /* Green */
      /* border: none; */
      border: 1px solid rgb(0, 0, 0);
      color: white;
      padding: 5px 10px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 18px;
      font-weight: bold;
      box-shadow: 0 3px 6px 0 rgba(0,0,0,0.2), 0 2px 7px 0 rgba(0,0,0,0.19);
      float: left;
    }
    .myButton:hover {
      background-color: rgb(174, 173, 233); /* Green */
      color: black;
      box-shadow: 0 3px 6px 0 rgba(0,0,0,0.24), 0 6px 15px 0 rgba(0,0,0,0.19);
    }

    .myTimePicker:hover {
      height: 155px;
      color: black;
      box-shadow: 0 3px 6px 0 rgba(0,0,0,0.24), 0 6px 15px 0 rgba(0,0,0,0.19);
    }

    .selectionBlock {
      font-size: 16px;
      margin: 10px 2px;
      float: left;
    }

    .selection {
      font-size: 14px;
    }

    .slidecontainer {
      width: 98%;
      margin: auto;
    }
  
    .slider {
      -webkit-appearance: none;
      width: 98%;
      margin: auto;
      height: 50px;
      background: rgb(255,122,122);
      outline: none;
      opacity: 1.0;
      -webkit-transition: .2s;
      transition: opacity .2s;
      border: 4px solid rgb(0, 0, 0);    }
    
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 50px;
      height: 50px;
      background: #FF0000;
      cursor: pointer;
    }
    
    .slider::-moz-range-thumb {
      width: 50px;
      height: 50px;
      background: #4CAF50;
      cursor: pointer;
    }

    .clearfix::after {
      content: "";
      clear: both;
      display: table;
    }
  
  </style>

</head>
    
    
<body onload="onLoadFunc()">

  <div class="container">
    <h1>GSM Thermostat</h1>

    <h2>Current status:</h2>
<TABLE BORDER=0>
  <TR ALIGN=LEFT> 
    <TD>Time:</TD> 
    <TD>%CURRENT_TIME_FIELD%</TD> 
  </TR>
  <TR ALIGN=LEFT>
    <TD>GSM status: </TD> 
    <TD>%GSM_STATUS_FIELD_0%</TD> 
  </TR>
  <TR ALIGN=LEFT>
    <TD>GSM status: </TD> 
    <TD>%GSM_STATUS_FIELD_1%</TD> 
  </TR>
  <TR ALIGN=LEFT>
    <TD>WiFi status:</TD> 
    <TD>%WIFI_STATUS_FIELD_0%</TD> 
  </TR>
  <TR ALIGN=LEFT>
    <TD>WiFi status:</TD> 
    <TD>%WIFI_STATUS_FIELD_1%</TD> 
  </TR>
  <TR ALIGN=LEFT>
    <TD>Heating:</TD>
    <TD>%HEATING_FIELD%</TD> 
  </TR>
  <TR ALIGN=LEFT>
    <TD>Desired temp:</TD> 
    <TD>%SET_ROOM_TEMP_FIELD% &deg;C</TD> 
  </TR>
  <TR ALIGN=LEFT>
    <TD>Current temp:</TD> 
    <TD>%CURRENT_ROOM_TEMP_FIELD% &deg;C</TD> 
  </TR>
  <TR ALIGN=LEFT>
    <TD>Heater:</TD> 
    <TD>%HEATER_WORKING_STATUS_FIELD%</TD> 
  </TR>
  <TR ALIGN=LEFT>
    <TD>Water temp:</TD> 
    <TD>%WATER_TEMP_FIELD% &deg;C</TD> 
  </TR>
</TABLE>

    <br>
    <br>
    <h2>Manual operation: %MANUAL_OPERATION_ENABLED_FIELD%</h2>
    <div class="clearfix">
      <form action="">
        <div class="slidecontainer">
          <label for="ManualDesiredTemperatureSliderId" class="myText">Desired room temp:</label>
          <input type="range" min="5.0" max="30.0" step="0.1" value="%MANUAL_TEMP_SLIDER_VALUE%" class="slider" id="ManualDesiredTemperatureSliderId" name="ManualDesiredTemperatureSliderId">
          <p class="myText">Selected temp: <span id="ManualDesiredTemperatureValue"></span></p>
        </div>
        <button class="myButton" type="submit">%MANUAL_ENABLE_RECONFIGURE_BUTTON_NAME%</button>
      </form>

      <form action="">
        <button class="myButton" type="submit" name="MANUAL_OFF" value="1">DISABLE MANUAL</button>
      </form>
    </div>

    <br>
    <br>
    <h2>Automatic operation: %AUTOMATIC_OPERATION_ENABLED_FIELD%</h2>
    <div class="clearfix">
      <form action="">
        <div class="slidecontainer">
          <label for="AutomaticDesiredTemperatureSliderId" class="myText">Desired room temp:</label>
          <input type="range" min="5.0" max="30.0" step="0.1" value="%AUTOMATIC_TEMP_SLIDER_VALUE%" class="slider" id="AutomaticDesiredTemperatureSliderId" name="AutomaticDesiredTemperatureSliderId">
          <p class="myText">Selected temp: <span id="AutomaticDesiredTemperatureValue"></span></p>
        </div>
        <button class="myButton" type="submit">%AUTOMATIC_ENABLE_RECONFIGURE_BUTTON_NAME%</button>

    <div class="clearfix">
      <label for="AutomaticStart" style="font-size:140%;">Heating Start time:</label>
      <input class=myTimePicker type="time" id="AutomaticStartId" name="AutomaticStart" min="00:00" max="24:00" value='%AUTOMATIC_START_TIME%' required style="width:16 0px;height:40px;font-size:200%;">
      <label for="AutomaticEnd" style="font-size:140%;">Heating End time  :</label>
      <input class=myTimePicker type="time" id="AutomaticEndId" name="AutomaticEnd" min="00:00" max="24:00" value='%AUTOMATIC_END_TIME%' required style="width:16 0px;height:40px;font-size:200%;">
    </div>
    
      </form>
      <form action="">
        <button class="myButton" type="submit" name="AUTOMATIC_OFF" value="1">DISABLE AUTO</button>
      </form>
    </div>

  </div>

  <script>
    var ManualDesiredTemperatureSlider = document.getElementById("ManualDesiredTemperatureSliderId");
    var ManualDesiredTemperatureValue = document.getElementById("ManualDesiredTemperatureValue");
    var AutomaticDesiredTemperatureSlider = document.getElementById("AutomaticDesiredTemperatureSliderId");
    var AutomaticDesiredTemperatureValue = document.getElementById("AutomaticDesiredTemperatureValue");

    ManualDesiredTemperatureValue.innerHTML = ManualDesiredTemperatureSlider.value + " &deg;C";
    ManualDesiredTemperatureSlider.oninput = function() {
      ManualDesiredTemperatureValue.innerHTML = this.value + " &deg;C";
      AutomaticDesiredTemperatureValue.innerHTML = this.value + " &deg;C";
    AutomaticDesiredTemperatureSlider.value = this.value;
    AutomaticDesiredTemperatureSlider.innerHTML = this.value;
    }

    AutomaticDesiredTemperatureValue.innerHTML = AutomaticDesiredTemperatureSlider.value + " &deg;C";
    AutomaticDesiredTemperatureSlider.oninput = function() {
      AutomaticDesiredTemperatureValue.innerHTML = this.value + " &deg;C";
      ManualDesiredTemperatureValue.innerHTML = this.value + " &deg;C";
    ManualDesiredTemperatureSlider.value = this.value;
    ManualDesiredTemperatureSlider.innerHTML = this.value;
    }

    function onLoadFunc() {
      ManualDesiredTemperatureSlider.style.background = rgb(255,122,122);
      AutomaticDesiredTemperatureSlider.style.background = rgb(255,122,122);
    }
  </script>

</body>
</html>
)=====";

const String CURRENT_TIME_FIELD = "%CURRENT_TIME_FIELD%";
const String GSM_STATUS_FIELD_0 = "%GSM_STATUS_FIELD_0%";
const String GSM_STATUS_FIELD_1 = "%GSM_STATUS_FIELD_1%";
const String WIFI_STATUS_FIELD_0 = "%WIFI_STATUS_FIELD_0%";
const String WIFI_STATUS_FIELD_1 = "%WIFI_STATUS_FIELD_1%";
const String HEATING_FIELD = "%HEATING_FIELD%";
const String SET_ROOM_TEMP_FIELD = "%SET_ROOM_TEMP_FIELD%";
const String CURRENT_ROOM_TEMP_FIELD = "%CURRENT_ROOM_TEMP_FIELD%";
const String HEATER_WORKING_STATUS_FIELD = "%HEATER_WORKING_STATUS_FIELD%";
const String WATER_TEMP_FIELD = "%WATER_TEMP_FIELD%";
const String MANUAL_OPERATION_ENABLED_FIELD = "%MANUAL_OPERATION_ENABLED_FIELD%";
const String MANUAL_ENABLE_RECONFIGURE_BUTTON_NAME = "%MANUAL_ENABLE_RECONFIGURE_BUTTON_NAME%";
const String MANUAL_TEMP_SLIDER_VALUE = "%MANUAL_TEMP_SLIDER_VALUE%";
const String AUTOMATIC_OPERATION_ENABLED_FIELD = "%AUTOMATIC_OPERATION_ENABLED_FIELD%";
const String AUTOMATIC_TEMP_SLIDER_VALUE = "%AUTOMATIC_TEMP_SLIDER_VALUE%";
const String AUTOMATIC_ENABLE_RECONFIGURE_BUTTON_NAME = "%AUTOMATIC_ENABLE_RECONFIGURE_BUTTON_NAME%";
const String AUTOMATIC_START_TIME = "%AUTOMATIC_START_TIME%";
const String AUTOMATIC_END_TIME = "%AUTOMATIC_END_TIME%";


String CreateHtmlTimePickerValue(byte hour, byte min)
{
  String hourAsString = String(hour);
  if (hourAsString.length() == 1)
  {
    hourAsString = "0" + hourAsString;
  }
  String minAsString = String(min);
  if (minAsString.length() == 1)
  {
    minAsString = "0" + minAsString;
  }
  return hourAsString + ":" + minAsString;
}

extern ESP32Time rtc;

String generate_html_page(String gsmStatusPart0,
                          String gsmStatusPart1,
                          String wifiStatusPart0,
                          String wifiStatusPart1,
                          String heating,
                          float desiredTemp,
                          float currentTemp,
                          bool heaterIsON,
                          float waterTemp,
                          bool manualModeEnabled,
                          bool automaticModeEnabled,
                          byte autoStartHour,
                          byte autoStartMin,
                          byte autoEndHour,
                          byte autoEndMin
                          )
{
  String html_page = html_page_template;

  String currentTimeAsString = "unavailable";
  if (rtc.isTimeSet())
  {
    currentTimeAsString = ESP32Time::getDateTime(rtc.getTimeStruct(), false);
  }
  html_page.replace(CURRENT_TIME_FIELD, currentTimeAsString);

  html_page.replace(GSM_STATUS_FIELD_0, gsmStatusPart0);
  html_page.replace(GSM_STATUS_FIELD_1, gsmStatusPart1);
  html_page.replace(WIFI_STATUS_FIELD_0, wifiStatusPart0);
  html_page.replace(WIFI_STATUS_FIELD_1, wifiStatusPart1);

  html_page.replace(HEATING_FIELD, heating);

  char s[51];
  dtostrf(desiredTemp, 3, 1, s);
  html_page.replace(SET_ROOM_TEMP_FIELD, s);
  dtostrf(currentTemp, 3, 1, s);
  html_page.replace(CURRENT_ROOM_TEMP_FIELD, s);

  html_page.replace(HEATER_WORKING_STATUS_FIELD, (heaterIsON ? "ON" : "OFF"));
  dtostrf(waterTemp, 3, 1, s);
  html_page.replace(WATER_TEMP_FIELD, s);

  html_page.replace(MANUAL_OPERATION_ENABLED_FIELD, (manualModeEnabled ? "ENABLED" : "DISABLED"));
  html_page.replace(MANUAL_TEMP_SLIDER_VALUE, String((int)desiredTemp));
  html_page.replace(MANUAL_ENABLE_RECONFIGURE_BUTTON_NAME, (manualModeEnabled ? "RECONFIGURE MANUAL" : "ENABLE MANUAL"));

  html_page.replace(AUTOMATIC_OPERATION_ENABLED_FIELD, (automaticModeEnabled ? "ENABLED" : "DISABLED"));
  html_page.replace(AUTOMATIC_TEMP_SLIDER_VALUE, String((int)desiredTemp));
  html_page.replace(AUTOMATIC_ENABLE_RECONFIGURE_BUTTON_NAME, (automaticModeEnabled ? "RECONFIGURE AUTO" : "ENABLE AUTO"));

  html_page.replace(AUTOMATIC_START_TIME, CreateHtmlTimePickerValue(autoStartHour, autoStartMin));
  html_page.replace(AUTOMATIC_END_TIME, CreateHtmlTimePickerValue(autoEndHour, autoEndMin));

  return html_page;
}
