#ifndef MY_WIFI_H
#define MY_WIFI_H

#include <Arduino.h>
#include <WiFi.h>



class My_wifi
{

private:
    const char *wifi_name;
    const char *wifi_password;
    const char *google_dns;
    wl_status_t wifi_status;

public:
    My_wifi(const char *dns = "8.8.8.8");
    void my_wifi_connect(const char *name, const char *password);
    void my_wifi_internet_status(void *pvParameters);
    wl_status_t get_wifi_status();
};

#endif