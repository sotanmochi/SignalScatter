#pragma once

#include "../../../src/cpp/Point.h"
#include <cstdint>
#include <string>

namespace SignalScatter
{
    class BitonicSortSample
    {
        public:
            BitonicSortSample(uint32_t size);
            ~BitonicSortSample();
            void Run();

        private:
            uint32_t _size;

            Point* _points;
            Point *_d_PointList;

            float *_distanceMatrix;
            uint32_t *_packedIdMatrix;

            float *_d_DistanceMatrixOut;
            uint32_t *_d_PackedIdMatrixOut;

            float *_d_DistanceMatrix;
            uint32_t *_d_PackedIdMatrix;

            size_t _pointListMemorySize;
            size_t _distanceMatrixMemorySize;
            size_t _packedIdMatrixMemorySize;
    };
}