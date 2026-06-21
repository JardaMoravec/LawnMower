#include "modules/network/sensorApi.h"

#include <WebServer.h>
#include <WiFi.h>

static WebServer server(80);
static Sht4xSensor *climateSensor = nullptr;
static Bmi270Sensor *motionSensor = nullptr;
static MotionEstimator *movementEstimator = nullptr;
static MotorController *motorController = nullptr;

struct ApiMotorTest {
    bool active;
    uint8_t motorId;
    uint8_t step;
    uint8_t targetSpeed;
    unsigned long stepStartMs;
};

static ApiMotorTest apiMotorTest = {false, 0, 0, 0, 0};

static const char *motorLabel(uint8_t motorId)
{
    return motorId == 1 ? "M1" : "M2";
}

static bool parseMotorTarget(const String &raw, int *motorId, bool allowBoth)
{
    if (raw.equalsIgnoreCase("both") || raw == "0")
    {
        if (!allowBoth)
        {
            return false;
        }
        *motorId = 0;
        return true;
    }

    if (raw.equalsIgnoreCase("M1") || raw.equalsIgnoreCase("m1"))
    {
        *motorId = 1;
        return true;
    }

    if (raw.equalsIgnoreCase("M2") || raw.equalsIgnoreCase("m2"))
    {
        *motorId = 2;
        return true;
    }

    *motorId = raw.toInt();
    if (*motorId == 1 || *motorId == 2)
    {
        return true;
    }
    if (allowBoth && *motorId == 0)
    {
        return true;
    }
    return false;
}

static void appendMotorStatusJson(char *body, size_t bodySize, size_t *offset, uint8_t motorId)
{
    const uint8_t vr = motorId == 1 ? 4 : 7;
    const uint8_t zf = motorId == 1 ? 5 : 15;
    const uint8_t el = motorId == 1 ? 6 : 16;
    const uint8_t signal = motorId == 1 ? 18 : 17;

    *offset += snprintf(body + *offset, bodySize - *offset,
                        "{\"id\":%u,\"label\":\"%s\","
                        "\"gpio\":{\"vr\":%u,\"zf\":%u,\"el\":%u,\"signal\":%u},"
                        "\"speed_percent\":%u,\"direction\":\"%s\",\"enabled\":%s,\"signal_pps\":%u}",
                        motorId, motorLabel(motorId), vr, zf, el, signal,
                        motorController->getSpeedPercent(motorId),
                        motorController->getForward(motorId) ? "forward" : "reverse",
                        motorController->isEnabled(motorId) ? "true" : "false",
                        motorController->getSignalPulsesPerSecond(motorId));
}

static void sendMotorsJson(uint8_t targetMotorId)
{
    char body[640];
    size_t offset = 0;

    offset += snprintf(body + offset, sizeof(body) - offset, "{\"valid\":true");
    if (targetMotorId >= 1 && targetMotorId <= 2)
    {
        offset += snprintf(body + offset, sizeof(body) - offset,
                           ",\"target\":{\"id\":%u,\"label\":\"%s\"}",
                           targetMotorId, motorLabel(targetMotorId));
    }
    else if (targetMotorId == 0)
    {
        offset += snprintf(body + offset, sizeof(body) - offset,
                           ",\"target\":{\"id\":0,\"label\":\"both\"}");
    }

    offset += snprintf(body + offset, sizeof(body) - offset, ",\"motors\":[");
    appendMotorStatusJson(body, sizeof(body), &offset, 1);
    offset += snprintf(body + offset, sizeof(body) - offset, ",");
    appendMotorStatusJson(body, sizeof(body), &offset, 2);
    offset += snprintf(body + offset, sizeof(body) - offset, "]}");
    server.send(200, "application/json", body);
}

static void applyMotorSpeed(int motorId, int speed, bool directionGiven, bool forward)
{
    if (motorId == 0)
    {
        if (directionGiven)
        {
            motorController->setMotor(1, static_cast<uint8_t>(speed), forward);
            motorController->setMotor(2, static_cast<uint8_t>(speed), forward);
        }
        else
        {
            motorController->setMotor(1, static_cast<uint8_t>(speed), motorController->getForward(1));
            motorController->setMotor(2, static_cast<uint8_t>(speed), motorController->getForward(2));
        }
        return;
    }

    motorController->setMotor(static_cast<uint8_t>(motorId), static_cast<uint8_t>(speed), forward);
}

static void tickApiMotorTest()
{
    if (!apiMotorTest.active || motorController == nullptr)
    {
        return;
    }

    const unsigned long now = millis();
    if (now - apiMotorTest.stepStartMs < 3000)
    {
        return;
    }

    motorController->tickSignalSampling();

    if (apiMotorTest.step == 0)
    {
        apiMotorTest.step = 1;
        apiMotorTest.targetSpeed = 30;
        apiMotorTest.stepStartMs = now;
        motorController->setMotor(apiMotorTest.motorId, 30, true);
        Serial.print("API test M");
        Serial.print(apiMotorTest.motorId);
        Serial.println(" -> 30%");
        return;
    }

    if (apiMotorTest.step == 1)
    {
        apiMotorTest.step = 2;
        apiMotorTest.targetSpeed = 60;
        apiMotorTest.stepStartMs = now;
        motorController->setMotor(apiMotorTest.motorId, 60, true);
        Serial.print("API test M");
        Serial.print(apiMotorTest.motorId);
        Serial.println(" -> 60%");
        return;
    }

    if (apiMotorTest.step == 2)
    {
        apiMotorTest.step = 3;
        apiMotorTest.targetSpeed = 100;
        apiMotorTest.stepStartMs = now;
        motorController->setMotor(apiMotorTest.motorId, 100, true);
        Serial.print("API test M");
        Serial.print(apiMotorTest.motorId);
        Serial.println(" -> 100%");
        return;
    }

    motorController->stop(apiMotorTest.motorId);
    Serial.print("API test M");
    Serial.print(apiMotorTest.motorId);
    Serial.println(" done");
    apiMotorTest.active = false;
}

static void handleRoot()
{
    server.send(200, "application/json",
                "{\"name\":\"LawnMower Sensor API\","
                "\"motors\":{\"M1\":{\"id\":1,\"vr\":4,\"zf\":5,\"el\":6,\"signal\":18},"
                "\"M2\":{\"id\":2,\"vr\":7,\"zf\":15,\"el\":16,\"signal\":17}},"
                "\"routes\":["
                "{\"method\":\"GET\",\"path\":\"/api/climate\",\"description\":\"SHT4x temperature and humidity\"},"
                "{\"method\":\"GET\",\"path\":\"/api/motion\",\"description\":\"BMI270 accelerometer and gyroscope\"},"
                "{\"method\":\"GET\",\"path\":\"/api/movement\",\"description\":\"Estimated speed and direction from IMU\"},"
                "{\"method\":\"GET\",\"path\":\"/api/motors\",\"description\":\"Motor status M1/M2 with GPIO and signal pps\"},"
                "{\"method\":\"GET|POST\",\"path\":\"/api/motors/speed?motor=1&speed=50&direction=forward\",\"description\":\"motor=1|M1 or 2|M2, motor=both pro oba\"},"
                "{\"method\":\"GET|POST\",\"path\":\"/api/motors/test?motor=1\",\"description\":\"API test 30/60/100% jen pro motor 1 nebo 2\"},"
                "{\"method\":\"GET\",\"path\":\"/api/motors/test\",\"description\":\"Stav beziciho API testu\"},"
                "{\"method\":\"GET|POST\",\"path\":\"/api/motors/stop?motor=1\",\"description\":\"Stop M1, M2 nebo both\"},"
                "{\"method\":\"GET|POST\",\"path\":\"/api/motors/emergency\",\"description\":\"Emergency stop all motors\"}"
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

static bool parseDirectionArg(const String &dir, bool *forward)
{
    if (dir.equalsIgnoreCase("forward") || dir.equalsIgnoreCase("fwd") || dir == "1")
    {
        *forward = true;
        return true;
    }
    if (dir.equalsIgnoreCase("reverse") || dir.equalsIgnoreCase("rev") ||
        dir.equalsIgnoreCase("backward") || dir.equalsIgnoreCase("back") || dir == "-1")
    {
        *forward = false;
        return true;
    }
    return false;
}

static void handleMotors()
{
    if (motorController == nullptr)
    {
        server.send(500, "application/json", "{\"valid\":false,\"error\":\"not_initialized\"}");
        return;
    }

    motorController->tickSignalSampling();
    sendMotorsJson(255);
}

static void handleMotorSpeed()
{
    if (motorController == nullptr)
    {
        server.send(500, "application/json", "{\"valid\":false,\"error\":\"not_initialized\"}");
        return;
    }

    if (!server.hasArg("motor") || !server.hasArg("speed"))
    {
        server.send(400, "application/json",
                    "{\"valid\":false,\"error\":\"missing motor or speed (motor=1|M1, 2|M2, both)\"}");
        return;
    }

    int motorId = 0;
    if (!parseMotorTarget(server.arg("motor"), &motorId, true))
    {
        server.send(400, "application/json",
                    "{\"valid\":false,\"error\":\"motor must be 1, 2, both or 0\"}");
        return;
    }

    const int speed = server.arg("speed").toInt();

    if (speed < 0 || speed > 100)
    {
        server.send(400, "application/json", "{\"valid\":false,\"error\":\"speed must be 0..100\"}");
        return;
    }

    bool forward = true;
    const bool directionGiven = server.hasArg("direction");
    if (directionGiven)
    {
        if (!parseDirectionArg(server.arg("direction"), &forward))
        {
            server.send(400, "application/json",
                        "{\"valid\":false,\"error\":\"direction must be forward or reverse\"}");
            return;
        }
    }
    else if (motorId == 1)
    {
        forward = motorController->getForward(1);
    }
    else if (motorId == 2)
    {
        forward = motorController->getForward(2);
    }
    else
    {
        forward = motorController->getForward(1);
    }

    applyMotorSpeed(motorId, speed, directionGiven, forward);

    Serial.print("API speed target=");
    Serial.print(motorId == 0 ? "both" : motorLabel(static_cast<uint8_t>(motorId)));
    Serial.print(" speed=");
    Serial.print(speed);
    Serial.print(" direction=");
    Serial.println(forward ? "forward" : "reverse");

    char body[384];
    snprintf(body, sizeof(body),
             "{\"valid\":true,\"target\":{\"id\":%d,\"label\":\"%s\"},"
             "\"speed_percent\":%d,\"direction\":\"%s\","
             "\"motors\":["
             "{\"id\":1,\"label\":\"M1\",\"speed_percent\":%u,\"enabled\":%s,\"signal_pps\":%u},"
             "{\"id\":2,\"label\":\"M2\",\"speed_percent\":%u,\"enabled\":%s,\"signal_pps\":%u}"
             "]}",
             motorId, motorId == 0 ? "both" : motorLabel(static_cast<uint8_t>(motorId)),
             speed, forward ? "forward" : "reverse",
             motorController->getSpeedPercent(1),
             motorController->isEnabled(1) ? "true" : "false",
             motorController->getSignalPulsesPerSecond(1),
             motorController->getSpeedPercent(2),
             motorController->isEnabled(2) ? "true" : "false",
             motorController->getSignalPulsesPerSecond(2));
    server.send(200, "application/json", body);
}

static void handleMotorTestStart()
{
    if (motorController == nullptr)
    {
        server.send(500, "application/json", "{\"valid\":false,\"error\":\"not_initialized\"}");
        return;
    }

    if (!server.hasArg("motor"))
    {
        server.send(400, "application/json",
                    "{\"valid\":false,\"error\":\"motor required: 1 or 2 (ne both)\"}");
        return;
    }

    int motorId = 0;
    if (!parseMotorTarget(server.arg("motor"), &motorId, false))
    {
        server.send(400, "application/json",
                    "{\"valid\":false,\"error\":\"motor must be 1 or 2\"}");
        return;
    }

    if (apiMotorTest.active)
    {
        server.send(409, "application/json", "{\"valid\":false,\"error\":\"test_already_running\"}");
        return;
    }

    motorController->stop(0);
    apiMotorTest.active = true;
    apiMotorTest.motorId = static_cast<uint8_t>(motorId);
    apiMotorTest.step = 0;
    apiMotorTest.targetSpeed = 0;
    apiMotorTest.stepStartMs = millis();

    Serial.print("API test start M");
    Serial.println(motorId);

    char body[256];
    snprintf(body, sizeof(body),
             "{\"valid\":true,\"test\":\"started\",\"target\":{\"id\":%d,\"label\":\"%s\"},"
             "\"sequence\":[30,60,100],\"step_ms\":3000,"
             "\"note\":\"Sleduj signal_pps pres GET /api/motors\"}",
             motorId, motorLabel(static_cast<uint8_t>(motorId)));
    server.send(200, "application/json", body);
}

static void handleMotorTestStatus()
{
    if (motorController == nullptr)
    {
        server.send(500, "application/json", "{\"valid\":false,\"error\":\"not_initialized\"}");
        return;
    }

    motorController->tickSignalSampling();

    if (!apiMotorTest.active)
    {
        server.send(200, "application/json", "{\"valid\":true,\"test\":\"idle\"}");
        return;
    }

    char body[320];
    snprintf(body, sizeof(body),
             "{\"valid\":true,\"test\":\"running\","
             "\"target\":{\"id\":%u,\"label\":\"%s\"},"
             "\"step\":%u,\"target_speed\":%u,"
             "\"signal_pps\":%u,"
             "\"motors\":["
             "{\"id\":1,\"label\":\"M1\",\"speed_percent\":%u,\"signal_pps\":%u},"
             "{\"id\":2,\"label\":\"M2\",\"speed_percent\":%u,\"signal_pps\":%u}"
             "]}",
             apiMotorTest.motorId, motorLabel(apiMotorTest.motorId),
             apiMotorTest.step, apiMotorTest.targetSpeed,
             motorController->getSignalPulsesPerSecond(apiMotorTest.motorId),
             motorController->getSpeedPercent(1),
             motorController->getSignalPulsesPerSecond(1),
             motorController->getSpeedPercent(2),
             motorController->getSignalPulsesPerSecond(2));
    server.send(200, "application/json", body);
}

static void handleMotorStop()
{
    if (motorController == nullptr)
    {
        server.send(500, "application/json", "{\"valid\":false,\"error\":\"not_initialized\"}");
        return;
    }

    apiMotorTest.active = false;

    int motorId = 0;
    if (server.hasArg("motor"))
    {
        if (!parseMotorTarget(server.arg("motor"), &motorId, true))
        {
            server.send(400, "application/json",
                        "{\"valid\":false,\"error\":\"motor must be 1, 2, both or 0\"}");
            return;
        }
    }

    motorController->stop(static_cast<uint8_t>(motorId));

    char body[256];
    snprintf(body, sizeof(body),
             "{\"valid\":true,\"target\":{\"id\":%d,\"label\":\"%s\"},"
             "\"motors\":["
             "{\"id\":1,\"label\":\"M1\",\"speed_percent\":%u,\"enabled\":%s},"
             "{\"id\":2,\"label\":\"M2\",\"speed_percent\":%u,\"enabled\":%s}"
             "]}",
             motorId, motorId == 0 ? "both" : motorLabel(static_cast<uint8_t>(motorId)),
             motorController->getSpeedPercent(1),
             motorController->isEnabled(1) ? "true" : "false",
             motorController->getSpeedPercent(2),
             motorController->isEnabled(2) ? "true" : "false");
    server.send(200, "application/json", body);
}

static void handleMotorEmergency()
{
    if (motorController == nullptr)
    {
        server.send(500, "application/json", "{\"valid\":false,\"error\":\"not_initialized\"}");
        return;
    }

    apiMotorTest.active = false;
    motorController->emergencyStop();
    server.send(200, "application/json",
                "{\"valid\":true,\"emergency\":true,"
                "\"motors\":["
                "{\"id\":1,\"label\":\"M1\",\"speed_percent\":0,\"enabled\":false},"
                "{\"id\":2,\"label\":\"M2\",\"speed_percent\":0,\"enabled\":false}"
                "]}");
}

void beginSensorApi(Sht4xSensor &climate, Bmi270Sensor &motion, MotionEstimator &movement,
                    MotorController &motors, uint16_t port)
{
    climateSensor = &climate;
    motionSensor = &motion;
    movementEstimator = &movement;
    motorController = &motors;

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/climate", HTTP_GET, handleClimate);
    server.on("/api/motion", HTTP_GET, handleMotion);
    server.on("/api/movement", HTTP_GET, handleMovement);
    server.on("/api/motors", HTTP_GET, handleMotors);
    server.on("/api/motors/speed", HTTP_GET, handleMotorSpeed);
    server.on("/api/motors/speed", HTTP_POST, handleMotorSpeed);
    server.on("/api/motors/test", HTTP_GET, handleMotorTestStatus);
    server.on("/api/motors/test", HTTP_POST, handleMotorTestStart);
    server.on("/api/motors/test/start", HTTP_GET, handleMotorTestStart);
    server.on("/api/motors/test/start", HTTP_POST, handleMotorTestStart);
    server.on("/api/motors/stop", HTTP_GET, handleMotorStop);
    server.on("/api/motors/stop", HTTP_POST, handleMotorStop);
    server.on("/api/motors/emergency", HTTP_GET, handleMotorEmergency);
    server.on("/api/motors/emergency", HTTP_POST, handleMotorEmergency);
    server.begin(port);

    Serial.print("Sensor API listening on http://");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.println(port);
    Serial.println("  GET  /");
    Serial.println("  GET  /api/motors");
    Serial.println("  GET|POST /api/motors/speed?motor=1&speed=50&direction=forward");
    Serial.println("  GET|POST /api/motors/test?motor=1");
    Serial.println("  GET  /api/motors/test");
    Serial.println("  GET|POST /api/motors/stop?motor=1");
    Serial.println("  GET|POST /api/motors/emergency");
}

void handleSensorApi()
{
    server.handleClient();
    tickApiMotorTest();
}
