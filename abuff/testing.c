
#include "abuff.h"
#include "testing.h"

DEF_TEST(case1) {
	struct abuff *buf;

	buf = abuff_new(64, 256);
	CHECK_NOT_NULL(buf);
	EXPECT_EQ_INT(5, abuff_append(buf, "12345", 5));
	abuff_seal(buf);
	EXPECT_EQ_STR("12345", abuff_string(buf));
	abuff_destory(buf);

	buf = abuff_new(2, 16);
	CHECK_NOT_NULL(buf);
	EXPECT_EQ_INT(5, abuff_append(buf, "12345", 5));
	EXPECT_EQ_INT(5, abuff_length(buf));
	EXPECT_EQ_INT(3, abuff_left(buf));
	abuff_seal(buf);
	EXPECT_EQ_STR("12345", abuff_string(buf));

	abuff_restart(buf);
	EXPECT_EQ_INT(8, abuff_left(buf));
	EXPECT_EQ_INT(16, abuff_append(buf, "12345678901234567890", 16));
	EXPECT_EQ_INT(16, abuff_length(buf));
	EXPECT_EQ_INT(0, abuff_left(buf));
	abuff_seal(buf);
	EXPECT_EQ_STR("123456789012345", abuff_string(buf));

	abuff_restart(buf);
	EXPECT_EQ_INT(16, abuff_left(buf));
	EXPECT_EQ_INT(16, abuff_append(buf, "12345678901234567890", 17));
	EXPECT_EQ_INT(16, abuff_length(buf));
	EXPECT_EQ_INT(0, abuff_left(buf));
	abuff_seal(buf);
	EXPECT_EQ_STR("123456789012345", abuff_string(buf));
	EXPECT_EQ_INT(0, abuff_append(buf, "123", 1));
	EXPECT_EQ_STR("123456789012345", abuff_string(buf));
	abuff_destory(buf);

	buf = abuff_new(2, 16);
        CHECK_NOT_NULL(buf);
        EXPECT_EQ_INT(2, abuff_left(buf));
        EXPECT_EQ_INT(4, abuff_exponent_expand(buf));
        EXPECT_EQ_INT(8, abuff_exponent_expand(buf));
        EXPECT_EQ_INT(16, abuff_exponent_expand(buf));
        EXPECT_EQ_INT(16, abuff_exponent_expand(buf));
        EXPECT_EQ_INT(16, abuff_exponent_expand(buf));
        abuff_destory(buf);

	return 0;
}

int main(void)
{
	RUN_TEST(case1);

	END_TEST;
}

