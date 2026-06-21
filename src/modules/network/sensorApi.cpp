#include "modules/network/sensorApi.h"

#include <WebServer.h>
#include <WiFi.h>

static WebServer server(80);
static Sht4xSensor *climateSensor = nullptr;
static Bmi270Sensor *motionSensor = nullptr;
static MotionEstimator *movementEstimator = nullptr;

static void handleRoot()
{
    server.send(200, "application/json",
                "{\"name\":\"LawnMower Sensor API\","
                "\"routes\":["
                "{\"method\":\"GET\",\"path\":\"/api/climate\",\"description\":\"SHT4x temperature and humidity\"},"
                "{\"method\":\"GET\",\"path\":\"/api/motion\",\"description\":\"BMI270 accelerometer and gyroscope\"},"
                "{\"method\":\"GET\",\"path\":\"/api/movement\",\"description\":\"Estimated speed and direction from IMU\"}"
                "]}");
}

static void handleClimate()
{
    if (climateSensor == nullptr)
    {
        server.send(500, "application/json", "{\"valid\":false,\"error\":\"not_initialized\"}");
        return;
    }

    if (!climateSensor->isReady())
    {
        server.send(503, "application/json", "{\"valid\":false,\"error\":\"sensor_not_ready\"}");
        return;
    }

    Sht4xSensor::Reading reading;
    if (!climateSensor->read(reading) || !reading.valid)
    {
        server.send(503, "application/json", "{\"valid\":false,\"error\":\"read_failed\"}");
        return;
    }

    char body[128];
    snprintf(body, sizeof(body),
             "{\"valid\":true,\"temperature_c\":%.2f,\"humidity_percent\":%.2f}",
             reading.temperatureC, reading.humidityPercent);
    server.send(200, "application/json", body);
}

static void handleMotion()
{
    if (motionSensor == nullptr)
    {
        server.send(500, "application/json", "{\"valid\":false,\"error\":\"not_initialized\"}");
        return;
    }

    if (!motionSensor->isReady())
    {
        server.send(503, "application/json", "{\"valid\":false,\"error\":\"sensor_not_ready\"}");
        return;
    }

    Bmi270Sensor::Reading reading;
    if (!motionSensor->read(reading) || !reading.valid)
    {
        server.send(503, "application/json", "{\"valid\":false,\"error\":\"read_failed\"}");
        return;
    }

    char body[256];
    snprintf(body, sizeof(body),
             "{\"valid\":true,"
             "\"accel\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f},"
             "\"gyro\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f}}",
             reading.accelX, reading.accelY, reading.accelZ,
             reading.gyroX, reading.gyroY, reading.gyroZ);
    server.send(200, "application/json", body);
}

static void handleMovement()
{
    if (movementEstimator == nullptr)
    {
        server.send(500, "application/json", "{\"valid\":false,\"error\":\"not_initialized\"}");
        return;
    }

    const MotionEstimator::State &state = movementEstimator->getState();
    if (!state.valid)
    {
        server.send(503, "application/json", "{\"valid\":false,\"error\":\"estimate_not_ready\"}");
        return;
    }

    char body[192];
    snprintf(body, sizeof(body),
             "{\"valid\":true,\"moving\":%s,"
             "\"speed_mps\":%.3f,\"direction_deg\":%.1f,"
             "\"heading_deg\":%.1f,\"yaw_rate_dps\":%.2f}",
             state.moving ? "true" : "false",
             state.speedMps, state.directionDeg,
             state.headingDeg, state.yawRateDps);
    server.send(200, "application/json", body);
}

void beginSensorApi(Sht4xSensor &climate, Bmi270Sensor &motion, MotionEstimator &movement,
                    uint16_t port)
{
    climateSensor = &climate;
    motionSensor = &motion;
    movementEstimator = &movement;

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/climate", HTTP_GET, handleClimate);
    server.on("/api/motion", HTTP_GET, handleMotion);
    server.on("/api/movement", HTTP_GET, handleMovement);
    server.begin(port);

    Serial.print("Sensor API listening on http://");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.println(port);
    Serial.println("  GET /");
    Serial.println("  GET /api/climate");
    Serial.println("  GET /api/motion");
    Serial.println("  GET /api/movement");
}

void handleSensorApi()
{
    server.handleClient();
}
