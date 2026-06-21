#ifndef MOTOR_TEST_H
#define MOTOR_TEST_H

#include "modules/motors/motorController.h"

/** Blokující test jednoho motoru (30 → 60 → 100 %), bez API. Vrací true po dokončení. */
void runMotorSpeedTest(MotorController &motors, uint8_t motorId);

/** Blokující test M1 a pak M2. */
void runMotorSpeedTestAll(MotorController &motors);

#endif
