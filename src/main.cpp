#include <Arduino.h>
#include "modules/motorController.h"
#include "modules/sensors/bmi270Sensor.h"
#include "modules/sensors/sht4xSensor.h"
#include "modules/sensors/ushupBus.h"
#include "modules/wifiConnect.h"

const char *ssid = "JMHome";
const char *password = "84d1f54K95x";

MotorController motors(Serial2);
Sht4xSensor sht4;
Bmi270Sensor imu;

void setup()
{
    Serial.begin(115200);

    unsigned long serialWaitStart = millis();
    while (!Serial && millis() - serialWaitStart < 3000)
    {
        delay(10);
    }

    if (!UshupBus::begin())
    {
        Serial.println("uŠup I2C init failed");
    }

    if (!sht4.begin())
    {
        Serial.println("SHT4x not found on uŠup I2C");
    }
    else
    {
        Serial.println("SHT4x ready");
    }

    if (!imu.begin())
    {
        Serial.println("BMI270 not found on uŠup I2C (try address 0x69 if wired)");
    }
    else
    {
        Serial.println("BMI270 ready");
    }

    connectWiFi(ssid, password);
    motors.begin(9600);

    Serial.println("ESP32-S3 LawnMower");
    Serial.println("====================");
}

void loop()
{
    Sht4xSensor::Reading climate;
    if (sht4.read(climate) && climate.valid)
    {
        Serial.print("Temp: ");
        Serial.print(climate.temperatureC, 2);
        Serial.print(" C, Humidity: ");
        Serial.print(climate.humidityPercent, 2);
        Serial.println(" %");
    }

    Bmi270Sensor::Reading motion;
    if (imu.read(motion) && motion.valid)
    {
        Serial.print("Accel g  X:");
        Serial.print(motion.accelX, 3);
        Serial.print(" Y:");
        Serial.print(motion.accelY, 3);
        Serial.print(" Z:");
        Serial.print(motion.accelZ, 3);
        Serial.print("  Gyro dps X:");
        Serial.print(motion.gyroX, 2);
        Serial.print(" Y:");
        Serial.print(motion.gyroY, 2);
        Serial.print(" Z:");
        Serial.println(motion.gyroZ, 2);
    }

    Serial.println("---------------------");
    delay(1000);
}
