#ifndef MY_PIN_H
#define MY_PIN_H

#include <ArduinoJson.h>
#include "components/my_local_storage/my_local_storage.h"
#include <ArduinoWebsockets.h>
#include "components/my_wifi/my_wifi.h"

using namespace websockets;
extern WebsocketsClient client;

extern My_wifi my_wifi;

extern My_local_storage local_storage;

class My_pin
{

public:
    void processPinData(const int pinMapping[15], const int pwmMapping[4], String jsonString);
    void updatePinState(const int pinMapping[15], String pin, bool state, String network = "");
    void updatePwmValue(const int pwmMapping[4], String pin, int value, String network = "");
    void updateWhileReconnectWebsocket();
};

#endif