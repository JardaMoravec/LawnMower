#include "modules/sensors/sht4xSensor.h"

#include <Adafruit_SHT4x.h>

static Adafruit_SHT4x sht4;

bool Sht4xSensor::begin()
{
    if (!sht4.begin())
    {
        _ready = false;
        return false;
    }

    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
    _ready = true;
    return true;
}

bool Sht4xSensor::read(Reading &out)
{
    out.valid = false;

    if (!_ready)
    {
        return false;
    }

    sensors_event_t humidity;
    sensors_event_t temperature;

    if (!sht4.getEvent(&humidity, &temperature))
    {
        return false;
    }

    out.temperatureC = temperature.temperature;
    out.humidityPercent = humidity.relative_humidity;
    out.valid = true;
    return true;
}
