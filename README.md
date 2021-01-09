# The Yo-Yo Network Manager Library
The Yo-Yo Network Manager Library is an [Arduino](http://www.arduino.cc/download) Library that manages WiFi connectivity for the [ESP8266](https://en.wikipedia.org/wiki/ESP8266) and [ESP32](https://en.wikipedia.org/wiki/ESP32). It hosts rich web experiences using HTML and JavaScript that can integrate with external circuitry. It also offers a simple means to configure WiFi credentials for devices via a web captive portal. Where there are multiple devices to configure at once, it automatically forms a peer-to-peer network and supports broadcast operations.

## Installation

~~The latest stable release of the library is available in the Arduino IDE Library Manager - search for "YoYoWiFiManager". Click install.~~

Alternatively, the library can be installed manually. First locate and open the `libraries` directory used by the Arduino IDE, then clone this repository (https://github.com/interactionresearchstudio/YoYoNetworkManager) into that folder - this will create a new subfolder called `YoYoWiFiManager`.

The Yo-Yo Network Manager requires that either the Arduino core for the ESP8266 or ESP32 is installed - follow these instructions:

* ESP8266 - https://github.com/esp8266/Arduino#installing-with-boards-manager
* ESP32 - https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md

## Endpoints
The following endpoints are built-in:

/yoyo/credentials GET + POST

/yoyo/networks GET

/yoyo/clients GET

/yoyo/peers GET

## Limitations
The Yo-Yo Network Manager works with 2.4GHz WiFi networks, but not 5GHz networks - neither ESP8266 or ESP32 support this technology.

## Authors
The library was developed by David Chatting ([@davidchatting](https://github.com/davidchatting)), Mike Vanis ([@mikevanis](https://github.com/mikevanis)) and Andy Sheen ([@andysheen](https://github.com/andysheen)) for the [Yo–Yo Machines](https://www.yoyomachines.io/) project at the [Interaction Research Studio](https://github.com/interactionresearchstudio) - Goldsmiths, University of London. Collaboration welcome - please contribute by raising issues and making pull requests via GitHub.