/*
    Example documented here > https://github.com/interactionresearchstudio/YoYoWiFiManager#websocketsserver
*/

#include <YoYoWiFiManager.h>
#include <YoYoSettings.h>

#include <WebSocketsServer.h>

YoYoWiFiManager wifiManager;
YoYoSettings *settings;

WebSocketsServer webSocket = WebSocketsServer(81);

void setup() {
  Serial.begin(115200);

  settings = new YoYoSettings(512);
  wifiManager.init(settings, onceConnected, NULL, NULL, false, 80);

  wifiManager.begin("YoYoMachines", "blinkblink");
}

void onceConnected() {
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  uint8_t wifiStatus = wifiManager.loop();

  if(wifiStatus == YY_CONNECTED || wifiStatus == YY_CONNECTED_PEER_CLIENT || wifiStatus == YY_CONNECTED_PEER_SERVER) {
    webSocket.loop();
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      break;
    case WStype_BIN: break;
    case WStype_ERROR: break;
    case WStype_FRAGMENT_TEXT_START: break;
    case WStype_FRAGMENT_BIN_START: break;
    case WStype_FRAGMENT: break;
    case WStype_FRAGMENT_FIN: break;
    default: break;
    }
}