#include <Arduino.h>
#include "wifiConnect.h"
#include "motorController.h"

const char *ssid = "JMHome";
const char *password = "84d1f54K95x";

MotorController motors(Serial2);

void setup()
{
    // Spuštění sériové komunikace na 115200 baud
    Serial.begin(115200);

    // Počkej max. 3 s na USB CDC (ESP32-S3), pak pokračuj i bez monitoru
    unsigned long serialWaitStart = millis();
    while (!Serial && millis() - serialWaitStart < 3000)
    {
        delay(10);
    }

    // Připoj se k Wi-Fi
    connectWiFi(ssid, password);

    // Inicializace motor controlleru
    motors.begin(9600);

    // Úvodní zpráva
    Serial.println("ESP32-S3 Demo Program");
    Serial.println("=====================");
}

void loop()
{
    // Výpis aktuálního času od startu desky (v ms)
    unsigned long currentMillis = millis();
    Serial.print("Čas od startu (ms): ");
    Serial.println(currentMillis);

    // Informace o desce
    Serial.print("Free Heap: ");
    Serial.println(esp_get_free_heap_size());

    Serial.print("Free Flash (ROM) bytes: ");
    Serial.println(ESP.getFreeSketchSpace());

    Serial.print("CPU Frequency: ");
    Serial.print(getCpuFrequencyMhz());
    Serial.println(" MHz");

    Serial.println("---------------------");

    //motors.setSpeed(1, 50);  // Motor 1 na 50% vpřed
    //motors.setSpeed(2, -30); // Motor 2 na 30% vzad
    //delay(2000);
    //motors.stop(0);

    // Pauza 1 sekunda
    delay(1000);
}
