
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
		"program $1PATH", 0, -1,
		{NULL}, "Environment begin with digit"
	}, {
		"program $(PATH ", 0, -1,
		{NULL}, "Invalid environment ending"
	}, {
		"\" ", 0, -1,
		{NULL}, "Unterminated quoting"
	}, {
		"1234567890 123", 2, 0,
		{"1234567890", "123"}, NULL
	}, {
		"\"\"", 0, -1,
		{NULL}, "Empty string"
	}, {
		" \n \t \n \t ", 0, -1,
		{NULL}, "Empty string"
	}, {    /* white space */
		"\n\t /usr/bin/program\n\t \n\t-f\n\t \n\tstart\n\t  ", 3, 0,
		{"/usr/bin/program", "-f", "start"}, NULL
	}, {    /* escaped */
		"/usr/bin/program \\$foo \\\\ \\t \\\t \\\n \\\\t \\\" \\' \\ak \\ foo start", 12, 0,
		{"/usr/bin/program", "$foo", "\\", "t", "\t", "\n", "\\t", "\"", "'", "ak", " foo", "start"}, NULL
	}, {    /* quote */
		"\"/usr/bin/your program\" \"\" \"'\" '\nfoo\n\t bar' \" -f\" 'start' '\"up'", 7, 0,
		{"/usr/bin/your program", "", "'", "\\nfoo\\n\\t bar", " -f", "start", "\"up"}, NULL
	}, {    /* macro, not exists */
		"program $(ENV_NOT_EXISTS)foo ${ENV_ALSO_NOT_EXISTS}bar", 3, 0,
		{"program", "foo", "bar"}, NULL
	}, {    /* macro, macros exist */
		"program $(TEST1)foo ${TEST1} bar${TEST1}", 4, 0,
		{"program", "test1foo", "test1", "bartest1"}, NULL
	}, {    /* macro, empty macros */
		"program $ ${}", 3, 0,
		{"program", "$", ""}, NULL
	} };

	CHECK_ZERO(setenv("TEST1", "test1", 1));

        for (i = 0; i < (int)(sizeof(cases)/sizeof(cases[0])); i++) {
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

