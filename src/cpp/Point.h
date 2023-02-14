#pragma once

#include <cstdint>

namespace SignalScatter
{
    struct Point
    {
        uint32_t Id;
        float PositionX;
        float PositionY;
        float PositionZ;
    };
}