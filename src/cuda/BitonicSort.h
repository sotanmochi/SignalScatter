#ifndef _BITONIC_SORT_H_
#define _BITONIC_SORT_H_

#include <stdint.h>

#define KEY_TYPE   float
#define VALUE_TYPE uint32_t

// Enables maximum occupancy
#define SHARED_SIZE_LIMIT 1024U

extern "C" uint bitonicSort(KEY_TYPE *d_DstKey, VALUE_TYPE *d_DstVal, 
                            KEY_TYPE *d_SrcKey, VALUE_TYPE *d_SrcVal, 
                            uint batchSize, uint arrayLength, uint dir);

#endif // _BITONIC_SORT_H_