#include "modules/sensors/ushupBus.h"

#include <Wire.h>

namespace UshupBus
{
    bool begin()
    {
        pinMode(PIN_POWER, OUTPUT);
        digitalWrite(PIN_POWER, HIGH);
        delay(50);

        Wire.begin(PIN_SDA, PIN_SCL);
        return true;
    }
}
