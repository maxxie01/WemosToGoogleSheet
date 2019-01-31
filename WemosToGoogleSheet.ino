// BY: Akshaya Niraula
// ON: Nov 11, 2016
// AT: http://www.embedded-lab.com/
// http://embedded-lab.com/blog/post-data-google-sheets-using-esp8266/

//info by Maxxie
// the above link use the function printRedir but this is't nolonger work with the library
// use instead of printRedir the GET function.
// 
// set the security on your Google spreadsheet document to everyone.
// Get your GoogleScript ID like : AKfycbyA36tO6v_zch-pkyvnDE0N8EplcuHMe1uGk-tvPAPySEIWJ_h-
//  - the hole url looks like    : https://script.google.com/macros/s/AKfycbyA36tO6v_zch-pkyvnDE0N8EplcuHMe1uGk-tvPAPySEIWJ_h-/exec?tag=test&value=-1
// Get the certificate of your Google site by
//  - go in Chrome to your document
//  - click on the securicy marker before your URL and select details of the certificate
//  - select the tab Details en search for Fingerprint

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <HTTPSRedirect.h>      //https://github.com/electronicsguy/ESP8266
#include <OneWire.h>            //DS18B20 temperature library https://github.com/PaulStoffregen/OneWire
#include <DallasTemperature.h>  //DS18B20 temperature library https://github.com/milesburton/Arduino-Temperature-Control-Library

//Wifi manager starts as AccessPoint if there is no wifi connection
WiFiManager wifiManager;
String deviceName = "GoogleTest";      //the prefix for your WiFi device and tag name in your GoogleDoc column C

// The ID below comes from Google Sheets, not the sheet ID but the script created in the sheet
const char *GScriptId = "AKfycbyA36tO6v_zch-pkyvnDE0N8EplcuHMe1uGk-tvPAPySEIWJ_h-";         //change to your Google Script ID
const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
const int httpsPort =  443;
//get this from the cetificate (select slot before gooogle doc site)
const char* fingerprint =   "a2 0c 5b c0 f7 4f 2b 94 48 57 a1 be b0 b3 40 b6 33 c8 b3 7c";  //change to your Google Certificate Fingerprint

// Prepare the url (without the varying data)
String url = String("/macros/s/") + GScriptId + "/exec?";

HTTPSRedirect client(httpsPort);

// interval to read data
unsigned long lastIntervalTime;
int interval = 1000;    //update interval for checking the temperature
float minTempDif = 0.15; //minimum temperature difference for sending

//DS18B20 setup
#define ONE_WIRE_BUS D2 //   //DS18B20 is connector to D2
float lastTemperature;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);


void setup() {
  Serial.begin(57600);

  deviceName = deviceName + "-" + String(ESP.getChipId()); //Generate device name based on ID
  Serial.print("wifiManager...");
  Serial.println(deviceName);
  wifiManager.autoConnect(deviceName.c_str(), ""); //Start wifiManager
  Serial.println("Connected!");
  Serial.print(" IP address: ");
  Serial.println(WiFi.localIP());

  
  Serial.print(String("Connecting to: "));
  Serial.println(host);

  bool flag = false;
  client.setInsecure();
  for (int i=0; i<5; i++){
    int retval = client.connect(host, httpsPort);
    if (retval == 1) {
       flag = true;
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  // Connection Status, 1 = Connected, 0 is not.
  Serial.println("Connection Status: " + String(client.connected()));
  
  if (!flag){
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    return;
  }

  // Data will still be pushed even certification don't match.
  if (client.verify(fingerprint, host)) {
    Serial.println("Certificate match.");
  } else {
    Serial.println("Certificate mis-match");
  }

  // Start DS18B20 library
  sensors.begin();
}


void loop() {
  unsigned long tmpInterval = millis();
  if( (tmpInterval - lastIntervalTime) > interval){
    lastIntervalTime = tmpInterval;

    //start Requesting temperatures
    sensors.requestTemperatures(); // Send the command to get temperatures
    float tmpTemperature = sensors.getTempCByIndex(0);
    if( abs( (lastTemperature - tmpTemperature) * 100) > (minTempDif * 100) && tmpTemperature > -127.00 ){
      lastTemperature = tmpTemperature;
      postData(lastTemperature);
    }
  }

}

// This is the main method where data gets pushed to the Google sheet
void postData(float value){
  Serial.println("Tag[" + deviceName + "] value: " + String(value) + "]");
  if (!client.connected()){
    Serial.print("Connecting to client again..."); 
    client.connect(host, httpsPort);
    if (!client.connected()) Serial.println("error"); else Serial.println("oke");
  }
  String urlFinal = url + "tag=" + deviceName + "&value=" + String(value);
  Serial.println(" - Connected to host[" + String(host) + "] on httpsPort[" + String(httpsPort) + "]");
  Serial.println(" - send by GET: " + urlFinal);
  if(client.GET(urlFinal, host)){
    Serial.println(" - data is send");
  }else{
    Serial.println(" - error by sending data");
  }
  
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    line.trim();
    if(line.length() > 0){
      Serial.print(" - ");
      Serial.println(line);
    }
  }
  
}
