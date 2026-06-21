#include "modules/wifiConnect.h"

void connectWiFi(const char *ssid, const char *password)
{
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    // čekej, dokud se nepřipojí
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        attempts++;
        if (attempts > 60)
        { // timeout po cca 30 s
            Serial.println("\nFailed to connect.");
            return;
        }
    }

    Serial.println("\nWi-Fi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}
