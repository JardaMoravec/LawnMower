#include "modules/network/wifiConnect.h"

static const char *wifiStatusText(wl_status_t status)
{
    switch (status)
    {
    case WL_NO_SSID_AVAIL:
        return "SSID not found";
    case WL_CONNECT_FAILED:
        return "authentication failed (wrong password?)";
    case WL_CONNECTION_LOST:
        return "connection lost";
    case WL_DISCONNECTED:
        return "disconnected";
    default:
        return "timeout or unknown error";
    }
}

static void logTargetNetworkScan(const char *ssid)
{
    Serial.println("Scanning for Wi-Fi networks...");

    const int networkCount = WiFi.scanNetworks();
    if (networkCount <= 0)
    {
        Serial.println("  No networks found.");
        return;
    }

    bool targetFound = false;
    for (int i = 0; i < networkCount; i++)
    {
        if (WiFi.SSID(i) == ssid)
        {
            targetFound = true;
            Serial.print("  Target network visible: RSSI ");
            Serial.print(WiFi.RSSI(i));
            Serial.print(" dBm, channel ");
            Serial.println(WiFi.channel(i));
        }
    }

    if (!targetFound)
    {
        Serial.println("  Target SSID not visible — check name, 2.4 GHz, or range.");
    }
}

static bool waitForLocalIp(unsigned long timeoutMs = 15000)
{
    const unsigned long start = millis();
    while (WiFi.localIP()[0] == 0)
    {
        if (millis() - start >= timeoutMs)
        {
            return false;
        }
        delay(100);
    }
    return true;
}

bool connectWiFi(const char *ssid, const char *password)
{
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(WIFI_PS_NONE);
    WiFi.disconnect(true);
    delay(100);

    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        attempts++;
        if (attempts > 60)
        {
            Serial.println();
            Serial.print("Wi-Fi connection failed: ");
            Serial.println(wifiStatusText(WiFi.status()));
            logTargetNetworkScan(ssid);
            return false;
        }
    }

    Serial.println();
    Serial.println("Wi-Fi connected.");

    if (!waitForLocalIp())
    {
        Serial.println("Failed to obtain IP address (DHCP timeout).");
        return false;
    }

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
}
