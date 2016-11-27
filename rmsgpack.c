/*
 * An almost complete implementation of msgpack by
 * Saggi Mizrahi <ficoos@gmail.com>
 *
 * TODO:
 *  - ext types
 *
 * For more information http://msgpack.org/
 */

#include "rmsgpack.h"

#include <stdlib.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "endian.h"

static const uint8_t MPF_FIXMAP = 0x80;
static const uint8_t MPF_MAP16 = 0xde;
static const uint8_t MPF_MAP32 = 0xdf;

static const uint8_t MPF_FIXARRAY = 0x90;
static const uint8_t MPF_ARRAY16 = 0xdc;
static const uint8_t MPF_ARRAY32 = 0xdd;

static const uint8_t MPF_FIXSTR = 0xa0;
static const uint8_t MPF_STR8 = 0xd9;
static const uint8_t MPF_STR16 = 0xda;
static const uint8_t MPF_STR32 = 0xdb;

static const uint8_t MPF_BIN8 = 0xc4;
static const uint8_t MPF_BIN16  = 0xc5;
static const uint8_t MPF_BIN32 = 0xc6;

static const uint8_t MPF_FALSE = 0xc2;
static const uint8_t MPF_TRUE = 0xc3;

static const uint8_t MPF_INT8 = 0xd0;
static const uint8_t MPF_INT16 = 0xd1;
static const uint8_t MPF_INT32 = 0xd2;
static const uint8_t MPF_INT64 = 0xd3;

static const uint8_t MPF_UINT8 = 0xcc;
static const uint8_t MPF_UINT16 = 0xcd;
static const uint8_t MPF_UINT32 = 0xce;
static const uint8_t MPF_UINT64 = 0xcf;

static const uint8_t MPF_FLOAT32 = 0xca;
static const uint8_t MPF_FLOAT64 = 0xcb;

static const uint8_t MPF_NIL = 0xc0;

int rmsgpack_write_array_header(struct rmsgpack_file * file, uint32_t size)
{
   uint16_t tmp_i16;
   uint32_t tmp_i32;

   if (size < 16)
   {
      size = (size | MPF_FIXARRAY);
      if (file->write(file->user_data, &size, sizeof(int8_t)) == -1)
         return -errno;
      return sizeof(int8_t);
   }
   else if (size == (uint16_t)size)
   {
      if (file->write(file->user_data, &MPF_ARRAY16, sizeof(MPF_ARRAY16)) == -1)
         return -errno;
      tmp_i16 = rmsgpack_swap_if_little_endian16(size);
      if (file->write(file->user_data, (void *)(&tmp_i16), sizeof(uint16_t)) == -1)
         return -errno;
      return sizeof(int8_t) + sizeof(uint16_t);
   }

   if (file->write(file->user_data, &MPF_ARRAY32, sizeof(MPF_ARRAY32)) == -1)
      return -errno;
   tmp_i32 = rmsgpack_swap_if_little_endian32(size);
   if (file->write(file->user_data, (void *)(&tmp_i32), sizeof(uint32_t)) == -1)
      return -errno;
   return sizeof(int8_t) + sizeof(uint32_t);
}

int rmsgpack_write_map_header(struct rmsgpack_file * file, uint32_t size)
{
   uint16_t tmp_i16;
   uint32_t tmp_i32;

   if (size < 16)
   {
      size = (size | MPF_FIXMAP);
      if (file->write(file->user_data, &size, sizeof(int8_t)) == -1)
         return -errno;
      return sizeof(int8_t);
   }
   else if (size < (uint16_t)size)
   {
      if (file->write(file->user_data, &MPF_MAP16, sizeof(MPF_MAP16)) == -1)
         return -errno;
      tmp_i16 = rmsgpack_swap_if_little_endian16(size);
      if (file->write(file->user_data, (void *)(&tmp_i16), sizeof(uint16_t)) == -1)
         return -errno;
      return sizeof(uint8_t) + sizeof(uint16_t);
   }

   tmp_i32 = rmsgpack_swap_if_little_endian32(size);
   if (file->write(file->user_data, &MPF_MAP32, sizeof(MPF_MAP32)) == -1)
      return -errno;
   if (file->write(file->user_data, (void *)(&tmp_i32), sizeof(uint32_t)) == -1)
      return -errno;
   return sizeof(int8_t) + sizeof(uint32_t);
}

int rmsgpack_write_string(struct rmsgpack_file * file, const char *s, uint32_t len)
{
   int8_t fixlen = 0;
   uint16_t tmp_i16;
   uint32_t tmp_i32;
   int written = sizeof(int8_t);

   if (len < 32)
   {
      fixlen = len | MPF_FIXSTR;
      if (file->write(file->user_data, &fixlen, sizeof(int8_t)) == -1)
         return -errno;
   }
   else if (len < 1<<8)
   {
      if (file->write(file->user_data, &MPF_STR8, sizeof(MPF_STR8)) == -1)
         return -errno;
      if (file->write(file->user_data, &len, sizeof(uint8_t)) == -1)
         return -errno;
      written += sizeof(uint8_t);
   }
   else if (len < 1<<16)
   {
      if (file->write(file->user_data, &MPF_STR16, sizeof(MPF_STR16)) == -1)
         return -errno;
      tmp_i16 = rmsgpack_swap_if_little_endian16(len);
      if (file->write(file->user_data, &tmp_i16, sizeof(uint16_t)) == -1)
         return -errno;
      written += sizeof(uint16_t);
   }
   else
   {
      if (file->write(file->user_data, &MPF_STR32, sizeof(MPF_STR32)) == -1)
         return -errno;
      tmp_i32 = rmsgpack_swap_if_little_endian32(len);
      if (file->write(file->user_data, &tmp_i32, sizeof(uint32_t)) == -1)
         return -errno;
      written += sizeof(uint32_t);
   }

   if (file->write(file->user_data, s, len) == -1)
      return -errno;
   written += len;
   return written;
}

int rmsgpack_write_bin(struct rmsgpack_file * file, const void *s, uint32_t len)
{
   uint16_t tmp_i16;
   uint32_t tmp_i32;
   int written = sizeof(int8_t);
   if (len == (uint8_t)len)
   {
      if (file->write(file->user_data, &MPF_BIN8, sizeof(MPF_BIN8)) == -1)
         return -errno;
      if (file->write(file->user_data, &len, sizeof(uint8_t)) == -1)
         return -errno;
      written += sizeof(uint8_t);
   }
   else if (len == (uint16_t)len)
   {
      if (file->write(file->user_data, &MPF_BIN16, sizeof(MPF_BIN16)) == -1)
         return -errno;
      tmp_i16 = rmsgpack_swap_if_little_endian16(len);
      if (file->write(file->user_data, &tmp_i16, sizeof(uint16_t)) == -1)
         return -errno;
      written += sizeof(uint16_t);
   }
   else
   {
      if (file->write(file->user_data, &MPF_BIN32, sizeof(MPF_BIN32)) == -1)
         return -errno;
      tmp_i32 = rmsgpack_swap_if_little_endian32(len);
      if (file->write(file->user_data, &tmp_i32, sizeof(uint32_t)) == -1)
         return -errno;
      written += sizeof(uint32_t);
   }
   if (file->write(file->user_data, s, len) == -1)
      return -errno;
   written += len;
   return 0;
}

int rmsgpack_write_nil(struct rmsgpack_file * file)
{
   if (file->write(file->user_data, &MPF_NIL, sizeof(MPF_NIL)) == -1)
      return -errno;
   return sizeof(uint8_t);
}

int rmsgpack_write_bool(struct rmsgpack_file * file, int value)
{
   if (value)
   {
      if (file->write(file->user_data, &MPF_TRUE, sizeof(MPF_TRUE)) == -1)
         return -errno;
   }
   else
   {
      if (file->write(file->user_data, &MPF_FALSE, sizeof(MPF_FALSE)) == -1)
         return -errno;
   }
   return sizeof(uint8_t);
}

int rmsgpack_write_int(struct rmsgpack_file * file, int64_t value)
{
   int16_t tmp_i16;
   int32_t tmp_i32;
   uint8_t tmpval  = 0;
   int     written = sizeof(uint8_t);

   if (value >=0 && value < 128)
   {
      if (file->write(file->user_data, &value, sizeof(int8_t)) == -1)
         return -errno;
   }
   else if (value < 0 && value > -32)
   {
      tmpval = (value) | 0xe0;
      if (file->write(file->user_data, &tmpval, sizeof(uint8_t)) == -1)
         return -errno;
   }
   else if (value == (int8_t)value)
   {
      if (file->write(file->user_data, &MPF_INT8, sizeof(MPF_INT8)) == -1)
         return -errno;

      if (file->write(file->user_data, &value, sizeof(int8_t)) == -1)
         return -errno;
      written += sizeof(int8_t);
   }
   else if (value == (int16_t)value)
   {
      if (file->write(file->user_data, &MPF_INT16, sizeof(MPF_INT16)) == -1)
         return -errno;

      tmp_i16 = rmsgpack_swap_if_little_endian16(value);
      if (file->write(file->user_data, &tmp_i16, sizeof(int16_t)) == -1)
         return -errno;
      written += sizeof(int16_t);
   }
   else if (value == (int32_t)value)
   {
      if (file->write(file->user_data, &MPF_INT32, sizeof(MPF_INT32)) == -1)
         return -errno;

      tmp_i32 = rmsgpack_swap_if_little_endian32(value);
      if (file->write(file->user_data, &tmp_i32, sizeof(int32_t)) == -1)
         return -errno;
      written += sizeof(int32_t);
   }
   else
   {
      if (file->write(file->user_data, &MPF_INT64, sizeof(MPF_INT64)) == -1)
         return -errno;

      value = rmsgpack_swap_if_little_endian64(value);
      if (file->write(file->user_data, &value, sizeof(int64_t)) == -1)
         return -errno;
      written += sizeof(int64_t);
   }
   return written;
}

int rmsgpack_write_float32(struct rmsgpack_file * file, float value)
{
	if (file->write(file->user_data, &MPF_FLOAT32, sizeof(MPF_FLOAT32)) == -1)
		return -errno;

	value = rmsgpack_swap_if_little_endian32(value);
	if (file->write(file->user_data, &value, sizeof(float)) == -1)
		return -errno;

	return sizeof(float) + sizeof(MPF_FLOAT32);
}

int rmsgpack_write_float64(struct rmsgpack_file * file, double value)
{
	if (file->write(file->user_data, &MPF_FLOAT64, sizeof(MPF_FLOAT64)) == -1)
		return -errno;

	value = rmsgpack_swap_if_little_endian64(value);
	if (file->write(file->user_data, &value, sizeof(double)) == -1)
		return -errno;

	return sizeof(float) + sizeof(MPF_FLOAT32);
}

int rmsgpack_write_uint(struct rmsgpack_file * file, uint64_t value)
{
   uint16_t tmp_i16;
   uint32_t tmp_i32;
   int written = sizeof(uint8_t);

   if (value == (uint8_t)value)
   {
      if (file->write(file->user_data, &MPF_UINT8, sizeof(MPF_UINT8)) == -1)
         return -errno;

      if (file->write(file->user_data, &value, sizeof(uint8_t)) == -1)
         return -errno;
      written += sizeof(uint8_t);
   }
   else if (value == (uint16_t)value)
   {
      if (file->write(file->user_data, &MPF_UINT16, sizeof(MPF_UINT16)) == -1)
         return -errno;

      tmp_i16 = rmsgpack_swap_if_little_endian16(value);
      if (file->write(file->user_data, &tmp_i16, sizeof(uint16_t)) == -1)
         return -errno;
      written += sizeof(uint16_t);
   }
   else if (value == (uint32_t)value)
   {
      if (file->write(file->user_data, &MPF_UINT32, sizeof(MPF_UINT32)) == -1)
         return -errno;

      tmp_i32 = rmsgpack_swap_if_little_endian32(value);
      if (file->write(file->user_data, &tmp_i32, sizeof(uint32_t)) == -1)
         return -errno;
      written += sizeof(uint32_t);
   }
   else
   {
      if (file->write(file->user_data, &MPF_UINT64, sizeof(MPF_UINT64)) == -1)
         return -errno;

      value = rmsgpack_swap_if_little_endian64(value);
      if (file->write(file->user_data, &value, sizeof(uint64_t)) == -1)
         return -errno;
      written += sizeof(uint64_t);
   }
   return written;
}

static int read_uint(struct rmsgpack_file * file, uint64_t *out, size_t size)
{
   uint64_t tmp;

   if (file->read(file->user_data, &tmp, size) == -1)
      return -errno;

   switch (size)
   {
      case 1:
         *out = *(uint8_t *)(&tmp);
         break;
      case 2:
         *out = rmsgpack_swap_if_little_endian16(tmp);
         break;
      case 4:
         *out = rmsgpack_swap_if_little_endian32(tmp);
         break;
      case 8:
         *out = rmsgpack_swap_if_little_endian64(tmp);
         break;
   }
   return 0;
}

static int read_int(struct rmsgpack_file * file, int64_t *out, size_t size)
{
   uint8_t tmp8 = 0;
   uint16_t tmp16;
   uint32_t tmp32;
   uint64_t tmp64;
   if (file->read(file->user_data, &tmp64, size) == -1)
      return -errno;

   (void)tmp8;

   switch (size)
   {
      case 1:
         *out = *((int8_t *)(&tmp64));
         break;
      case 2:
         tmp16 = rmsgpack_swap_if_little_endian16(tmp64);
         *out = *((int16_t *)(&tmp16));
         break;
      case 4:
         tmp32 = rmsgpack_swap_if_little_endian32(tmp64);
         *out = *((int32_t *)(&tmp32));
         break;
      case 8:
         tmp64 = rmsgpack_swap_if_little_endian64(tmp64);
         *out = *((int64_t *)(&tmp64));
         break;
   }
   return 0;
}

static int read_buff(
        struct rmsgpack_file * file,
        size_t size,
        char ** pbuff,
        uint64_t * len
){
	uint64_t tmp_len = 0;
	if (read_uint(file, &tmp_len, size) == -1)
		return -errno;

	*pbuff = (char *)calloc(tmp_len + 1, sizeof(char));
	if (file->read(file->user_data, *pbuff, tmp_len) == -1)
   {
		free(*pbuff);
		return -errno;
	}
	*len = tmp_len;
	return 0;
}

static int read_map(
        struct rmsgpack_file * file,
        uint32_t len,
        struct rmsgpack_read_callbacks * callbacks,
        void * data
){
	int rv;
	unsigned i;

	if (callbacks->read_map_start &&
	        (rv = callbacks->read_map_start(len, data)) < 0)
		return rv;

	for (i = 0; i < len; i++)
   {
		if ((rv = rmsgpack_read(file->user_data, callbacks, data)) < 0)
			return rv;
		if ((rv = rmsgpack_read(file->user_data, callbacks, data)) < 0)
			return rv;
	}

	return 0;
}

static int read_array(
        struct rmsgpack_file * file,
        uint32_t len,
        struct rmsgpack_read_callbacks * callbacks,
        void * data
)
{
   int rv;
   unsigned i;

   if (callbacks->read_array_start &&
         (rv = callbacks->read_array_start(len, data)) < 0)
      return rv;

   for (i = 0; i < len; i++)
   {
      if ((rv = rmsgpack_read(file->user_data, callbacks, data)) < 0)
         return rv;
   }

   return 0;
}

int rmsgpack_read(
		struct rmsgpack_file * file,
		struct rmsgpack_read_callbacks * callbacks,
      void * data)
{
   int rv;
   uint64_t tmp_len = 0;
   uint64_t tmp_uint = 0;
   int64_t tmp_int = 0;
   uint8_t type = 0;
   char * buff = NULL;
   if (file->read(file->user_data, &type, sizeof(uint8_t)) == -1)
      return -errno;

   if (type < MPF_FIXMAP)
   {
      if (!callbacks->read_int)
         return 0;
      return callbacks->read_int(type, data);
   }
   else if (type < MPF_FIXARRAY)
   {
      tmp_len = type - MPF_FIXMAP;
      return read_map(file, tmp_len, callbacks, data);
   }
   else if (type < MPF_FIXSTR)
   {
      tmp_len = type - MPF_FIXARRAY;
      return read_array(file, tmp_len, callbacks, data);
   }
   else if (type < MPF_NIL)
   {
      tmp_len = type - MPF_FIXSTR;
      buff = (char *)calloc(tmp_len + 1, sizeof(char));
      if (!buff)
         return -ENOMEM;
      if (file->read(file->user_data, buff, tmp_len) == -1)
      {
         free(buff);
         return -errno;
      }
      buff[tmp_len] = '\0';
      if (!callbacks->read_string)
      {
         free(buff);
         return 0;
      }
      return callbacks->read_string(buff, tmp_len, data);
   }
   else if (type > MPF_MAP32)
   {
      if (!callbacks->read_int)
         return 0;
      return callbacks->read_int(type - 0xff - 1, data);
   }

   switch (type)
   {
      case 0xc0:
         if (callbacks->read_nil)
            return callbacks->read_nil(data);
         break;
      case 0xc2:
         if (callbacks->read_bool)
            return callbacks->read_bool(0, data);
         break;
      case 0xc3:
         if (callbacks->read_bool)
            return callbacks->read_bool(1, data);
         break;
      case 0xc4:
      case 0xc5:
      case 0xc6:
         if ((rv = read_buff(file, 1<<(type - 0xc4), &buff, &tmp_len)) < 0)
            return rv;

         if (callbacks->read_bin)
            return callbacks->read_bin(buff, tmp_len, data);
         break;
      case 0xca:
	 // TODO: FLOAT32
	 break;

      case 0xcb:
	 // TODO: FLOAT64
         break;
      case 0xcc:
      case 0xcd:
      case 0xce:
      case 0xcf:
         tmp_len = 1ULL << (type - 0xcc);
         tmp_uint = 0;
         if (read_uint(file, &tmp_uint, tmp_len) == -1)
            return -errno;

         if (callbacks->read_uint)
            return callbacks->read_uint(tmp_uint, data);
         break;
      case 0xd0:
      case 0xd1:
      case 0xd2:
      case 0xd3:
         tmp_len = 1ULL << (type - 0xd0);
         tmp_int = 0;
         if (read_int(file, &tmp_int, tmp_len) == -1)
            return -errno;

         if (callbacks->read_int)
            return callbacks->read_int(tmp_int, data);
         break;
      case 0xd9:
      case 0xda:
      case 0xdb:
         if ((rv = read_buff(file, 1<<(type - 0xd9), &buff, &tmp_len)) < 0)
            return rv;

         if (callbacks->read_string)
            return callbacks->read_string(buff, tmp_len, data);
         break;
      case 0xdc:
      case 0xdd:
         if (read_uint(file, &tmp_len, 2<<(type - 0xdc)) == -1)
            return -errno;

         return read_array(file, tmp_len, callbacks, data);
      case 0xde:
      case 0xdf:
         if (read_uint(file, &tmp_len, 2<<(type - 0xde)) == -1)
            return -errno;

         return read_map(file, tmp_len, callbacks, data);
   }

   return 0;
}
