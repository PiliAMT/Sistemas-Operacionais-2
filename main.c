/*
 * main.c - Loop principal do MySh  (responsabilidade: integrante 1)
 *
 * Este é o main.c DEFINITIVO, que substitui o stub provisório.
 *
 * Responsabilidades deste arquivo:
 *   1. Chamar setup_signals() antes de qualquer outra coisa;
 *   2. Manter o loop interativo da shell (prompt → leitura → parsing → execução);
 *   3. Tratar comandos internos: exit e (opcionalmente) outros built-ins;
 *   4. Ignorar linhas vazias sem encerrar a shell.
 *
 * Fluxo de uma iteração:
 *   show_prompt()          → exibe "user@host:/cwd$ "
 *   fgets()                → lê a linha digitada
 *   parse_input()          → tokeniza em args[]
 *   strcmp("exit")         → se for exit, sai do loop
 *   execute_command(args)  → fork + execvp (Integrante 2)
 */

#include "mysh.h"

int main(void) {
    char  input[MAX_INPUT];   /* buffer para a linha digitada pelo usuário */
    char *args[MAX_ARGS];     /* array de ponteiros para os tokens         */

    /*
     * PONTO DE INTEGRAÇÃO — INTEGRANTE 3
     * setup_signals() DEVE ser a primeira chamada do main.
     * Registra o handler de SIGCHLD antes de qualquer fork(),
     * garantindo que nenhum processo filho vire zumbi desde o início.
     */
    setup_signals();

    /* loop infinito — encerrado pelo comando "exit" ou EOF (Ctrl+D) */
    while (1) {

        /* zera o buffer antes de cada leitura para evitar lixo de memória */
        memset(input, 0, sizeof(input));

        /* exibe o prompt dinâmico "user@host:/cwd$ " */
        show_prompt();

        /*
         * fgets lê até MAX_INPUT - 1 caracteres ou até '\n', o que vier primeiro.
         * Retorna NULL em EOF (Ctrl+D) ou em erro de leitura.
         */
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");   /* nova linha para não deixar o terminal "sujo" */
            break;          /* EOF encerra a shell normalmente              */
        }

        /* ignora linhas vazias (usuário pressionou Enter sem digitar nada) */
        if (input[0] == '\n')
            continue;

        /*
         * parse_input tokeniza a string em args[] e retorna a contagem.
         * args[argc] == NULL após a chamada (requisito do execvp).
         */
        int argc = parse_input(input, args);

        /* entrada com apenas espaços/tabs resulta em argc == 0 */
        if (argc == 0)
            continue;

        /*
         * Comando interno: exit
         * Encerra o loop principal imediatamente.
         * Não cria processo filho — é tratado aqui mesmo.
         */
        if (strcmp(args[0], "exit") == 0)
            break;

        /* cd: deve rodar no pai com chdir(), nunca via fork */
        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL)
                fprintf(stderr, "mysh: cd: argumento ausente\n");
            else if (chdir(args[1]) == -1)
                fprintf(stderr, "mysh: cd: %s\n", strerror(errno));
            continue;
        }

        /*
         * Delega a execução ao módulo do Integrante 2 (executor.c).
         * execute_command realiza fork() + execvp() + waitpid() internamente.
         * O valor de retorno é o código de saída do processo filho (-1 em erro).
         */
        execute_command(args);
    }

    return 0;
}