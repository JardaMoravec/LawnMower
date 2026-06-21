#ifndef MOTION_ESTIMATOR_H
#define MOTION_ESTIMATOR_H

#include "modules/sensors/bmi270Sensor.h"

/**
 * @brief Odhad rychlosti a směru pohybu z BMI270 (akcelerometr + gyro).
 *
 * Směr a rychlost vodorovného pohybu se odvozují integrací lineárního zrychlení
 * po odečtení gravitace. Při klidu se rychlost nuluje (ZUPT).
 *
 * Bez GPS nebo enkodérů kol integrace driftuje — vhodné pro krátkodobý odhad
 * a detekci „jede / stojí“, ne pro přesnou navigaci.
 */
class MotionEstimator
{
public:
    struct State
    {
        float speedMps = 0.0f;      // odhadovaná rychlost v m/s (vodorovná rovina)
        float directionDeg = 0.0f;  // směr pohybu 0–360°, 0 = osa +X senzoru
        float headingDeg = 0.0f;    // natočení zařízení kolem svislé osy (integr. gyro Z)
        float yawRateDps = 0.0f;    // okamžitá rychlost otáčení (°/s)
        bool moving = false;
        bool valid = false;
    };

    void reset();

    /** @return true pokud byl stav úspěšně aktualizován */
    bool update(const Bmi270Sensor::Reading &imu);

    const State &getState() const { return _state; }

private:
    State _state{};
    float _rollDeg = 0.0f;
    float _pitchDeg = 0.0f;
    float _yawDeg = 0.0f;
    float _velocityX = 0.0f;
    float _velocityY = 0.0f;
    unsigned long _lastUpdateMs = 0;
    bool _initialized = false;

    static float normalizeDegrees(float angle);
};

#endif
