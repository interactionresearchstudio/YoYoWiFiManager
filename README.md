![esp8266](https://github.com/interactionresearchstudio/YoYoWiFiManager/workflows/esp8266/badge.svg)
![esp32](https://github.com/interactionresearchstudio/YoYoWiFiManager/workflows/esp32/badge.svg)
# The Yo-Yo WiFi Manager Library
The Yo-Yo WiFi Manager Library is an [Arduino](http://www.arduino.cc/download) Library for [ESP8266](https://en.wikipedia.org/wiki/ESP8266) and [ESP32](https://en.wikipedia.org/wiki/ESP32) that manages WiFi credentials via a [captive portal](https://en.wikipedia.org/wiki/Captive_portal) configuration webpage; in this respect it is an alternative to the excellent [WiFiManager](https://github.com/tzapu/WiFiManager). However, the Yo-Yo WiFi Manager Library also supports the configuration of multiple devices simultaneously through one portal page, manages multiple sets of network credentials and offers full customisation of the portal HTML and JavaScript. 

Beyond WiFi credential management, the Yo-Yo WiFi Manager Library provides a means to hosts rich web experiences that can integrate with electronics for Physical Computing applications. Hosted webpages using HTML and JavaScript can call custom RESTful endpoints that can easily be defined to talk directly to the ESP modules and any external circuitry. The webserver can serve any [SPIFFS](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/spiffs.html) file, be that media files (JPEG, PNG, MP3, etc) or any JavaScript libraries ([jQuery](https://jquery.com/), [bootstrap.js](https://getbootstrap.com/), [Vue.js](https://vuejs.org/), [p5.js](https://p5js.org/), etc) within the storage capacity of the device. Operating as a captive portal and managing its own WiFi network, there is no requirement for an Internet connection. Furthermore, the library will manage local peer networks of multiple devices operating together in this way.

The library is being developed by David Chatting ([@davidchatting](https://github.com/davidchatting)), Mike Vanis ([@mikevanis](https://github.com/mikevanis)) and Andy Sheen ([@andysheen](https://github.com/andysheen)) for the [Yo–Yo Machines](https://www.yoyomachines.io/) project at the [Interaction Research Studio](https://github.com/interactionresearchstudio) - Goldsmiths, University of London. Collaboration welcome - please contribute by raising issues and making pull requests via GitHub.

## Installation

~~The latest stable release of the library is available in the Arduino IDE Library Manager - search for "YoYoWiFiManager". Click install.~~

Alternatively, the library can be installed manually. First locate and open the `libraries` directory used by the Arduino IDE, then clone this repository (https://github.com/interactionresearchstudio/YoYoNetworkManager) into that folder - this will create a new subfolder called `YoYoWiFiManager`.

The Yo-Yo WiFi Manager requires that either the Arduino core for the ESP8266 or ESP32 is installed - follow these instructions:

* ESP8266 - https://github.com/esp8266/Arduino#installing-with-boards-manager
* ESP32 - https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md

### Sketch Data Folder Uploader Tool
* ESP8266 - https://randomnerdtutorials.com/install-esp8266-filesystem-uploader-arduino-ide/
* ESP32 - https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/

## Examples
* Basic
* BasicWithEndpoints
* P5js
* PeerNetwork
* Vue

## Endpoints
The following endpoints are built-in:

/yoyo/credentials GET + POST

/yoyo/networks GET

/yoyo/clients GET

/yoyo/peers GET

## Development

* Fix the TODOs in the existing codebase
* The default HTML page should generate a page that allows basic wifi config
* Network discovery - zero conf (bonjour) support - use of iBeacon on the ESP32?
* HTTP File upload to SPIFFS
* SD card support as an alternative to SPIFFS
* Use of websockets to stream sensor values
* Support for peer-to-peer serverless connections across the Internet

## Limitations
The Yo-Yo WiFi Manager works with 2.4GHz WiFi networks, but not 5GHz networks - neither ESP8266 or ESP32 support this technology.