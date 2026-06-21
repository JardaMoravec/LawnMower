#include "modules/sensors/bmi270Sensor.h"

#include "SparkFun_BMI270_Arduino_Library.h"

static BMI270 imu;

bool Bmi270Sensor::begin(uint8_t i2cAddress)
{
    if (imu.beginI2C(i2cAddress) != BMI2_OK)
    {
        _ready = false;
        return false;
    }

    _ready = true;
    return true;
}

bool Bmi270Sensor::read(Reading &out)
{
    out.valid = false;

    if (!_ready)
    {
        return false;
    }

    if (imu.getSensorData() != BMI2_OK)
    {
        return false;
    }

    out.accelX = imu.data.accelX;
    out.accelY = imu.data.accelY;
    out.accelZ = imu.data.accelZ;
    out.gyroX = imu.data.gyroX;
    out.gyroY = imu.data.gyroY;
    out.gyroZ = imu.data.gyroZ;
    out.valid = true;
    return true;
}
