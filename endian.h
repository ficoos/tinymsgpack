#ifndef __RARCHDB_ENDIAN_H__
#define __RARCHDB_ENDIAN_H__

#include <stdint.h>

uint16_t rmsgpack_swap_if_little_endian16(uint16_t value);

uint32_t rmsgpack_swap_if_little_endian32(uint32_t value);

uint64_t rmsgpack_swap_if_little_endian64(uint64_t value);

#endif
