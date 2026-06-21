#include "motorController.h"

// Definice příkazů pro JYQD V7.3E2
#define CMD_HEADER 0xAA
#define CMD_SPEED 0x01
#define CMD_STOP 0x02
#define CMD_STATUS 0x03

MotorController::MotorController(HardwareSerial& serial) 
    : _serial(serial), _motor1Speed(0), _motor2Speed(0), _initialized(false) {
}

bool MotorController::begin(uint32_t baudRate) {
    _serial.begin(baudRate);
    delay(100); // Počkejme na inicializaci sériového portu
    
    // Reset motorů při startu
    stop(0);
    
    _initialized = true;
    return true;
}

void MotorController::setSpeed(uint8_t motorId, int8_t speed) {
    if (!_initialized || motorId < 1 || motorId > 2) {
        return;
    }

    // Omezení rychlosti na rozsah -100 až 100
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;

    // Uložení aktuální rychlosti
    if (motorId == 1) {
        _motor1Speed = speed;
    } else {
        _motor2Speed = speed;
    }

    // Příprava příkazu
    // Formát: [HEADER, CMD, MOTOR_ID, DIRECTION, SPEED, CHECKSUM]
    uint8_t direction = (speed >= 0) ? 0x01 : 0x02; // 0x01 = vpřed, 0x02 = vzad
    uint8_t absSpeed = abs(speed);
    
    uint8_t command[6];
    command[0] = CMD_HEADER;
    command[1] = CMD_SPEED;
    command[2] = motorId;
    command[3] = direction;
    command[4] = absSpeed;
    command[5] = calculateChecksum(command, 5);

    sendCommand(command, 6);
}

void MotorController::stop(uint8_t motorId) {
    if (!_initialized) {
        return;
    }

    if (motorId == 0) {
        // Zastavení obou motorů
        stop(1);
        stop(2);
        return;
    }

    if (motorId < 1 || motorId > 2) {
        return;
    }

    // Vynulování rychlosti
    if (motorId == 1) {
        _motor1Speed = 0;
    } else {
        _motor2Speed = 0;
    }

    // Příprava příkazu pro zastavení
    uint8_t command[4];
    command[0] = CMD_HEADER;
    command[1] = CMD_STOP;
    command[2] = motorId;
    command[3] = calculateChecksum(command, 3);

    sendCommand(command, 4);
}

void MotorController::emergencyStop() {
    if (!_initialized) {
        return;
    }

    // Okamžité zastavení všech motorů
    _motor1Speed = 0;
    _motor2Speed = 0;
    
    stop(0);
    
    // Odeslání nouzového stop příkazu
    uint8_t command[3];
    command[0] = CMD_HEADER;
    command[1] = CMD_STOP;
    command[2] = 0x00; // 0x00 = emergency stop pro všechny motory
    
    sendCommand(command, 3);
}

int8_t MotorController::getSpeed(uint8_t motorId) {
    if (motorId == 1) {
        return _motor1Speed;
    } else if (motorId == 2) {
        return _motor2Speed;
    }
    return 0;
}

bool MotorController::isRunning(uint8_t motorId) {
    if (motorId == 1) {
        return _motor1Speed != 0;
    } else if (motorId == 2) {
        return _motor2Speed != 0;
    }
    return false;
}

void MotorController::sendCommand(const uint8_t* command, size_t length) {
    // Odeslání příkazu přes sériový port
    _serial.write(command, length);
    _serial.flush();
    
    // Malé zpoždění pro zpracování příkazu
    delay(10);
}

uint8_t MotorController::calculateChecksum(const uint8_t* data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= data[i]; // XOR checksum
    }
    return checksum;
}
