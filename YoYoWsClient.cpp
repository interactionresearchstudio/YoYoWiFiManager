#include "YoYoWsClient.h"

YoYoWsClient::WebSocketsEventHandler YoYoWsClient::wsEventHandler = NULL;

void YoYoWsClient::setWsEventHandler(WebSocketsEventHandler incomingEventHandler) {
  wsEventHandler = incomingEventHandler;
}

void YoYoWsClient::begin() {
  socket_client.begin("192.168.4.1", 80, "/ws");
  socket_client.onEvent(webSocketEvent);
  socket_client.setReconnectInterval(5000);
}

void YoYoWsClient::sendMessage(char * payload) {
  socket_client.sendTXT(payload);
}

void YoYoWsClient::webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  if (wsEventHandler) {
    wsEventHandler(type, payload, length);
  }
  /*
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!\n");
      //Hot fix for when client doesn't catch RESTART command
      // TODO softReset(4000);
      break;
    case WStype_CONNECTED:
      Serial.println("Connected!");
      // TODO socket_client.sendTXT(getJSONMac().c_str());
      break;
    case WStype_TEXT:
      Serial.println("Text received");
      #ifdef DEV
      Serial.println((char *)payload);
      #endif
      String output = (char *)payload;
      if (output == "RESTART") {
        // TODO softReset(4000);
        Serial.println("i'm going to reset");
      } else {
        // TODO decodeData((char *)payload);
      }
      break;
  }
  */
}
