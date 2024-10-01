/// set up WIFIManager
// this is the thing that creates a WIFI access point if it can't connect to the internet

// Load Wi-Fi library
#include <WiFi.h>


#include <WiFiManager.h>  // library https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include "WiFiClientSecure.h"


#ifdef ESP32
  #include <SPIFFS.h>
#endif
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

// WiFiFlientSecure for SSL/TLS support
WiFiClientSecure client;

// Set web server port number to 80
WiFiServer server(9000);





// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

char* apname = "ap_config_me";
bool configMode = true;

WiFiManager wifiManager;

WiFiManagerParameter *custom_foo;


bool json_loaded = false;

String bank_program = "0:0";
int channel = 0;
float volume_map[32][3] = {{0.0,0.0,0.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0}, 
                           {1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},
                           {1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},
                           {1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0},{1.0,1.0,1.0}};
int volume_map_length = 3;
const int MAP_MAX_LENGTH = 32;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("in setup");

  config_file_setup();
//  wifiManager_setup();
//  webserver_setup();

  Serial.println("wifi connected");
  led_flash(LED_BUILTIN, 100,125,8); // flash a light 8 times fast to say we connected to wifi

}

//       // "<br/><input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>";
void webserver_setup(){
  Serial.println("webserver_setup");
  // Set web server port number to 80
  server.begin();
}

void wifiManager_setup(){
  //WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement) {


  // add params to its own menu page and remove from wifi, NOT TO BE COMBINED WITH setMenu!
  //  void          setParamsPage(bool enable);
  wifiManager.setParamsPage(true);
  wifiManager.setConfigPortalBlocking(false);

  custom_foo = new WiFiManagerParameter(NULL, "variable foo", "defaultfoo", 40,
  "<a href='http://192.168.4.1:9000'>link</a><script>document.location.href='http://192.168.4.1:9000'</script>");

  wifiManager.addParameter(custom_foo);

  wifiManager.setConfigPortalTimeout(120);
  //automatically connect using saved credentials if they exist
  //If connection fails it starts an access point with the specified name
  if(wifiManager.autoConnect(apname)){
      Serial.println("connected...yeey :)");
  }
  else {
      Serial.println("Configportal running");
  }
}




void loop(){
  wifiManager.process();
  webserver_loop();
}


void webserver_loop() {

  // put your main code here, to run repeatedly:
//  Serial.println("in loop");
  WiFiClient client = server.available();   // Listen for incoming clients


  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
         // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();


            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");
            client.println("<span id='changeme' onclick=\"document.getElementById('changeme').innerText='new';\">old</span>");
            client.println("<script>document.getElementById('changeme').innerText='halfway';</script>");
            client.println("</body></html>");
                 // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }


}

/*********************************
 * CONFIG FILE FUNCTIONS
 */


void config_file_setup(){
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }  
  Serial.println("config_file_setup");
  load_config_file();
 // volume_map[2][1] = 0.564;
  save_config_file();
}

void load_config_file(){
  if (SPIFFS.exists("/config.json")) {
    //file exists, reading and loading
    Serial.println("reading config file");
    File configFile = SPIFFS.open("/config.json", "r");
    if (configFile) {
      Serial.println("opened config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);
      DynamicJsonDocument json_config(2048);
      auto deserializeError = deserializeJson(json_config, buf.get());
      serializeJson(json_config, Serial); // this just prints to serial
      if ( ! deserializeError ) {
        Serial.println("\nparsed json");
      } else {
        Serial.println("failed to load json config");
      }




      /***********************************************
      LOAD CONFIG VARIABLES INTO THE GLOBALS HERE
      ************************************************/
      // load parsed JSON values into global variables
      if(!json_config["bank_program"].isNull()){
        bank_program = json_config["bank_program"].as<String>();
      }else{
        Serial.println("don't have bank_program yet");
      }
      if(!json_config["channel"].isNull()){
        channel = json_config["channel"];
      }else{
        Serial.println("don't have channel yet");
      }

      // HANDLE ARRAYS LIKE THIS
      if(!json_config["volume_map"].isNull()){
        JsonArray mappingArray = json_config["volume_map"].as<JsonArray>();
        int i = 0;
        for (JsonVariant value : mappingArray) {
            volume_map[i][0] = value[0];
            volume_map[i][1] = value[1];
            volume_map[i][2] = value[2];
            i += 1;
        }
      }else{
        Serial.println("don't have volume_map yet");
      }    
      // END ARRAY HANDLING EXAMPLE
      /***********************************************
      END LOAD CONFIG VARIABLES INTO THE GLOBALS HERE
      ************************************************/

      serializeJson(json_config, Serial); // this just prints to serial
      configFile.close();
      json_loaded = true;
    }
  }else{
    Serial.println("no config.json file found for reading");
  }

}

void save_config_file(){
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("no config file found for writing");
  }
  DynamicJsonDocument json_config(2048);

  /***********************************************
  SAVE CONFIG VARIABLES INTO JSON HERE
  ************************************************/

  json_config["bank_program"] = bank_program;
  json_config["channel"] = channel;

  // HANDLE ARRAYS LIKE THIS
  json_config["volume_map"].clear();
  JsonDocument pointsdoc;
  JsonArray points = pointsdoc.to<JsonArray>();  
  for(int i = 0; i< MAP_MAX_LENGTH; i++){
    JsonDocument pointdoc;
    JsonArray point = pointdoc.to<JsonArray>();  
    point.add(volume_map[i][0]);
    point.add(volume_map[i][1]);
    point.add(volume_map[i][2]);
    points.add(point);
  }
  json_config["volume_map"] = points;
  // END ARRAY HANDLING EXAMPLE

  /***********************************************
  SAVE CONFIG VARIABLES INTO JSON HERE
  ************************************************/



  Serial.println("saving config.json");
  serializeJson(json_config, Serial); // this jsut writes the json to Serial out
  serializeJson(json_config, configFile);
  configFile.close();
}

void delete_config_file(){
  // unco/*mment this to actxually run the code
  Serial.println("deleting all stored SSID credentials");
  SPIFFS.remove("/config.json");
}


// a little helper pin that flashes the builin LED some number of times.
void led_flash(int pin, int onms, int offms, int times){
  // flash an led a certain number of times. Good for status things.
  for(int i = 0; i<times; i++){
    digitalWrite(pin, HIGH);
    delay(onms);
    digitalWrite(pin, LOW);
    delay(offms);
  }
}