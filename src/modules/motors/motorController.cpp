#include "modules/motors/motorController.h"

namespace {
MotorController* g_activeController = nullptr;
} // namespace

void IRAM_ATTR MotorController::onSignal1Pulse() {
    if (g_activeController != nullptr) {
        g_activeController->_signal1Pulses++;
    }
}

void IRAM_ATTR MotorController::onSignal2Pulse() {
    if (g_activeController != nullptr) {
        g_activeController->_signal2Pulses++;
    }
}

void MotorController::holdMotorsOff() {
    const uint8_t pwms[] = {4, 7};
    const uint8_t directions[] = {5, 15};
    const uint8_t enables[] = {6, 16};

    for (uint8_t i = 0; i < 2; ++i) {
        pinMode(enables[i], OUTPUT);
        digitalWrite(enables[i], LOW);
        pinMode(directions[i], INPUT);
        pinMode(pwms[i], OUTPUT);
        digitalWrite(pwms[i], LOW);
    }
}

MotorController::MotorController()
    : _motor1{0, true},
      _motor2{0, true},
      _initialized(false),
      _signal1Pulses(0),
      _signal2Pulses(0),
      _signal1LastSnapshot(0),
      _signal2LastSnapshot(0),
      _signal1Pps(0),
      _signal2Pps(0),
      _signalSampleMs(0) {
    holdMotorsOff();
}

MotorController::MotorPins MotorController::pinsForId(uint8_t motorId) {
    if (motorId == 1) {
        return {4, 5, 6, 18, 0};
    }
    return {7, 15, 16, 17, 1};
}

MotorController::MotorState& MotorController::stateFor(uint8_t motorId) {
    return motorId == 1 ? _motor1 : _motor2;
}

const MotorController::MotorState& MotorController::stateFor(uint8_t motorId) const {
    return motorId == 1 ? _motor1 : _motor2;
}

uint8_t MotorController::speedToPwmDuty(uint8_t speedPercent) {
    if (speedPercent == 0) {
        return 0;
    }

    const uint8_t minDuty = (255U * MIN_VR_DUTY_PERCENT) / 100U;
    return minDuty + (speedPercent * (255U - minDuty)) / 100U;
}

bool MotorController::begin() {
    holdMotorsOff();

    for (uint8_t motorId = 1; motorId <= 2; ++motorId) {
        const MotorPins pins = pinsForId(motorId);

        pinMode(pins.signal, INPUT);
        setDirectionPin(pins, true);
        setEnablePin(pins, false);

        setupPwm(pins);
        writePwmDuty(pins, 0);

        attachInterrupt(digitalPinToInterrupt(pins.signal),
                        motorId == 1 ? onSignal1Pulse : onSignal2Pulse,
                        RISING);
    }

    _motor1 = {0, true};
    _motor2 = {0, true};
    _signalSampleMs = millis();
    g_activeController = this;
    _initialized = true;
    return true;
}

void MotorController::setMotor(uint8_t motorId, uint8_t speedPercent, bool forward) {
    if (!_initialized || motorId < 1 || motorId > 2) {
        return;
    }

    if (speedPercent > 100) {
        speedPercent = 100;
    }

    MotorState& state = stateFor(motorId);
    state.speedPercent = speedPercent;
    state.forward = forward;

    applyMotor(pinsForId(motorId), motorId, speedPercent, forward);
}

void MotorController::stop(uint8_t motorId) {
    if (!_initialized) {
        return;
    }

    if (motorId == 0) {
        stop(1);
        stop(2);
        return;
    }

    setMotor(motorId, 0, stateFor(motorId).forward);
}

void MotorController::emergencyStop() {
    if (!_initialized) {
        return;
    }

    _motor1.speedPercent = 0;
    _motor2.speedPercent = 0;

    for (uint8_t motorId = 1; motorId <= 2; ++motorId) {
        forceMotorOff(pinsForId(motorId));
    }
}

uint8_t MotorController::getSpeedPercent(uint8_t motorId) const {
    if (motorId == 1 || motorId == 2) {
        return stateFor(motorId).speedPercent;
    }
    return 0;
}

bool MotorController::getForward(uint8_t motorId) const {
    if (motorId == 1 || motorId == 2) {
        return stateFor(motorId).forward;
    }
    return true;
}

bool MotorController::isEnabled(uint8_t motorId) const {
    return getSpeedPercent(motorId) > 0;
}

void MotorController::tickSignalSampling() {
    if (!_initialized) {
        return;
    }

    const unsigned long now = millis();
    if (now - _signalSampleMs < 1000) {
        return;
    }

    noInterrupts();
    const uint32_t count1 = _signal1Pulses;
    const uint32_t count2 = _signal2Pulses;
    interrupts();

    _signal1Pps = count1 - _signal1LastSnapshot;
    _signal2Pps = count2 - _signal2LastSnapshot;
    _signal1LastSnapshot = count1;
    _signal2LastSnapshot = count2;
    _signalSampleMs = now;
}

uint32_t MotorController::getSignalPulsesPerSecond(uint8_t motorId) const {
    if (motorId == 1) {
        return _signal1Pps;
    }
    if (motorId == 2) {
        return _signal2Pps;
    }
    return 0;
}

void MotorController::setupPwm(const MotorPins& pins) {
    ledcSetup(pins.ledcChannel, PWM_FREQUENCY_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(pins.pwm, pins.ledcChannel);
}

void MotorController::setDirectionPin(const MotorPins& pins, bool forward) {
    if (forward) {
        pinMode(pins.direction, INPUT);
        return;
    }

    pinMode(pins.direction, OUTPUT);
    digitalWrite(pins.direction, LOW);
}

void MotorController::setEnablePin(const MotorPins& pins, bool allowRun) {
    if (allowRun) {
        pinMode(pins.enable, INPUT);
        return;
    }

    pinMode(pins.enable, OUTPUT);
    digitalWrite(pins.enable, LOW);
}

void MotorController::forceMotorOff(const MotorPins& pins) {
    writePwmDuty(pins, 0);
    setEnablePin(pins, false);
}

void MotorController::applyMotor(const MotorPins& pins, uint8_t motorId, uint8_t speedPercent, bool forward) {
    if (speedPercent == 0) {
        forceMotorOff(pins);
        return;
    }

    const uint8_t duty = speedToPwmDuty(speedPercent);

    setDirectionPin(pins, forward);
    writePwmDuty(pins, duty);
    setEnablePin(pins, true);

    Serial.print("Motor ");
    Serial.print(motorId);
    Serial.print(" run: duty=");
    Serial.print(duty);
    Serial.print(" (");
    Serial.print(speedPercent);
    Serial.print("%), dir=");
    Serial.println(forward ? "fwd" : "rev");
}

void MotorController::writePwmDuty(const MotorPins& pins, uint8_t duty) {
    ledcWrite(pins.ledcChannel, duty);
}
