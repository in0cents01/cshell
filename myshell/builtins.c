#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins.h"

/*
 * builtin_cd - change directory, using $HOME when no argument is given.
 */
static int builtin_cd(char **argv)
{
    const char *dir = argv[1];

    if (dir == NULL)
        dir = getenv("HOME");

    if (dir == NULL) {
        fprintf(stderr, "myshell: cd: HOME not set\n");
        return 1;
    }

    if (chdir(dir) != 0) {
        fprintf(stderr, "myshell: cd: %s: %s\n", dir, strerror(errno));
        return 1;
    }

    return 0;
}

/*
 * builtin_exit - exit the shell with the given status (default 0).
 */
static int builtin_exit(char **argv)
{
    int status = 0;

    if (argv[1] != NULL)
        status = atoi(argv[1]);

    exit(status);
    return 0;                         /* not reached */
}

/*
 * builtin_help - print the list of built-in commands.
 */
static int builtin_help(void)
{
    printf("myshell built-in commands:\n"
           "  cd [dir]   change directory (default: $HOME)\n"
           "  exit [n]   exit with status n\n"
           "  help       show this help\n");
    return 0;
}

/*
 * run_builtin - run argv if it is a built-in.
 * Returns its status, or NOT_A_BUILTIN when it is not a built-in.
 */
int run_builtin(char **argv)
{
    if (strcmp(argv[0], "cd") == 0)
        return builtin_cd(argv);

    if (strcmp(argv[0], "exit") == 0)
        return builtin_exit(argv);

    if (strcmp(argv[0], "help") == 0)
        return builtin_help();

    return NOT_A_BUILTIN;
}
