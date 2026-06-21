#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>

/**
 * @brief Ovládání JYQD V7.3E2 přes PWM (VR), směr (Z/F) a enable (EL).
 *
 * JYQD: EL/Z/F = 5V nebo volno (povoleno/vpřed), GND = zakázáno/vzad.
 * Nepoužívat OUTPUT HIGH — stačí INPUT (float) nebo OUTPUT LOW.
 */
class MotorController {
public:
    MotorController();

    bool begin();

    /**
     * @param speedPercent 0–100; 0 = motor úplně vypnut (EL → GND, PWM 0)
     * @param forward true = vpřed, false = vzad
     */
    void setMotor(uint8_t motorId, uint8_t speedPercent, bool forward);

    /** motorId 0 = oba motory. */
    void stop(uint8_t motorId = 0);

    void emergencyStop();

    uint8_t getSpeedPercent(uint8_t motorId) const;
    bool getForward(uint8_t motorId) const;
    bool isEnabled(uint8_t motorId) const;

    void tickSignalSampling();
    uint32_t getSignalPulsesPerSecond(uint8_t motorId) const;

private:
    struct MotorPins {
        uint8_t pwm;
        uint8_t direction;
        uint8_t enable;
        uint8_t signal;
        uint8_t ledcChannel;
    };

    struct MotorState {
        uint8_t speedPercent;
        bool forward;
    };

    static constexpr uint32_t PWM_FREQUENCY_HZ = 10000;
    static constexpr uint8_t PWM_RESOLUTION_BITS = 8;
    static constexpr uint8_t MIN_VR_DUTY_PERCENT = 50;

    static MotorPins pinsForId(uint8_t motorId);
    static uint8_t speedToPwmDuty(uint8_t speedPercent);
    static void holdMotorsOff();

    MotorState& stateFor(uint8_t motorId);
    const MotorState& stateFor(uint8_t motorId) const;

    MotorState _motor1;
    MotorState _motor2;
    bool _initialized;

    volatile uint32_t _signal1Pulses;
    volatile uint32_t _signal2Pulses;
    uint32_t _signal1LastSnapshot;
    uint32_t _signal2LastSnapshot;
    uint32_t _signal1Pps;
    uint32_t _signal2Pps;
    unsigned long _signalSampleMs;

    static void IRAM_ATTR onSignal1Pulse();
    static void IRAM_ATTR onSignal2Pulse();

    void setupPwm(const MotorPins& pins);
    void setDirectionPin(const MotorPins& pins, bool forward);
    void setEnablePin(const MotorPins& pins, bool allowRun);
    void forceMotorOff(const MotorPins& pins);
    void applyMotor(const MotorPins& pins, uint8_t motorId, uint8_t speedPercent, bool forward);
    void writePwmDuty(const MotorPins& pins, uint8_t duty);
};

#endif // MOTOR_CONTROLLER_H
