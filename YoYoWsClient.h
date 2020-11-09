#ifndef YoYoWsClient_h
#define YoYoWsClient_h

#include <ArduinoJson.h>
#include <WebSocketsClient.h>

class YoYoWsClient {
  public:
    typedef void (*WebSocketsEventHandler)(WStype_t type, uint8_t * payload, size_t length);

    void begin();
    void setWsEventHandler(WebSocketsEventHandler incomingEventHandler);
    void sendMessage(char * payload);

  private:
    WebSocketsClient socket_client;
    static WebSocketsEventHandler wsEventHandler;

    static void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    void decodeData(const char* data);
};

#endif
