#include "Sonoff.h"

uint8_t Sonoff::_pins[8];
uint8_t Sonoff::_cmask = 0;
uint8_t Sonoff::_count = 0;

uint8_t Sonoff::count() // Geter
{
    return Sonoff::_count;
}

uint8_t Sonoff::cmask() // Geter
{
    return Sonoff::_cmask;
}

uint8_t *Sonoff::pins() // Geter
{
    return Sonoff::_pins;
}

bool Sonoff::begin(String path)
{
    File file = LittleFS.open(path, "r");

    while (file.available())
    {
        unsigned short pin = 255;
        unsigned short state = 255;
        String csd = file.readStringUntil(';');
        sscanf(csd.c_str(), "%hu:%hu", &pin, &state);

        if (pin != 255 || state != 255)
        {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, state);
            Sonoff::_pins[Sonoff::_count] = pin;
            Sonoff::_count++;
        }
    }

    file.close();
    return (bool)Sonoff::_count;
};

bool Sonoff::read(uint8_t index)
{
    return ((index < Sonoff::_count)
                ? digitalRead(Sonoff::_pins[index])
                : 0);
}

bool Sonoff::write(uint8_t index, uint8_t state)
{
    if (index < Sonoff::_count)
    {
        bool currentState = digitalRead(Sonoff::_pins[index]);

        if (state > 1)
            digitalWrite(Sonoff::_pins[index], !currentState);
        else if (state != currentState)
            digitalWrite(Sonoff::_pins[index], state);
        else
            return false; /* No Pin Affected */
        Sonoff::_cmask |= (1 << index);

#ifdef ENABLE_SONOFF_EVENT
        Sonoff::task.restartDelayed(Sonoff::delay);
#endif
        return true;
    }

    bool hasChanged = false;
    for (uint8_t i = 0; i < Sonoff::_count; i++)
    {
        hasChanged = Sonoff::write(i, state) || hasChanged;
    }

    return hasChanged;
}

String Sonoff::reads(uint8_t index)
{
    if (index < Sonoff::_count)
    {
        char csd[7];
        sprintf(csd, "%u:%u", index, Sonoff::read(index));
        return String(csd);
    }

    String result;

    if (index == 255)
    {
        for (uint8_t i = 0; i < Sonoff::_count; i++)
            if (Sonoff::_cmask & (1 << i))
                result += Sonoff::reads(i) + ";";

        result.remove(result.length() - 1);
        Sonoff::_cmask = 0;
        return result;
    }

    for (uint8_t i = 0; i < Sonoff::_count; i++)
    {
        result += Sonoff::reads(i) + ";";
    }

    result.remove(result.length() - 1);
    return result;
}

bool Sonoff::writes(char *extrw)
{
    char *token = strtok(extrw, ";");
    bool hasChanged = false;
    while (token != NULL)
    {
        unsigned short index = 255;
        unsigned short state = 255;
        sscanf(token, "%hu:%hu", &index, &state);
        hasChanged = Sonoff::write(index, state) || hasChanged;
        token = strtok(NULL, ";");
    }
    return hasChanged;
}

void Sonoff::reset()
{
    Sonoff::_cmask = 0;
}

bool Sonoff::press(uint64_t value)
{
    bool hasChanged = false;
    switch (value)
    {
    case IR_POWER:
        hasChanged = Sonoff::write(128, HIGH);
        break;
    case IR_MUTE:
        hasChanged = Sonoff::write(128, LOW);
        break;
    case IR_MODE:
        hasChanged = Sonoff::write(128);
        break;
    case IR_1:
        hasChanged = Sonoff::write(0);
        break;
    case IR_2:
        hasChanged = Sonoff::write(1);
        break;
    case IR_3:
        hasChanged = Sonoff::write(2);
        break;
    case IR_4:
        hasChanged = Sonoff::write(3);
        break;
    default:
        return false;
    }
    return hasChanged;
}

#ifdef ENABLE_SONOFF_EVENT
Task Sonoff::task;
uint32_t Sonoff::delay = 0;

void Sonoff::taskSetup(Scheduler &ts, TaskCallback cb, uint32_t delay, bool check)
{
    if (check && !Sonoff::_count)
        return;

    Sonoff::task.set(0, 1, cb);
    ts.addTask(Sonoff::task);
    Sonoff::delay = delay;
}
#endif