/// set up WIFIManager
// this is the thing that creates a WIFI access point if it can't connect to the internet

// Load Wi-Fi library
#include <WiFi.h>


#include <WiFiManager.h>  // library https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include "WiFiClientSecure.h"
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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("in setup");

  wifiManager_setup();
  webserver_setup();

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
  "<a href='http://192.168.4.1:9000'>link</a>");


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