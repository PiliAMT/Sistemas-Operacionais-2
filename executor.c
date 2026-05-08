#include "mysh.h"

int execute_command(char *args[]) {
    pid_t pid;
    int   status;

    pid = fork();

    if (pid == -1) {
        perror("mysh: fork");
        return -1;
    }

    if (pid == 0) {
        execvp(args[0], args);
        handle_exec_error(args[0]);
        exit(EXIT_FAILURE);
    }

    /*
     * ECHILD means sigchld_handler already reaped this child with WNOHANG;
     * treat it as successful completion.
     */
    if (waitpid(pid, &status, 0) == -1) {
        if (errno != ECHILD)
            perror("mysh: waitpid");
        return 0;
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    return -1;
}
