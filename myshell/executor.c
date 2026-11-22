#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "executor.h"

/*
 * execute_command - fork a child, run argv with execvp, and wait for it.
 * Returns the child's exit status, or 1 if fork/waitpid fails.
 */
int execute_command(char **argv)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("myshell: fork");
        return 1;
    }

    if (pid == 0) {
        /* This runs in the child. execvp only returns on failure. */
        execvp(argv[0], argv);

        if (errno == ENOENT)
            fprintf(stderr, "myshell: %s: command not found\n", argv[0]);
        else
            fprintf(stderr, "myshell: %s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }

    /* This runs in the parent: wait until the child finishes. */
    for (;;) {
        int status;

        if (waitpid(pid, &status, 0) >= 0) {
            if (WIFEXITED(status))
                return WEXITSTATUS(status);
            if (WIFSIGNALED(status))
                return 128 + WTERMSIG(status);
            return 1;
        }

        if (errno != EINTR) {          /* retry if a signal interrupted us */
            perror("myshell: waitpid");
            return 1;
        }
    }
}
