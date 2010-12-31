/****************************************************************************
 * 
 * This header file contains some type definitions needed.
 * 
 * "Word" (unsigned word) and "SWord" (signed word) represent MIPS
 * registers, and were introduced to allow a better debugging; by using them
 * appropriately, it was possible to detect where possibly incorrect
 * manipulation of register values and format were done.
 *
 ****************************************************************************/

#ifndef UMPS_TYPES_H
#define UMPS_TYPES_H

#include "base/basic_types.h"

typedef uint32_t Word;

typedef int32_t SWord;

#endif // UMPS_TYPES_H
