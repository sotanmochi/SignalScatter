// Copyright (c) 2022 Soichiro Sugimoto
// Licensed under the MIT License.

#pragma once

#include <cstdint>

namespace SignalScatter
{
    struct ByteSpan
    {
        uint8_t* Pointer;
        int Length;

        ByteSpan()
        {
            Length = 0;
        }

        ByteSpan(uint8_t* pointer, int length)
        {
            Pointer = pointer;
            Length = length;
        }
    };
}