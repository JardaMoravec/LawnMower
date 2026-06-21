#ifndef USHUP_BUS_H
#define USHUP_BUS_H

#include <Arduino.h>

/**
 * @brief Společná I2C sběrnice uŠup konektoru na LaskaKit ESP32-S3-DEVKit.
 * GPIO 47 povoluje LDO pro uŠup I2C/SPI, SDA=42, SCL=2.
 */
namespace UshupBus
{
    constexpr uint8_t PIN_POWER = 47;
    constexpr uint8_t PIN_SDA = 42;
    constexpr uint8_t PIN_SCL = 2;

    /** Zapne napájení uŠup a inicializuje Wire. Vrátí true při úspěchu. */
    bool begin();
}

#endif
