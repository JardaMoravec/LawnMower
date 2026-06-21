#ifndef SENSOR_API_H
#define SENSOR_API_H

#include "modules/motors/motorController.h"
#include "modules/sensors/bmi270Sensor.h"
#include "modules/sensors/motionEstimator.h"
#include "modules/sensors/sht4xSensor.h"

void beginSensorApi(Sht4xSensor &climate, Bmi270Sensor &motion, MotionEstimator &movement,
                    MotorController &motors, uint16_t port = 80);
void handleSensorApi();

#endif
