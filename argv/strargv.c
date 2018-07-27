
#define ARGV_DEBUG     0
#define ARGV_MACRO     1

#include <stdlib.h>
#include <assert.h>

#include <string.h>

#if ARGV_DEBUG
#include <stdio.h>
#include <ctype.h>
#endif

#define ARGV_CHAR_NORMAL   1

#define MAX_ENV_NAME       256

#if ARGV_MACRO
struct envs {
	int length;
	int strlen;
	char *value;
};

#define is_env(c)      (('0' <= c && c <= '9') ||   \
			('A' <= c && c <= 'Z') ||   \
			('a' <= c && c <= 'z') ||   \
			c == '_')

#define env_destory(env) do {                       \
	if (env) {                                  \
		for (i = 0; i < mnum; i++) {        \
			if (env[i].value)           \
				free(env[i].value); \
		}                                   \
		free(env);                          \
	}                                           \
} while(0)
#else
#define env_destory(env)
#define macro                0
#endif

int strargv(const char *str, int *argc, char ***argv, char **errmsg)
{
	char *buffer;
	char *ptr, *begin;

	int escaped = 0, inquote = 0, intoken = 0;
	int targc = 0, numbytes = 0, endnull = 0;

#if ARGV_MACRO
	int i;
	int macro = 0, mcount = 1, mnum = 0, cpymacro = 0;
	char mname[MAX_ENV_NAME], *mval;
	struct envs *envs = NULL;
	void *tmptr;
#endif

	char **cpyargv, *cpyptr;
	int cpynum = 0;

	if (str == NULL || argc == NULL || argv == NULL || errmsg == NULL)
		return -1;

	*argc = 0;
	*argv = NULL;
	*errmsg = NULL;

	/* skip heading whitespace */
	ptr = (char *)str;
	while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')
		++ptr;

	begin = ptr;
	for (; *ptr; ptr++) {
		switch (*ptr) {
		/* handle whitespace */
		case ' ':
			if (inquote) {
				numbytes++;
				endnull = 0;
				continue;
			}

		case '\t': case '\n':
			if (inquote) {
				/* including escape char */
				numbytes += 2;
				endnull = 0;
				continue;
			}

			if (escaped) {
				numbytes++;
				escaped = 0;
				endnull = 0;
				continue;
			}

			if (intoken == 0)
				continue;
			numbytes++; /* '\0' */
			intoken = 0;
			endnull = 1;
			continue;

		case '"': case '\'':
			if (inquote) {
				if (inquote == *ptr) {
					inquote = 0;
				 } else {
					numbytes++;
					endnull = 0;
				 }
			} else {
				if (escaped) {
					numbytes++;
					escaped = 0;
					endnull = 0;
				} else {
					inquote = *ptr;
				}
			}
			break;

		case '\\':
			if (escaped) {
				escaped = 0;
				numbytes++;
				endnull = 0;
			} else {
				escaped = 1;
			}
			break;
#if ARGV_MACRO
		case '$':
			if (escaped) {
				escaped = 0;
				numbytes++;
				endnull = 0;
				continue;
			}

			mval = ptr;

			if (*(ptr + 1) == 0 || *(ptr + 1) == ' ' ||
					*(ptr + 1) == '\t' || *(ptr + 1) == '\n') {
				if (intoken == 0) {
					targc++;
					intoken = 1;
				}
				numbytes++;
				endnull = 0;
				continue;
			}

			/*
			 * valid enviroment name:
			 *   uppercase letters, digits, '_' (underscore)
			 *   do not begin with a digit.
			 * Ref. The Open Group - Environment Variable Definition
			 *   http://pubs.opengroup.org/onlinepubs/000095399/basedefs/xbd_chap08.html
			 */
			if (*(ptr + 1) == '{') {
				ptr++;
				macro = '}';
			} else if (*(ptr + 1) == '(') {
				ptr++;
				macro = ')';
			} else if (*(ptr + 1) >= '0' && *(ptr + 1) <= '9') {
				*errmsg = "Environment begin with digit";
				env_destory(envs);
				return -1;
			} else {
				macro = 1;
			}

			for (ptr++, mcount = 0; *ptr; ptr++) {
				if (is_env(*ptr) == 0)
					break;

				if (mcount < MAX_ENV_NAME - 1)
					mname[mcount++] = *ptr;
			}
			mname[mcount] = 0;

			if (macro > 1 && macro != *ptr) {
				*errmsg = "Invalid environment ending";
				env_destory(envs);
				return -1;
			}

			tmptr = realloc(envs, (++mnum) * sizeof(struct envs));
			if (tmptr == NULL) {
				*errmsg = "Out of memory";
				env_destory(envs);
				return -1;
			}

			envs = (struct envs *)tmptr;
			envs[mnum - 1].length = ptr - mval;
			envs[mnum - 1].value = NULL;

			tmptr = NULL;
			mval = getenv(mname);
			if (mval) {
				tmptr = strdup(mval);
				if (tmptr == NULL) {
					*errmsg = "Out of memory";
					env_destory(envs);
					return -1;
				}
				if (intoken == 0) {
					targc++;
					intoken = 1;
				}
			}
			envs[mnum - 1].value = (char *)tmptr;
			envs[mnum - 1].strlen = mval ? strlen(mval) : 0;

			numbytes += envs[mnum - 1].strlen;

#if ARGV_DEBUG
			printf("ENV[%d] '%s'(%d)='%s'(%d)\n", mnum - 1,
					mname, envs[mnum - 1].length,
					mval, envs[mnum - 1].strlen);
#endif
			continue;
#endif

		default:
			if (escaped)
				escaped = 0;
			if (intoken == 0) {
				targc++;
				intoken = 1;
			}
			numbytes++;
			endnull = 0;
			continue;
		}

		if (intoken == 0) {
			targc++;
			intoken = 1;
		}
	}

	if (inquote) {
		*errmsg = "Unterminated quoting";
		env_destory(envs);
		return -1;
	}

	if (numbytes == 0) {
		*errmsg = "Empty string";
		env_destory(envs);
		return -1;
	}
	if (endnull == 0)
		numbytes++; /* ending NULL */

	buffer = malloc(numbytes + (targc + 1) * sizeof(char *));
	if (buffer == NULL) {
		*errmsg = "Out of memory";
		env_destory(envs);
		return -1;
	}

	cpyargv = (char **)buffer;
	cpyargv[targc] = 0;
	cpyptr = buffer + (targc + 1) * sizeof(char *);

	intoken = 0;
	inquote = 0;
	escaped = 0;
#if ARGV_MACRO
	macro = 0;
#endif
	endnull = 0;
	cpyargv[0] = cpyptr;

#if ARGV_DEBUG
	int count = 0;

	printf("ALL(%d-%p) %s\n", numbytes, buffer, begin);
#endif
	for (ptr = begin; *ptr; ptr++) {
#if ARGV_DEBUG
		printf("%03d-%03d C[0x%02x %c] argc=%03d macro=%c inquote=%c escaped=%d last=%d\n",
				count++, (int)(cpyptr - buffer - targc * sizeof(char *)),
				*ptr,
				(isgraph(*ptr) || *ptr == ' ') ? *ptr : '*',
				cpynum,
				macro > 0 ? macro > 1 ? macro : '1' : ' ',
				inquote > 0 ? inquote: ' ',
				escaped,
				intoken);
#endif

		switch (*ptr) {
		case ' ':
			if (inquote) {
				*cpyptr++ = ' ';
				endnull = 0;
				continue;
			}

		case '\t': case '\n':
			if (inquote) {
				*cpyptr++ = '\\';
				*cpyptr++ = *ptr == '\t' ? 't' : 'n';
				endnull = 0;
				continue;
			}

			if (escaped) {
				*cpyptr++ = *ptr;
				escaped = 0;
				endnull = 0;
				continue;
			}

			if (intoken == 0)
				continue;

			*cpyptr++ = 0;
			intoken = 0;
			endnull = 1;
			continue;

		case '"': case '\'':
			if (inquote) {
				if (inquote == *ptr) {
					inquote = 0;
				} else {
					*cpyptr++ = *ptr;
					endnull = 0;
				}
			} else {
				if (escaped) {
					*cpyptr++ = *ptr;
					escaped = 0;
					endnull = 0;
				} else {
					inquote = *ptr;
				}
			}
			break;

		case '\\':
			if (escaped) {
				*cpyptr++ = '\\';
				escaped = 0;
				endnull = 0;
			} else {
				escaped = 1;
			}
			break;
#if ARGV_MACRO
		case '$':
			if (escaped) {
				*cpyptr++ = '$';
				escaped = 0;
				endnull = 0;
				continue;
			}

			if (*(ptr + 1) == 0 || *(ptr + 1) == ' ' ||
					*(ptr + 1) == '\t' || *(ptr + 1) == '\n') {
				if (intoken == 0) {
					cpyargv[cpynum++] = cpyptr;
					intoken = 1;
				}
				*cpyptr++ = '$';
				endnull = 0;
				break;
			}

			ptr += envs[cpymacro].length;

			if (envs[cpymacro].value) {
				if (intoken == 0) {
					cpyargv[cpynum++] = cpyptr;
					intoken = 1;
				}

				for (i = 0; i < envs[cpymacro].strlen; i++)
					*cpyptr++ = envs[cpymacro].value[i];
			}

			cpymacro++;
			continue;
#endif

		default:
			if (escaped)
				escaped = 0;
			if (intoken == 0) {
				cpyargv[cpynum++] = cpyptr;
				intoken = 1;
			}
			*cpyptr++ = *ptr;
			endnull = 0;
			continue;
		}

		if (intoken == 0) {
			cpyargv[cpynum++] = cpyptr;
			intoken = 1;
		}
	}
	if (endnull == 0)
		*cpyptr++ = 0;

#if ARGV_DEBUG
	printf("====> END(%d %d-%d) %p\n", numbytes, cpynum, targc, cpyptr);
	for (int i = 0; i < targc; i++)
		printf("==>argv[%03d-%p] '%s'\n",
			i, cpyargv[i], cpyargv[i]);
	for (int i = 0; i < numbytes; i++)
		printf("'%c' ", *(buffer + (targc + 1) * sizeof(char *) + i));
#endif

	assert(targc == cpynum);
	assert((cpyptr - buffer) == (numbytes + (targc + 1) * sizeof(char *)));
	env_destory(envs);

	*argc = targc + 1;
	*argv = cpyargv;

	return numbytes;
}

void strargv_free(char **argv)
{
	if (argv == NULL)
		return;
	free(argv);
}

