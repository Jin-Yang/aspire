
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "testing.h"
#include "strargv.h"

DEF_TEST(strargv_test) {
	char *errmsg;
	char **argv;
	int rc, i, j, argc;

        struct {
                const char *args;
		int argc;
                int retcode;
                char *expect[64];
		char *errmsg;
        } cases[] = {
	{
		"\" ", 0, -1,
		{NULL}, "Unterminated quoting"
	}, {
		" \n \t \n \t ", 0, -1,
		{NULL}, "Empty string"
	}, {
		"1234567890 123", 3, 15,
		{"1234567890", "123", NULL}, NULL
	}, {    /* white space */
		"\n\t /usr/bin/program\n\t \n\t-f\n\t \n\tstart\n\t  ", 4, 26,
		{"/usr/bin/program", "-f", "start", NULL}, NULL
	}, {    /* escaped */
		"/usr/bin/program \\$foo \\\\ \\t \\\t \\\n \\\\t \\\" \\' \\ak \\ foo start", 13, 51,
		{"/usr/bin/program", "$foo", "\\", "t", "\t", "\n", "\\t", "\"", "'", "ak", " foo", "start", NULL}, NULL
	}, {    /* quote */
		"\"/usr/bin/your program\" \"foo\"\"bar\" \"\" \"'\" '\nfoo\n\t bar' \" -f\" 'start' '\"up'", 9, 60,
		{"/usr/bin/your program", "foobar", "", "'", "\\nfoo\\n\\t bar", " -f", "start", "\"up", NULL}, NULL
#if ARGV_MACRO
	}, {
		"program $1PATH", 0, -1,
		{NULL}, "Environment begin with digit"
	}, {
		"program $(PATH ", 0, -1,
		{NULL}, "Invalid environment ending"
	}, {    /* macro, not exists */
		"program $(ENV_NOT_EXISTS)foo ${ENV_ALSO_NOT_EXISTS}bar ${NOT_EXISTS}", 4, 16,
		{"program", "foo", "bar", NULL}, NULL
	}, {    /* macro, macros exist */
		"program $(TEST1)foo ${TEST1} bar${TEST1}", 5, 32,
		{"program", "test1foo", "test1", "bartest1", NULL}, NULL
	}, {    /* macro, empty macros */
		"program $ ${NOT_EXISTS}", 3, 10,
		{"program", "$", NULL}, NULL
#endif
	} };

	CHECK_ZERO(setenv("TEST1", "test1", 1));

        for (i = 0; i < (int)(sizeof(cases)/sizeof(cases[0])); i++) {
		printf("===========================================\n");

		rc = strargv(cases[i].args, &argc, &argv, &errmsg);
		//printf("jjjjj %s\n", errmsg);

		EXPECT_EQ_INT(cases[i].retcode, rc);
		EXPECT_EQ_INT(cases[i].argc, argc);
		for (j = 0; j < argc; j++)
			EXPECT_EQ_STR(cases[i].expect[j], argv[j]);
		if (cases[i].errmsg)
			EXPECT_EQ_STR(cases[i].errmsg, errmsg);
		strargv_free(argv);
        }

	return 0;
}


int main(void)
{
        RUN_TEST(strargv_test);

        END_TEST;

	return 0;
}

