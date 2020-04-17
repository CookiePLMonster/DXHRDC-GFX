#include <Windows.h>
#include "Metadata.h"

#include <map>

static std::map<int, bool> pressedKeys;
static std::map<int, int> enabledKeys;

int Effects::KeyToggled(int vk, int min, int max)
{
    auto& pressed = pressedKeys[vk];
    auto& enabled = enabledKeys.try_emplace(vk, 1).first->second;

    if ( GetAsyncKeyState(vk) & 0x8000 )
    {
        // If wasn't pressed, flip state
        if ( !pressed )
        {
            pressed = true;
            enabled++;
            if ( enabled > max )
            {
                enabled = min;
            }
        }
    }
    else
    {
        pressed = false;
    }
    return enabled;
}