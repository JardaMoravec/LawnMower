#include <Arduino.h>
#include "modules/motors/motorController.h"
#include "modules/sensors/bmi270Sensor.h"
#include "modules/sensors/motionEstimator.h"
#include "modules/sensors/sht4xSensor.h"
#include "modules/sensors/ushupBus.h"
#include "modules/network/sensorApi.h"
#include "modules/network/wifiConnect.h"

const char *ssid = "JMHome";
const char *password = "84d1f54K95x";

MotorController motors(Serial2);
Sht4xSensor sht4;
Bmi270Sensor imu;
MotionEstimator movement;

void setup()
{
    Serial.begin(115200);
    delay(3000); // UART: čas na otevření Serial monitoru po resetu / uploadu

    Serial.println();
    Serial.println("ESP32-S3 LawnMower");
    Serial.println("====================");

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

    if (connectWiFi(ssid, password))
    {
        beginSensorApi(sht4, imu, movement);
    }
    else
    {
        Serial.println("Sensor API not started (Wi-Fi unavailable).");
    }
    motors.begin(9600);

    Serial.println("Setup complete.");
    Serial.println("====================");
}

void loop()
{
    Bmi270Sensor::Reading imuReading;
    if (imu.read(imuReading))
    {
        movement.update(imuReading);
    }

    handleSensorApi();

    static unsigned long lastPrintMs = 0;
    const unsigned long now = millis();
    if (now - lastPrintMs < 1000)
    {
        return;
    }
    lastPrintMs = now;

    const MotionEstimator::State &state = movement.getState();
    if (!state.valid)
    {
        return;
    }

    Serial.print("Movement: ");
    Serial.print(state.moving ? "moving" : "stopped");
    Serial.print(", speed ");
    Serial.print(state.speedMps, 3);
    Serial.print(" m/s, direction ");
    Serial.print(state.directionDeg, 1);
    Serial.print(" deg, heading ");
    Serial.print(state.headingDeg, 1);
    Serial.print(" deg, yaw rate ");
    Serial.print(state.yawRateDps, 2);
    Serial.println(" dps");
}
