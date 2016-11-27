#ifndef __RARCHDB_MSGPACK_H__
#define __RARCHDB_MSGPACK_H__

#include <stdint.h>
#include <stdio.h>

struct rmsgpack_read_callbacks {
	int (* read_nil)(void *);
	int (* read_bool)(
	        int,
	        void *
	);
	int (* read_int)(
	        int64_t,
	        void *
	);
	int (* read_uint)(
	        uint64_t,
	        void *
	);
	int (* read_string)(
	        char *,
	        uint32_t,
	        void *
	);
	int (* read_bin)(
	        void *,
	        uint32_t,
	        void *
	);
	int (* read_map_start)(
	        uint32_t,
	        void *
	);
	int (* read_array_start)(
	        uint32_t,
	        void *
	);
};

struct rmsgpack_file {
	void * user_data;
	int (* read)(
		void * user_data,
		void * buffer,
		size_t len
	);
	int (* write)(
		void * user_data,
		const void * buffer,
		size_t len
	);
};


int rmsgpack_write_array_header(
        struct rmsgpack_file * file,
        uint32_t size
);
int rmsgpack_write_map_header(
        struct rmsgpack_file * file,
        uint32_t size
);
int rmsgpack_write_string(
        struct rmsgpack_file * file,
        const char * s,
        uint32_t len
);
int rmsgpack_write_bin(
        struct rmsgpack_file * file,
        const void * s,
        uint32_t len
);
int rmsgpack_write_nil(struct rmsgpack_file * file);
int rmsgpack_write_bool(
        struct rmsgpack_file * file,
        int value
);
int rmsgpack_write_int(
        struct rmsgpack_file * file,
        int64_t value
);
int rmsgpack_write_uint(
        struct rmsgpack_file * file,
        uint64_t value
);
int rmsgpack_write_float32(
        struct rmsgpack_file * file,
        float value
);
int rmsgpack_write_float64(
        struct rmsgpack_file * file,
        double value
);

int rmsgpack_read(
        struct rmsgpack_file * file,
        struct rmsgpack_read_callbacks * callbacks,
        void * data
);

#endif

