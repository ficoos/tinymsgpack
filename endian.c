#include "endian.h"

#define SWAP16(x) ((uint16_t)(                  \
         (((uint16_t)(x) & 0x00ff) << 8)      | \
         (((uint16_t)(x) & 0xff00) >> 8)        \
          ))
#define SWAP32(x) ((uint32_t)(           \
         (((uint32_t)(x) & 0x000000ff) << 24) | \
         (((uint32_t)(x) & 0x0000ff00) <<  8) | \
         (((uint32_t)(x) & 0x00ff0000) >>  8) | \
         (((uint32_t)(x) & 0xff000000) >> 24)   \
         ))
#define SWAP64(val)                                             \
	((((uint64_t)(val) & 0x00000000000000ffULL) << 56)      \
	 | (((uint64_t)(val) & 0x000000000000ff00ULL) << 40)    \
	 | (((uint64_t)(val) & 0x0000000000ff0000ULL) << 24)    \
	 | (((uint64_t)(val) & 0x00000000ff000000ULL) << 8)     \
	 | (((uint64_t)(val) & 0x000000ff00000000ULL) >> 8)     \
	 | (((uint64_t)(val) & 0x0000ff0000000000ULL) >> 24)    \
	 | (((uint64_t)(val) & 0x00ff000000000000ULL) >> 40)    \
	 | (((uint64_t)(val) & 0xff00000000000000ULL) >> 56))

static uint8_t is_little_endian(void)
{
#if defined(__x86_64) || defined(__i386) || defined(_M_IX86) || defined(_M_X64)
   return 1;
#elif defined(MSB_FIRST)
   return 0;
#else
   union
   {
      uint16_t x;
      uint8_t y[2];
   } u;

   u.x = 1;
   return u.y[0];
#endif
}

uint16_t rmsgpack_swap_if_little_endian16(uint16_t value)
{
	if (is_little_endian())
	{
		return SWAP16(value);
	}

	return value;
}

uint32_t rmsgpack_swap_if_little_endian32(uint32_t value)
{
	if (is_little_endian())
	{
		return SWAP32(value);
	}

	return value;
}

uint64_t rmsgpack_swap_if_little_endian64(uint64_t value)
{
	if (is_little_endian())
	{
		return SWAP64(value);
	}

	return value;
}
