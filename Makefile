CFLAGS   = -g -fPIC
INCFLAGS = -I.

LIB_OBJ_FILES = rmsgpack.o \
		endian.o \
		${NULL}

TEST_FILES = rmsgpack.o \
	     endian.o \
	     tests.o \
	     ${NULL}

.PHONY: all clean

all: rmsgpack_test librmsgpack.so

%.o: %.c
	${CC} $(INCFLAGS) $< -c ${CFLAGS} -o $@

librmsgpack.so: ${LIB_OBJ_FILES}
	${CC} ${INCFLAGS} ${LIB_OBJ_FILES} ${CFLAGS} -shared -o $@

test: ${TEST_FILES}
	${CC} $(INCFLAGS) ${TEST_FILES} -lcheck -g -o $@

check: test
	./test

clean:
	rm -rf *.o rmsgpack_test librmsgpack.so test
