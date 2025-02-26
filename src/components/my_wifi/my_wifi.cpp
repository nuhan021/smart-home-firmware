#include "my_wifi.h"

// Constructor Implimentation
My_wifi::My_wifi(const char *dns)
{

    google_dns = dns;
}

// my_wifi_connect function Implimentation
void My_wifi::my_wifi_connect(const char *name, const char *password)
{
    wifi_name = name;
    wifi_password = password;
    WiFi.begin(wifi_name, wifi_password); // connect wifi

    // for debug purpus
    // Connecting wifi...

    // counter variable for connection attempt
    int wifi_connection_attempt = 0;

    while (WiFi.status() != WL_CONNECTED && wifi_connection_attempt < 10)
    {
        // try to connect wifi if not connected
        delay(500);
        wifi_connection_attempt++;
    }

    wifi_status = WiFi.status();
    return;
}

void My_wifi::my_wifi_internet_status(void *pvParameters)
{

    wl_status_t status = WiFi.status(); // store wifi status in status variable

    if (status != WL_CONNECTED)
    { // if wifi is not connected
        wifi_status = status;
        vTaskDelete(NULL);
        return;
    }

    WiFiClient client;
    if (!client.connect(google_dns, 80))
    {
        // if there is no internet
        wifi_status = WL_DISCONNECTED;
        vTaskDelete(NULL);
        return;
    }

    client.stop();

    // if everything is OK
    wifi_status = WL_CONNECTED;
    vTaskDelete(NULL);
    return;
}

wl_status_t My_wifi::get_wifi_status()
{
    return wifi_status; // return private wifi status
}