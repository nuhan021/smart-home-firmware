#include <Arduino.h>
#include "components/my_local_storage/my_local_storage.h"
#include "components/my_wifi/my_wifi.h"
#include "components/my_pin/my_pin.h"
#include <ArduinoWebsockets.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <ArduinoJson.h>
#include <nvs_flash.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <Wire.h>
#include <nvs.h>
#include<ESPmDNS.h>



const char *testHost = "www.google.com"; // ping server
My_wifi my_wifi(testHost);        // My_wifi class object
WiFiUDP udp;

using namespace websockets;
WebsocketsClient client; // websocket class object

My_pin my_pin;                     // My_pin class object
My_local_storage my_local_storage; // My_local_storage class object
AsyncWebServer server(80);         // web server port


// websocket api
String base_url = "ws://192.168.0.200:8000/ws/socket-server/?api-key";
String websocket_url = "";        // base_url + api-key = websocket_url
bool websocket_reconnect = false; // websocket reconnect flag



const int fanPins[4] = {12, 14, 25, 26}; // fan control pins
const int freq = 10000;                  // frequency 10000 Hz
const int resolution = 8;                // duty cicle 0-255
int channels[4] = {0, 1, 2, 3};          // pwm channels

const int RESET_BUTTON = 34; // down or change button
const int OK_BUTTON = 39;   // ok button


int selectedItem = 0;                                                           // menu selected item
bool inMenu = true;                                                             // inside menu or not

bool credentialsReceived = false; // Flag to track if credentials are received

// 74HC595 SHIFT REGISTER CONNECTION
#define DATA_PIN 23   // SER (DS)
#define CLOCK_PIN 18  // SRCLK
#define LATCH_PIN 5   // RCLK

String pinState1 = "00000000";
String pinState2 = "00000000";



void updateShiftOutput(uint8_t shiftRegisterValue1, uint8_t shiftRegisterValue2) {
    digitalWrite(LATCH_PIN, LOW);  
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, shiftRegisterValue2);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, shiftRegisterValue1);
    digitalWrite(LATCH_PIN, HIGH);
}



// whe websocket get new message this function call automatically,
// for handling and manupulation data
void onMessageCallback(WebsocketsMessage message)
{

    String msg = message.data(); // store data from websocket message

    if (msg.indexOf("ping") >= 0)
    // if received data type is ping
    {
        // send pong to server
        client.send("{\"type\":\"pong\"}");
    }

    // Serial.println("Received Message: " + msg);

    if (websocket_reconnect == false)
    // check if websocket reconnect flag is false
    {
        // call processPinData function to process data
        my_pin.processPinData(channels, msg);
    }
    else
    {
        // when websocket reconnect flag is true,
        // that means esp32 should update local database data to Django server,
        // because user can control pins while there is no INTERNET
        my_pin.updateWhileReconnectWebsocket();
    }
}

// this function does all the WiFi credential save related task
void save_wifi_server()
{
    preferences.begin("credentials", false); // open database for read/write mode

    WiFi.mode(WIFI_AP);                    // set Esp32 internal hotspot as a WiFi Access Point
    WiFi.softAP("Smart Home", "12345678"); // set hotspot name and password

    // start a server based on it's own hotspot for
    // sending http request to set WiFi (SSID, PASSWORD) and Api- key
    server.on("/savewifi", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        
        // user should use -> HTTP POST method and must use body with json
        // {
        //     "ssid": "your ssid",
        //     "password": "your password",
        //     "api-key": "your api-key"
        // }


        if (request->hasArg("ssid") && request->hasArg("password") && request->hasArg("api-key")) {


            String req_ssid = request->arg("ssid");
            String req_password = request->arg("password");
            String api_key = request->arg("api-key");
            

            request->send(200, "text/plain", "Credentials Received");

            // store (ssid, password & api_key) in local database
            preferences.putString("ssid", req_ssid);
            preferences.putString("password", req_password);
            preferences.putString("api-key" , api_key);

            Serial.println(req_ssid);
            Serial.println(req_password);
            Serial.println(api_key);

            preferences.end(); // close preferences

            credentialsReceived = true; // Set flag to true
            WiFi.softAPdisconnect(true); // close hotspot
            server.end(); // end server
        } else {
            request->send(400, "text/plain", "Missing ssid or password or api-key");
        } });
}

// this function help to clear local database
void clearPreferences(const char *namespaceName)
{
    preferences.begin(namespaceName, false); // set spasific database to read/write mode
    preferences.clear();                     // clear spasific local database
    preferences.end();
}


void setup()
{



    Serial.begin(115200);  // অথবা 9600, তবে 115200 দ্রুত ও জনপ্রিয়

    Serial.println('--------------------------------Hello world');
    delay(1000);

    pinMode(DATA_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);

    pinMode(RESET_BUTTON, INPUT); // set RESET_BUTTON as INPUT

    // while booting if DOWN_BUTTON is press then device will reset all the settings
    if (digitalRead(RESET_BUTTON) == 1)
    {
        // clearing all the data from local database or reseting the device
        clearPreferences("pin_states");
        clearPreferences("initialized");
        clearPreferences("pinState1");
        clearPreferences("pinState2");
        clearPreferences("credentials");
        delay(300);
        ESP.restart();
        
    }


    preferences.begin("pin_states", false); // open pin_states database as read/write mode

    bool isInitialized = preferences.getBool("initialized", false); // get the value of initialized
    preferences.end();

    if (!isInitialized)
    { // if isInitialized = false, that mens device boot for the first time
        // device detect it is first time boot

        save_wifi_server(); // call save_wifi_server for save WiFi and api-data

        server.begin(); // start wifi server

        // Wait until credentials are received
        while (!credentialsReceived)
        {
            delay(100); // Keep waiting for the HTTP request
        }

        preferences.begin("pin_states", false); // open pin_states database as read/write mode

        delay(3000);

        // Set all pins to false (first time only)
        // for (int i = 0; i < total_relay_control_pin; i++)
        // {
        //     preferences.putBool(String(i).c_str(), false);
        // }

        preferences.putString("pinState1", pinState1);
        preferences.putString("pinState2", pinState2);

        // set all the pwm pin to 0 (first time only)
        for (int i = 0; i < 4; i++)
        {
            String key = "pwm_" + String(i);
            preferences.putInt(key.c_str(), 0);
        }

        preferences.putBool("initialized", true); // Mark as initialized
        

        preferences.end();
    }

    // set up ledPins as output and collect every
    // pin state from local storage and write the value acording to the pin
    // for (int i = 0; i < total_relay_control_pin; i++)
    // {
    //     bool pinState = my_local_storage.loadPinState(i); // get pin state -> true/false
    //     // pinMode(pinMapping[i], OUTPUT);                   // set pin mode as output
    //     // digitalWrite(pinMapping[i], pinState);            // write the pin state -> true = ON, false = OFF

    //     updateShiftRegister(i, pinState);
    // }


    


    preferences.begin("pin_states", false);
    String pinState1 = preferences.getString("pinState1");
    String pinState2 = preferences.getString("pinState2");

    uint8_t shiftRegisterValue1 = strtol(pinState1.c_str(), NULL, 2); // Convert binary string to uint8_t
    uint8_t shiftRegisterValue2 = strtol(pinState2.c_str(), NULL, 2);

    updateShiftOutput(shiftRegisterValue1, shiftRegisterValue2);

    // set up pwm pins
    // collect pwm value from the local storage and write them according to the pin
    for (int i = 0; i < 4; i++)
    {
        String key = "pwm_" + String(i);
        int dutyCycle = my_local_storage.loadPwmValue(key); // load the value of the pin -> 0-255

        ledcSetup(channels[i], freq, resolution);
        ledcAttachPin(fanPins[i], channels[i]);
        ledcWrite(channels[i], dutyCycle);
    }

    preferences.begin("credentials", true); // Read-only mode
    // get ssid, password and api-key from local database
    String ssid_str = preferences.getString("ssid", ""); // Default to empty string
    String password_str = preferences.getString("password", "");
    String api_key_str = preferences.getString("api-key", "");

    websocket_url = base_url + api_key_str; // merge base_url and api_key for make final websocket url
    Serial.println(websocket_url);
    preferences.end();



    my_wifi.my_wifi_connect(ssid_str.c_str(), password_str.c_str()); // connect WiFi using ssid and password

    Serial.println(WiFi.localIP());

    client.onMessage(onMessageCallback); // set websocket onMessage method

    if (my_wifi.get_wifi_status() == 3) // if WiFi is connected
    {
        // connect websocket server using websocket_url
        client.connect(websocket_url.c_str());
    }

    // local control

    //     {
    //     "message": {
    //         "network": "local",
    //         "data": {
    //             "pins": [
    //                 {
    //                     "pin_1": false,
    //                     "pin_2": false,
    //                     "pin_3": false,
    //                     "pin_4": false
    //                 }
    //             ],
    //             "pwms": [
    //                 {
    //                     "pwm_1": 0,
    //                     "pwm_2": 0,
    //                     "pwm_3": 0,
    //                     "pwm_4": 0
    //                 }
    //             ]
    //         }
    //     }
    // }



    // start a server at local ip for control device without internet or in home network
    server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
              {
                  
                  String jsonString = String((char*)data); // Convert uint8_t* to String
                  my_pin.processPinData(channels, jsonString); // call processPinData to process json data
                  request->send(200, "application/json", "{\"status\":\"success\"}"); });

    server.begin(); // start the server

    Serial.println(WiFi.localIP());
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.localIP());


}

unsigned long startWifiCheck = millis();
unsigned long startWebsocketCheck = millis();
unsigned long startIpBroadcusting = millis();

const long wifiCheckInterval = 10000;
const long websocketCheckInterval = 20000;
const long iPBroadcastInterval = 2000;

// helper functon to call my_wifi.my_wifi_internet_status
void checkWifiStatus(void *pvParameters)
{
    my_wifi.my_wifi_internet_status(NULL);
    vTaskDelete(NULL);
}



void connectWebsocket()
{
    if (my_wifi.get_wifi_status() == 3)
    {
        // only if WiFi is connected and Internet is ok
        if (!client.available())
        {
            if (millis() - startWebsocketCheck >= websocketCheckInterval)
            {
                // WebSocket is not connected. Attempting to reconnect...

                if (client.connect(websocket_url))
                {
                    websocket_reconnect = true; // set websocket reconnect flag to true
                    // WebSocket reconnected
                }

                // Failed to reconnect to WebSocket
                startWebsocketCheck = millis();
            }
        }
        else
        {
            client.poll();               // fetch new message from server
            websocket_reconnect = false; // set websocket reconnect flag to false
        }
    }
}

void loop()
{


    if (millis() - startIpBroadcusting >= iPBroadcastInterval) {
    Serial.println(WiFi.localIP());
        if (WiFi.status() == WL_CONNECTED) {
            String ipMessage = "smart_home:" + WiFi.localIP().toString();
            udp.begin(4210);
            udp.beginPacket("255.255.255.255", 4210);  // UDP Broadcast
            udp.write(reinterpret_cast<const uint8_t*>(ipMessage.c_str()), ipMessage.length());
            udp.endPacket();
            startIpBroadcusting = millis();
        }
    }

    // calling checkWifiStatus function in core 0 to check wifi status without block other code
    if (millis() - startWifiCheck >= wifiCheckInterval)
    {

        xTaskCreatePinnedToCore(
            checkWifiStatus,
            "my_wifi_internet_status",
            10000,
            NULL,
            1,
            NULL,
            0);

        startWifiCheck = millis();
    }

    connectWebsocket(); // connect Websocket
}