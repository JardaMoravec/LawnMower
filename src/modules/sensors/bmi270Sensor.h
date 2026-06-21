#ifndef BMI270_SENSOR_H
#define BMI270_SENSOR_H

#include <Arduino.h>

/**
 * @brief 6DoF IMU Bosch BMI270 (akcelerometr + gyroskop, I2C 0x68 / 0x69).
 * Datasheet: datasheeds/bst-bmi270-ds000.pdf
 */
class Bmi270Sensor
{
public:
    struct Reading
    {
        float accelX = 0.0f;
        float accelY = 0.0f;
        float accelZ = 0.0f;
        float gyroX = 0.0f;
        float gyroY = 0.0f;
        float gyroZ = 0.0f;
        bool valid = false;
    };

    bool begin(uint8_t i2cAddress = 0x68);
    bool read(Reading &out);
    bool isReady() const { return _ready; }

private:
    bool _ready = false;
};

#endif
