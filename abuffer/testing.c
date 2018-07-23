
#include "buffer.h"
#include "testing.h"

DEF_TEST(case1) {
	//int rc;
	struct abuffer *buf;

	buf = abuffer_new(64, 256);
	CHECK_NOT_NULL(buf);
	EXPECT_EQ_INT(5, abuffer_append(buf, "12345", 5));
	abuffer_seal(buf);
	EXPECT_EQ_STR("12345", abuffer_string(buf));
	abuffer_destory(buf);

	buf = abuffer_new(2, 16);
	CHECK_NOT_NULL(buf);
	EXPECT_EQ_INT(5, abuffer_append(buf, "12345", 5));
	EXPECT_EQ_INT(5, abuffer_length(buf));
	EXPECT_EQ_INT(3, abuffer_left(buf));
	abuffer_seal(buf);
	EXPECT_EQ_STR("12345", abuffer_string(buf));

	abuffer_restart(buf);
	EXPECT_EQ_INT(8, abuffer_left(buf));
	EXPECT_EQ_INT(16, abuffer_append(buf, "12345678901234567890", 16));
	EXPECT_EQ_INT(16, abuffer_length(buf));
	EXPECT_EQ_INT(0, abuffer_left(buf));
	abuffer_seal(buf);
	EXPECT_EQ_STR("123456789012345", abuffer_string(buf));

	abuffer_restart(buf);
	EXPECT_EQ_INT(16, abuffer_left(buf));
	EXPECT_EQ_INT(16, abuffer_append(buf, "12345678901234567890", 17));
	EXPECT_EQ_INT(16, abuffer_length(buf));
	EXPECT_EQ_INT(0, abuffer_left(buf));
	abuffer_seal(buf);
	EXPECT_EQ_STR("123456789012345", abuffer_string(buf));
	EXPECT_EQ_INT(0, abuffer_append(buf, "123", 1));
	EXPECT_EQ_STR("123456789012345", abuffer_string(buf));
	abuffer_destory(buf);

	return 0;
}

int main(void)
{
	RUN_TEST(case1);

	END_TEST;
}

