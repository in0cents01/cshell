#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

/*
 * parse_line - split a line into a NULL-terminated argv array.
 * strtok modifies the line in place, so the words point into `line`.
 * Free only the array with free_argv(), not the words.
 */
char **parse_line(char *line)
{
    size_t cap = 8;
    size_t argc = 0;
    char **argv = malloc(cap * sizeof(*argv));
    char *word;

    if (argv == NULL) {
        perror("myshell: malloc");
        exit(EXIT_FAILURE);
    }

    word = strtok(line, " \t");
    while (word != NULL) {
        if (argc + 1 >= cap) {          /* keep room for the final NULL */
            char **bigger;

            cap *= 2;
            bigger = realloc(argv, cap * sizeof(*argv));
            if (bigger == NULL) {
                perror("myshell: realloc");
                free(argv);
                exit(EXIT_FAILURE);
            }
            argv = bigger;
        }

        argv[argc] = word;
        argc++;
        word = strtok(NULL, " \t");
    }

    argv[argc] = NULL;
    return argv;
}

/*
 * free_argv - free the argv array itself (the words belong to the line).
 */
void free_argv(char **argv)
{
    free(argv);
}
