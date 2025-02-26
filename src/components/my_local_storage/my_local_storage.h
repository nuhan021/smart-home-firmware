#ifndef MY_LOCAL_STORAGE_H
#define MY_LOCAL_STORAGE_H

#include "Preferences.h"
extern Preferences preferences;

class My_local_storage
{
public:
    void savePinState(int pinIndex, bool state);
    bool loadPinState(int pinIndex);

    void savePwmValue(String key, int value);
    int loadPwmValue(String key);
};

#endif