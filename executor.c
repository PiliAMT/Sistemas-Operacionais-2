#include "mysh.h"

int execute_command(char *args[], const char *outfile) {
    pid_t pid;
    int   status;

    pid = fork();

    if (pid == -1) {
        perror("mysh: fork");
        return -1;
    }

    if (pid == 0) {
        /*
         * Redirecionamento de saída (">"):
         *   open()  — cria ou sobrescreve o arquivo destino.
         *             O_TRUNC zera o conteúdo existente, igual ao comportamento
         *             do ">" em qualquer shell POSIX.
         *   dup2()  — faz STDOUT_FILENO apontar para fd.
         *             A partir daqui qualquer write em stdout vai para o arquivo.
         *   close() — o fd original não é mais necessário; STDOUT_FILENO já
         *             é a cópia. Fechar evita vazamento de descritor.
         * O execvp não sabe que stdout foi substituído — usa normalmente.
         */
        if (outfile != NULL) {
            int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("mysh: open");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("mysh: dup2");
                exit(EXIT_FAILURE);
            }
            close(fd);
        }

        execvp(args[0], args);
        handle_exec_error(args[0]);
        exit(EXIT_FAILURE);
    }

    if (waitpid(pid, &status, 0) == -1) {
        if (errno != ECHILD)
            perror("mysh: waitpid");
        return 0;
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    return -1;
}
