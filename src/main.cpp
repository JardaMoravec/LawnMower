#include <Arduino.h>

#include "modules/motors/motorController.h"
#include "modules/motors/motorTest.h"
#include "modules/sensors/bmi270Sensor.h"
#include "modules/sensors/motionEstimator.h"
#include "modules/sensors/sht4xSensor.h"
#include "modules/sensors/ushupBus.h"
#include "modules/network/sensorApi.h"
#include "modules/network/wifiConnect.h"

const char *ssid = "JMHome";
const char *password = "84d1f54K95x";

// 1 = test motoru po startu (bez API), 0 = vypnuto
#define MOTOR_BOOT_TEST 1

MotorController motors;
Sht4xSensor sht4;
Bmi270Sensor imu;
MotionEstimator movement;

void setup()
{
    const bool motorsReady = motors.begin();
    Serial.begin(115200);
    if (!motorsReady)
    {
        Serial.println("Motor controller init failed");
    }
    delay(3000); // UART: čas na otevření Serial monitoru po resetu / uploadu
    motors.stop(0);

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

#if MOTOR_BOOT_TEST
    runMotorSpeedTestAll(motors);
#endif

    if (connectWiFi(ssid, password))
    {
        beginSensorApi(sht4, imu, movement, motors);
    }
    else
    {
        Serial.println("Sensor API not started (Wi-Fi unavailable).");
    }

    Serial.println("Setup complete.");
    Serial.println("Motor: EL off until API/command (speed != 0)");
    Serial.println("====================");
    motors.stop(0);
}

void loop()
{
    Bmi270Sensor::Reading imuReading;
    if (imu.read(imuReading))
    {
        movement.update(imuReading);
    }

    handleSensorApi();
    motors.tickSignalSampling();

    static unsigned long lastPrintMs = 0;
    const unsigned long now = millis();
    if (now - lastPrintMs < 1000)
    {
        return;
    }
    lastPrintMs = now;

    Serial.print("Motor: M1 ");
    Serial.print(motors.getSpeedPercent(1));
    Serial.print("% ");
    Serial.print(motors.getForward(1) ? "fwd" : "rev");
    Serial.print(", M2 ");
    Serial.print(motors.getSpeedPercent(2));
    Serial.print("% ");
    Serial.print(motors.getForward(2) ? "fwd" : "rev");
    Serial.print(" | Signal: M1 ");
    Serial.print(motors.getSignalPulsesPerSecond(1));
    Serial.print(" pps, M2 ");
    Serial.print(motors.getSignalPulsesPerSecond(2));
    Serial.print(" pps");

    const MotionEstimator::State &state = movement.getState();
    if (!state.valid)
    {
        Serial.println();
        return;
    }

    Serial.print(" | Movement: ");
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
