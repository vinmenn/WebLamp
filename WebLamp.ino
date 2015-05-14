/* WebLamp.ino
 *
 * Copyright (c) 2015 Vincenzo Mennella.  Use may be in
 * whole or in part in accordance to the General Public License (GPL).
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
*/
/*-----------------------------------------------------------------------------
  WebLamp : a web-api enabled lamp
  This project allow to make a wifi controlled RGB + white lamp
  This code is based on WiFiWebServer example present into ESP8266WiFi library
  (Thanks to Ivan Grokhotkov for his excellent work)
  
  The lamp is controlled from a simple web api:
    / : gives basic lamp information
    /on : set white light on
    /off: set white light off
    /toggle: invert white light status
    /set: set white light to dim level (if PWM is enabled)
    /color: set rgb light to specific color (if PWM is not enabled only 8 colors are possible)
    /reset: restart ESP chip
  
  The code is based on a ESP07 module but is usable from any ESP board with sufficient pins
  -----------------------------------------------------------------------------
 */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

//#define SERIAL_DEBUG  1
//#define PWM_OUTPUT  1
#define BLINK_CONNECT 1
#define MAX_TRIES     50
#define STATIC_IP     1

const char* LampVersion =  "0.0.1";
const char* ssid = "<INSERT SSID>";
const char* password = "<INSERT PASSWORD>";
const int lamp_pin = 16;
const int red_pin = 14;
const int green_pin = 12;
const int blue_pin = 13;

MDNSResponder mdns;
ESP8266WebServer server(80);
#ifdef STATIC_IP
IPAddress ip = IPAddress(192,168,1,18);
IPAddress gw = IPAddress(192,168,1,1);
IPAddress sn = IPAddress(255,255,255,0);
#endif

String _status;   //Current white lamp status
String _color;    //Current rgb color
String _response; //Current response

//setup chip
void setup(void){
  //Init pins
  pinMode(lamp_pin, OUTPUT);
  pinMode(red_pin, OUTPUT);
  pinMode(green_pin, OUTPUT);
  pinMode(blue_pin, OUTPUT);
  
  //Set pins initial values
  #ifdef PWM_OUTPUT
  analogWrite(lamp_pin,  0);
  analogWrite(red_pin,   0);
  analogWrite(green_pin, 0);
  analogWrite(blue_pin,  0);
  #else
  digitalWrite(lamp_pin,  0);
  digitalWrite(red_pin,   0);
  digitalWrite(green_pin, 0);
  digitalWrite(blue_pin,  0);
  #endif
  //Set default values
  _color   = "#000000";
  _status  = "on";
#ifdef STATIC_IP
  // ip = IPAddress(192,168,1,18);
  // gw = IPAddress(192,168,1,1);
  // sn = IPAddress(255,255,255,0);
#endif
#ifdef SERIAL_DEBUG
  Serial.begin(115200);
  Serial.print("\r\n\r\n\r\nWifiLamp version ");
  Serial.println(LampVersion);
#endif
  
  WiFi.begin(ssid, password);
#ifdef STATIC_IP
  WiFi.config(ip, gw, sn);
#endif
  
#ifdef SERIAL_DEBUG
  Serial.println("");
#endif

  // Wait for connection
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < MAX_TRIES) {
#ifdef BLINK_CONNECT  
    digitalWrite(blue_pin,  0);
#endif
    delay(500);
#ifdef BLINK_CONNECT  
    digitalWrite(blue_pin,  1);
#endif
#ifdef SERIAL_DEBUG
    Serial.print(".");
#endif
    tries++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    //Wifi connected
#ifdef SERIAL_DEBUG
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
#endif
  
    if (mdns.begin("esp8266", WiFi.localIP())) {
#ifdef SERIAL_DEBUG
      Serial.println("MDNS responder started");
#endif
    }
    //Basic functions
    server.on("/", handleRoot); //Return overall status
    server.on("/on", setOn);
    server.on("/off", setOff);
    server.on("/toggle", toggleLamp);
    server.on("/color", HTTP_POST, setColor); //Set color
    server.on("/reset", HTTP_GET, resetLamp);//Reset lamp
    server.onNotFound(handleNotFound);
    server.begin();
#ifdef SERIAL_DEBUG
    Serial.println("HTTP server started");
#endif
#ifdef BLINK_CONNECT  
    //Blink green => connection done
    for(uint8_t i = 0; i < 3; i++) {
      digitalWrite(green_pin, 0);
      delay(500);
      digitalWrite(green_pin, 1);
      delay(500);
    }
#endif
  }
  else {
#ifdef BLINK_CONNECT  
    //Blink red => no connection
    for(uint8_t i = 0; i < 5; i++) {
      digitalWrite(red_pin, 0);
      delay(500);
      digitalWrite(red_pin, 1);
      delay(500);
    }
#endif
  }
  delay(500);
  //turn on white lamp
#ifdef PWM_OUTPUT
  analogWrite(lamp_pin,  PWMRANGE);
#else
  digitalWrite(lamp_pin, 1);
#endif
  
}
//main loop
void loop(void){
  mdns.update();
  server.handleClient();
} 
//handle root
void handleRoot() {
  //TODO: send web interface
  _response = String(
                "\r\n\t\"version\" = \"" + String(LampVersion) + "\"," + 
                "\r\n\t\"status\" = \""  + _status + "\"," + 
                "\r\n\t\"color\" = \""   + _color + "\"");
                
  sendResponse(true);
}
//Not found response
void handleNotFound(){
  _response = "File Not Found\n\n";
  _response += "URI: ";
  _response += server.uri();
  _response += "\nMethod: ";
  _response += (server.method() == HTTP_GET)?"GET":"POST";
  _response += "\nArguments: ";
  _response += server.args();
  _response += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    _response += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", _response);
#ifdef SERIAL_DEBUG
  Serial.println(_response);
#endif
}
//Turn on white lamp
void setOn() {
  setLamp(true);
#ifdef SERIAL_DEBUG
  Serial.println("on");
#endif
}
//Turn off white lamp
void setOff() {
  setLamp(false);
#ifdef SERIAL_DEBUG
  Serial.println("off");
#endif
}
//Toggle white lamp
void toggleLamp() {
  if (digitalRead(lamp_pin) == 1)
    setLamp(false);
  else
    setLamp(true);
#ifdef SERIAL_DEBUG
  Serial.println("toggle");
#endif
}
//Set white lamp on|off
void setLamp(bool status) {
  if (status) {
    _status = "on";
    digitalWrite(lamp_pin, 1);
  }
  else {
    _status = "off";
    digitalWrite(lamp_pin, 0);
  }
  _response = "\"status\" = \"" + _status + "\"";
  
  sendResponse(true);
}
//Set RGB color
void setColor() {
  _color = server.arg("color");
  
#ifdef SERIAL_DEBUG
  Serial.print("color: ");
  Serial.print(_color);
  Serial.print(" length: ");
  Serial.println(_color.length());
#endif  

   long number = hex2long(_color);
  
  if (number >= 0) {
    int r = number >> 16;
    int g = (number >> 8) & 0xff;
    int b = number & 0xff;
    
    //Require pwm output
    #ifdef PWM_OUTPUT
    analogWrite(red_pin, r);
    analogWrite(green_pin, g);
    analogWrite(blue_pin, b);
    #else
    digitalWrite(red_pin,   r != 0 ? 1 : 0);
    digitalWrite(green_pin, g != 0 ? 1 : 0);
    digitalWrite(blue_pin,  b != 0 ? 1 : 0);
    #endif
    
    _response = "\"color\" = \"" + _color + "\"";
    sendResponse(true);
  }
  else {
    _response = "Invalid color";
    sendResponse(false);
  }
#ifdef SERIAL_DEBUG
  Serial.println("setcolor");
#endif
}
//reset chip
void resetLamp() {
  //TBD
  _response = "";
  sendResponse(true);
#ifdef SERIAL_DEBUG
  Serial.println("resetLamp");
#endif
  delay(2000); //Give time to send response out
  //Restart chip
  ESP.reset();
}
//Send http response
void sendResponse(bool success) {
  if (success)
  {
    _response = "{\r\n\t\"success\": true,\r\n\t" + _response + "\r\n\t\"error\" = \"\"\r\n}";
    server.send(200, "application/json", _response);
  }
  else
  {
    _response = "{\r\n\t\"success\": false,\r\n\t\"error\" = " + _response + "\r\n}";
    server.send(500, "application/json", _response);
  }
}
//Simple convert html hex string to number
long hex2long(String hex) {

  long num = 0;
  int length = hex.length() - 1; //length - first char (#)
  for(int i=0;i<length;i++) {
    char c = hex[i+1];
    //add nibble
    if (c >= 'A' && c <= 'F')  num += (c - 55) << 4 * (length - i - 1);
    else if (c >= 'a' && c <= 'f')  num += (c - 87) << 4 * (length - i - 1);
    else if (c >= '0' && c <= '9')  num += (c - 48) << 4 * (length - i - 1);
    else
      return -1;
  }  
  return num;
}
