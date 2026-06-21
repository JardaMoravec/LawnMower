#include "modules/motors/motorTest.h"

namespace {
constexpr uint8_t TEST_SPEEDS[] = {30, 60, 100};
constexpr unsigned long STEP_HOLD_MS = 3000;
constexpr unsigned long PAUSE_MS = 1500;

void logMotorHeader(uint8_t motorId) {
    Serial.println();
    Serial.print("--- Motor test M");
    Serial.print(motorId);
    Serial.print(" (GPIO VR=");
    Serial.print(motorId == 1 ? 4 : 7);
    Serial.print(" EL=");
    Serial.print(motorId == 1 ? 6 : 16);
    Serial.println(") ---");
}

void holdAndSample(MotorController &motors, uint8_t motorId, uint8_t speed) {
    motors.setMotor(motorId, speed, true);
    Serial.print("  speed ");
    Serial.print(speed);
    Serial.println("% forward, 3 s...");

    const unsigned long start = millis();
    while (millis() - start < STEP_HOLD_MS) {
        motors.tickSignalSampling();
        delay(500);
        Serial.print("    signal pps: ");
        Serial.println(motors.getSignalPulsesPerSecond(motorId));
    }
}
} // namespace

void runMotorSpeedTest(MotorController &motors, uint8_t motorId) {
    if (motorId < 1 || motorId > 2) {
        return;
    }

    logMotorHeader(motorId);
    motors.stop(0);
    delay(PAUSE_MS);

    for (uint8_t speed : TEST_SPEEDS) {
        holdAndSample(motors, motorId, speed);
    }

    motors.stop(motorId);
    Serial.print("  M");
    Serial.print(motorId);
    Serial.println(" stop");
    delay(PAUSE_MS);
}

void runMotorSpeedTestAll(MotorController &motors) {
    Serial.println();
    Serial.println("=== Motor speed test (prime, bez API) ===");
    runMotorSpeedTest(motors, 1);
    runMotorSpeedTest(motors, 2);
    motors.stop(0);
    Serial.println("=== Motor test hotovo ===");
    Serial.println();
}
