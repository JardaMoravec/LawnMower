#ifndef SHT4X_SENSOR_H
#define SHT4X_SENSOR_H

#include <Arduino.h>

/**
 * @brief Senzor teploty a vlhkosti Sensirion SHT4x (I2C 0x44 / 0x45).
 * Datasheet: datasheeds/sht4x.pdf
 */
class Sht4xSensor
{
public:
    struct Reading
    {
        float temperatureC = 0.0f;
        float humidityPercent = 0.0f;
        bool valid = false;
    };

    bool begin();
    bool read(Reading &out);
    bool isReady() const { return _ready; }

private:
    bool _ready = false;
};

#endif
