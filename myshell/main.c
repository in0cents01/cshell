#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins.h"
#include "executor.h"
#include "parser.h"

#define PROMPT "myshell> "

/*
 * read_line - print the prompt and read one line from stdin.
 * Returns a malloc'd string with the newline removed, or NULL on Ctrl-D.
 */
static char *read_line(void)
{
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;

    if (isatty(STDIN_FILENO)) {
        fputs(PROMPT, stdout);
        fflush(stdout);
    }

    n = getline(&line, &cap, stdin);
    if (n < 0) {
        free(line);
        return NULL;                 /* Ctrl-D (EOF) or a read error */
    }

    line[strcspn(line, "\n")] = '\0';  /* drop the trailing newline */
    return line;
}

/*
 * main - the shell loop: read a line, parse it, run it.
 */
int main(void)
{
    char *line;

    while ((line = read_line()) != NULL) {
        char **argv = parse_line(line);
        int status;

        if (argv[0] != NULL) {               /* skip empty input */
            status = run_builtin(argv);
            if (status == NOT_A_BUILTIN)
                status = execute_command(argv);
            (void)status;                     /* kept for future $? support */
        }

        free_argv(argv);
        free(line);
    }

    if (isatty(STDIN_FILENO))
        putchar('\n');                        /* tidy newline after Ctrl-D */

    return 0;
}
