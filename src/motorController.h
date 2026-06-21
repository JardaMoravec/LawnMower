#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>

/**
 * @brief Třída pro ovládání JYQD V7.3E2 BLDC motor controller
 * 
 * Shield podporuje sériovou komunikaci pro ovládání bezkartáčových motorů.
 * Typicky se používá UART komunikace s příkazy pro nastavení rychlosti a směru.
 */
class MotorController {
public:
    /**
     * @brief Konstruktor
     * @param serial Reference na sériový port (např. Serial1, Serial2)
     */
    MotorController(HardwareSerial& serial);

    /**
     * @brief Inicializace motor controller
     * @param baudRate Přenosová rychlost (typicky 9600)
     * @return true pokud inicializace proběhla úspěšně
     */
    bool begin(uint32_t baudRate = 9600);

    /**
     * @brief Nastavení rychlosti motoru
     * @param motorId ID motoru (1 nebo 2 pro dva motory)
     * @param speed Rychlost -100 až 100 (záporné hodnoty = zpětný chod)
     */
    void setSpeed(uint8_t motorId, int8_t speed);

    /**
     * @brief Zastavení motoru
     * @param motorId ID motoru (1 nebo 2), 0 = oba motory
     */
    void stop(uint8_t motorId = 0);

    /**
     * @brief Nouzové zastavení všech motorů
     */
    void emergencyStop();

    /**
     * @brief Získání aktuální rychlosti motoru
     * @param motorId ID motoru (1 nebo 2)
     * @return Aktuální rychlost -100 až 100
     */
    int8_t getSpeed(uint8_t motorId);

    /**
     * @brief Získání informace o stavu motoru
     * @param motorId ID motoru (1 nebo 2)
     * @return true pokud motor běží
     */
    bool isRunning(uint8_t motorId);

private:
    HardwareSerial& _serial;
    int8_t _motor1Speed;
    int8_t _motor2Speed;
    bool _initialized;

    /**
     * @brief Odeslání příkazu na controller
     * @param command Pole bytů s příkazem
     * @param length Délka příkazu
     */
    void sendCommand(const uint8_t* command, size_t length);

    /**
     * @brief Výpočet kontrolního součtu
     * @param data Pole bytů
     * @param length Délka dat
     * @return Kontrolní součet
     */
    uint8_t calculateChecksum(const uint8_t* data, size_t length);
};

#endif // MOTOR_CONTROLLER_H
