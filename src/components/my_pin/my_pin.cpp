#include "my_pin.h"

My_local_storage local_storage;

void My_pin::updatePinState(String pin, bool state, String network)
{

    int pinNumber = pin.substring(4).toInt(); // cut all string before index 4 -> pin_1 will be {int 1}
    if (pinNumber >= 1 && pinNumber <= 16)    // check pinNumber is under 1 to 15
    {
        if (network == "local")
        {
            // if request is came from local network and

            StaticJsonDocument<128> doc;
            doc[pin] = state; // making json -> {"pin_1": true}

            String jsonString; // convert to string

            serializeJson(doc, jsonString);
            Serial.print("My Json: ");
            Serial.println(jsonString);

            if (my_wifi.get_wifi_status() == 3)
            {
                // if wifi is ok then update the remote database
                client.send(jsonString);
            }
        }

        // int pinIndex = pinNumber - 1;
        // local_storage.savePinState(pinIndex, state);                      // store the value in local databasae
        // bool localStoragePinState = local_storage.loadPinState(pinIndex); // get the value from local database
        // digitalWrite(pinMapping[pinIndex], localStoragePinState);         // write the value to the exact pin
        // পূর্ববর্তী স্টেট লোড করা
        preferences.begin("pin_states", false);

        String pinState1 = preferences.getString("pinState1", "00000000");
        String pinState2 = preferences.getString("pinState2", "00000000");

        if (pinNumber < 9) {
            pinState1.setCharAt(pinNumber - 1, state ? '1' : '0'); // নির্দিষ্ট ইনডেক্স পরিবর্তন করুন
            preferences.putString("pinState1", pinState1); // নতুন স্টেট সংরক্ষণ করুন
        } 
        else {
            pinState2.setCharAt(pinNumber - 9, state ? '1' : '0');
            preferences.putString("pinState2", pinState2);
        }
        #define DATA_PIN 23   // SER (DS)
        #define CLOCK_PIN 18  // SRCLK
        #define LATCH_PIN 5   // RCLK

        uint8_t shiftRegisterValue1 = strtol(pinState1.c_str(), NULL, 2); // Convert binary string to uint8_t
        uint8_t shiftRegisterValue2 = strtol(pinState2.c_str(), NULL, 2); // Convert binary string to uint8_t

        digitalWrite(LATCH_PIN, LOW);  
        shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, shiftRegisterValue2);
        shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, shiftRegisterValue1);
        digitalWrite(LATCH_PIN, HIGH);
        preferences.end();
    }
}

void My_pin::updatePwmValue(const int pwmMapping[4], String pin, int value, String network)
{
    int pinNumber = pin.substring(4).toInt(); // pin_1 → 1
    if (pinNumber >= 1 && pinNumber <= 4)     // Ensure pinNumber is within valid range
    {
        if (network == "local")
        {
            Serial.println("The item from local network");

            StaticJsonDocument<128> doc;
            doc[pin] = value;

            String jsonString;
            serializeJson(doc, jsonString);

            Serial.print("The json is: ");
            Serial.println(jsonString);

            if (my_wifi.get_wifi_status() == 3)
            {
                client.send(jsonString); // Send updated value to server
                client.end();
            }
        }

        int pwmIndex = pinNumber - 1;

        String key = "pwm_" + String(pwmIndex);
        local_storage.savePwmValue(key, value);  // Store value locally
        int pwmValue = local_storage.loadPwmValue(key); // Retrieve saved value
        ledcWrite(pwmMapping[pwmIndex], pwmValue);  // Apply to PWM pin
    }
}


void My_pin::processPinData(const int pwmMapping[4], String jsonString)
{
    DynamicJsonDocument doc(256);

    DeserializationError error = deserializeJson(doc, jsonString);
    if (error)
    {
        // json parsing failed
        return;
    }

    JsonObject messageObj = doc["message"];                              // parsing message object
    String networkString = messageObj["network"].as<String>();           // from message obj parse network -> local/internet
    JsonArray pinDataArray = messageObj["data"]["pins"].as<JsonArray>(); // parse pins data
    JsonArray pwmDataArray = messageObj["data"]["pwms"].as<JsonArray>(); // parse pwms data

    for (JsonObject obj : pinDataArray) // grab object from pinDataArray
    {
        for (JsonPair kv : obj) // extrack data from the object
        {
            String pinName = kv.key().c_str();     // extract key
            bool pinValue = kv.value().as<bool>(); // extract value

            updatePinState(pinName, pinValue, networkString);
        }
    }


    for (JsonObject obj : pwmDataArray) // grab object from pwmDataArray
    {
        for (JsonPair kv : obj) // extract data from the object
        {
            String pinName = kv.key().c_str();   // extract key
            int pinValue = kv.value().as<int>(); // extract value

            updatePwmValue(pwmMapping, pinName, pinValue, networkString);
        }
    }
}

void My_pin::updateWhileReconnectWebsocket()
{
    StaticJsonDocument<128> doc;

    // grab all the pin value
    for (int i = 0; i < 15; i++)
    {
        String pin = "pin_" + String(i + 1);
        // make a json
        doc[pin] = local_storage.loadPinState(i);
    }

    // grab all the pwm value
    for (int i = 0; i < 4; i++)
    {
        String pin = "pwm_" + String(i);
        String jsonPin = "pwm_" + String(i + 1);
        // make the json
        doc[jsonPin] = local_storage.loadPwmValue(pin);
    }

    String jsonString;

    serializeJson(doc, jsonString);

    client.send(jsonString);
}