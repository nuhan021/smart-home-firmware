#include "my_local_storage.h"
Preferences preferences;

void My_local_storage::savePinState(int pinIndex, bool state)
{
    preferences.begin("pin_states", false);
    preferences.putBool(String(pinIndex).c_str(), state);
    preferences.end();
}

bool My_local_storage::loadPinState(int pinIndex)
{
    preferences.begin("pin_states", true);
    bool state = preferences.getBool(String(pinIndex).c_str(), false); // default false
    preferences.end();
    return state;
}

void My_local_storage::savePwmValue(String key, int value)
{
    preferences.begin("pin_states", false);
    preferences.putInt(key.c_str(), value);
    preferences.end();
}

int My_local_storage::loadPwmValue(String key)
{
    preferences.begin("pin_states", true);
    int value = preferences.getInt(key.c_str(), 0);
    preferences.end();
    return value;
}