# WebLamp
## A wifi enabled RGBW lamp with ESP8266

WebLamp : a web-api enabled lamp WITH esp8266
This project allow to make a wifi controlled RGB + white lamp
The code is based on WebServer example of ESP8266 library present into Arduino platform support 
(see https://github.com/sandeepmistry/esp8266-Arduino)
The example is located into ESP8266WiFi library

Webserver support simple commands to drive RGB and white colors:
  
The lamp is controlled from a simple web api:
  *    / : gives basic lamp information
  *    /on : set white light on
  *    /off: set white light off
  *    /toggle: invert white light status
  *    /color: set rgb light to specific color (if PWM is not enabled only 8 colors are possible)
  *    /reset: restart ESP chip
  
The code is based on a ESP07 module but is usable from any ESP board with sufficient pins
  
## History
 * 14/05/2015 V.0.0.1 - First public release
