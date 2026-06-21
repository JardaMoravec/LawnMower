#include "modules/sensors/motionEstimator.h"

#include <math.h>

namespace
{
constexpr float kGravityMps2 = 9.80665f;
constexpr float kComplementaryAlpha = 0.98f;
constexpr float kMaxDtSec = 0.2f;
constexpr float kStationaryAccelMagG = 0.08f; // | |a| - 1g |
constexpr float kStationaryGyroDps = 8.0f;
constexpr float kMovingSpeedMps = 0.05f;
constexpr float kMaxSpeedMps = 3.0f; // rozumný strop pro sekačku
} // namespace

void MotionEstimator::reset()
{
    _state = {};
    _rollDeg = 0.0f;
    _pitchDeg = 0.0f;
    _yawDeg = 0.0f;
    _velocityX = 0.0f;
    _velocityY = 0.0f;
    _lastUpdateMs = 0;
    _initialized = false;
}

float MotionEstimator::normalizeDegrees(float angle)
{
    while (angle < 0.0f)
    {
        angle += 360.0f;
    }
    while (angle >= 360.0f)
    {
        angle -= 360.0f;
    }
    return angle;
}

bool MotionEstimator::update(const Bmi270Sensor::Reading &imu)
{
    _state.valid = false;

    if (!imu.valid)
    {
        return false;
    }

    const float accelRollDeg = atan2f(imu.accelY, imu.accelZ) * 180.0f / PI;
    const float accelPitchDeg =
        atan2f(-imu.accelX, sqrtf(imu.accelY * imu.accelY + imu.accelZ * imu.accelZ)) * 180.0f / PI;

    const float accelMagG =
        sqrtf(imu.accelX * imu.accelX + imu.accelY * imu.accelY + imu.accelZ * imu.accelZ);
    const float gyroMagDps =
        sqrtf(imu.gyroX * imu.gyroX + imu.gyroY * imu.gyroY + imu.gyroZ * imu.gyroZ);
    const bool stationary =
        fabsf(accelMagG - 1.0f) < kStationaryAccelMagG && gyroMagDps < kStationaryGyroDps;

    const unsigned long nowMs = millis();
    if (!_initialized)
    {
        _rollDeg = accelRollDeg;
        _pitchDeg = accelPitchDeg;
        _lastUpdateMs = nowMs;
        _initialized = true;
        _state.headingDeg = normalizeDegrees(_yawDeg);
        _state.yawRateDps = imu.gyroZ;
        _state.valid = true;
        return true;
    }

    float dt = (nowMs - _lastUpdateMs) / 1000.0f;
    _lastUpdateMs = nowMs;

    if (dt <= 0.0f || dt > kMaxDtSec)
    {
        return false;
    }

    if (stationary)
    {
        _rollDeg = accelRollDeg;
        _pitchDeg = accelPitchDeg;
        _velocityX = 0.0f;
        _velocityY = 0.0f;
    }
    else
    {
        _rollDeg = kComplementaryAlpha * (_rollDeg + imu.gyroX * dt) +
                   (1.0f - kComplementaryAlpha) * accelRollDeg;
        _pitchDeg = kComplementaryAlpha * (_pitchDeg + imu.gyroY * dt) +
                    (1.0f - kComplementaryAlpha) * accelPitchDeg;

        const float rollRad = _rollDeg * DEG_TO_RAD;
        const float pitchRad = _pitchDeg * DEG_TO_RAD;
        const float yawRad = _yawDeg * DEG_TO_RAD;

        const float gravityX = sinf(pitchRad);
        const float gravityY = -sinf(rollRad) * cosf(pitchRad);

        const float linearAccelX = imu.accelX - gravityX;
        const float linearAccelY = imu.accelY - gravityY;

        const float cosYaw = cosf(yawRad);
        const float sinYaw = sinf(yawRad);
        const float worldAccelX = linearAccelX * cosYaw - linearAccelY * sinYaw;
        const float worldAccelY = linearAccelX * sinYaw + linearAccelY * cosYaw;

        _velocityX += worldAccelX * kGravityMps2 * dt;
        _velocityY += worldAccelY * kGravityMps2 * dt;
    }

    _yawDeg += imu.gyroZ * dt;

    _state.speedMps = sqrtf(_velocityX * _velocityX + _velocityY * _velocityY);
    if (_state.speedMps > kMaxSpeedMps)
    {
        _velocityX = 0.0f;
        _velocityY = 0.0f;
        _state.speedMps = 0.0f;
    }

    _state.headingDeg = normalizeDegrees(_yawDeg);
    _state.yawRateDps = imu.gyroZ;
    _state.moving = _state.speedMps >= kMovingSpeedMps;
    _state.directionDeg =
        _state.moving
            ? normalizeDegrees(atan2f(_velocityY, _velocityX) * 180.0f / PI)
            : _state.headingDeg;
    _state.valid = true;
    return true;
}
