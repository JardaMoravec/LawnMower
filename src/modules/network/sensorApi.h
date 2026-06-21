#ifndef SENSOR_API_H
#define SENSOR_API_H

#include "modules/sensors/bmi270Sensor.h"
#include "modules/sensors/sht4xSensor.h"

void beginSensorApi(Sht4xSensor &climate, Bmi270Sensor &motion, uint16_t port = 80);
void handleSensorApi();

#endif
