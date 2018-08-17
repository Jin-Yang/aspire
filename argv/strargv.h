
#ifndef _STRARGV_H_
#define _STRARGV_H_

#define strargv_value(argv, argc)   ((char *)(argv) + sizeof(char *) * (argc))

int strargv(const char *str, int *argc, char ***argv, char **errmsg);
void strargv_free(char **argv);

#endif
