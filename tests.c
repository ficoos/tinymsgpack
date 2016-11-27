#include <check.h>
#include "rmsgpack.h"

struct buff_t
{
	char * data;
	uint32_t len;
	uint32_t capacity;
};


static int write_buf(struct buff_t * buff, const char * data, size_t len)
{
	for (uint32_t i = 0; i < len; i++)
	{
		buff->data[i + buff->len] = data[i];
	}

	buff->len += len;
	return len;
}

static void ck_assert_buff_t_eq(struct buff_t * a, struct buff_t * b)
{
	ck_assert_int_eq(a->len, b->len);
	for (uint32_t i = 0; i < a->len; i++)
	{
		ck_assert_int_eq(a->data[i], b->data[i]);
	}
}

static struct rmsgpack_file create_buff_file(struct buff_t * buff)
{
	struct rmsgpack_file file;
	file.user_data = buff;
	file.write = (int(*)(void*, const void*, size_t))write_buf;
	return file;
}

START_TEST (write_int)
{
	char data[256];
	struct buff_t buff;
	buff.data = data;
	buff.len = 0;
	buff.capacity = 256;
	struct rmsgpack_file file = create_buff_file(&buff);

	struct {int input; struct buff_t output;} table[] = {
		{4, {"\x04", 1, 1}},
		{6, {"\x06", 1, 1}},
		{22, {"\x16", 1, 1}},
		{0, {0, 0, 0}}
	};

	for (int i = 0; table[i].output.data != 0; i++)
	{
		int input = table[i].input;
		struct buff_t expected = table[i].output;
		buff.len = 0;
		rmsgpack_write_int(&file, input);
		ck_assert_buff_t_eq(&buff, &expected);
	}

}
END_TEST

START_TEST (write_float64)
{
	char data[256];
	struct buff_t buff;
	buff.data = data;
	buff.len = 0;
	buff.capacity = 256;
	struct rmsgpack_file file = create_buff_file(&buff);

	struct {float input; struct buff_t output;} table[] = {
		{1.0, {"\xcb?\xf0\x00\x00\x00\x00\x00\x00", 9, 9}},
		{0, {0, 0, 0}}
	};

	for (int i = 0; table[i].output.data != 0; i++)
	{
		int input = table[i].input;
		struct buff_t expected = table[i].output;
		buff.len = 0;
		rmsgpack_write_float64(&file, input);
		ck_assert_buff_t_eq(&buff, &expected);
	}

}
END_TEST

Suite * rmsgpack_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("rmsgpack");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, write_int);
	tcase_add_test(tc_core, write_float64);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = rmsgpack_suite();
	sr = srunner_create(s);

	srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? 0 : -1;
}

