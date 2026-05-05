/*
 * main.c - Loop principal do MySh  (responsabilidade: integrante 1)
 *
 * 
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!! ATENÇÃO !!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * 
 * Este arquivo é um STUB (esqueleto) mínimo e provisório criado para que 
 * o Integrante 3 possa compilar e testar signals.c de forma independente.
 * 
 * Ele NÃO implementa o prompt dinâmico, o parsing completo nem qualquer lógica 
 * de interface - tudo isso é tarefa do Integrante 1 (parser.c / main.c real).
 * 
 * Quando o Integrante 1 entregar o main.c definitivo, este arquivo deve ser 
 * DESCARTADO. Basta substituí-lo na branch main durante o merge da Fase 3.
 */

#include "mysh.h"

/* --- stubs das funções do Integrante 1 -----------------------------
 * Implementações mínimas que simulam o comportamento esperado,
 * apenas para viabilizar a compilação e os testes do Int. 3.
 * O Integrante 1 substituirá estas implementações pelas definitivas.
 * ------------------------------------------------------------------- */

/*
 * show_prompt (stub) - exibe um prompt fixo e simples.
 * Versão real (Integrante 1): mostrará "user@host:/cwd$ " via getcwd/gethostname.
 */
void show_prompt(void) {
    printf("mysh> ");
    fflush(stdout);   /* garante que o prompt apareça antes do fgets */
}

/*
 * parse_input (stub) - tokeniza a linha de entrada com strtok.
 * Esta lógica básica pode permanecer; o Integrante 1 pode refiná-la
 * (tratamento de aspas, variáveis de ambiente, etc).
 */
int parse_input(char *input, char *args[]) {
    int count = 0;
    char *token = strtok(input, " \t\n");  /* separa por espaço, tab e newline */

    while (token != NULL && count < MAX_ARGS - 1) {
        args[count++] = token;
        token = strtok(NULL, " \t\n");
    }

    args[count] = NULL;   /* OBRIGATÓRIO: execvp exige NULL no final do array */
    return count;
}

/*
 * execute_command (stub) - simula a execução sem fork real.
 * Versão real (Integrante 2): usará fork + execvp + waitpid.
 * Este stub apenas imprime o que seria executado, sem criar processos.
 */
int execute_command(char *args[]) {
    /* --- stub: apenas exibe o que seria executado ------------------- */
    printf("[stub executor] Executaria: %s", args[0]);
    for (int i = 1; args[i] != NULL; i++)
        printf(" %s", args[i]);

    printf("\n");
    return 0;
    /* ---------------------------------------------------------------- */
}

/* --- main - loop principal mínimo ----------------------------------
 * Suficiente para que o Integrante 3 teste setup_signals() e
 * handle_exec_error() dentro de um loop real de shell.
 * ------------------------------------------------------------------- */
int main(void) {
    char  input[MAX_INPUT];       /* buffer para a linha digitada      */
    char *args[MAX_ARGS];         /* array de ponteiros para os tokens */

    /*
     * --- PONTO DE INTEGRAÇÃO DO INTEGRANTE 3 ---
     * setup_signals() DEVE ser a primeira chamada do main.
     * Ela registra o handler de SIGCHLD antes de qualquer fork,
     * garantindo que nenhum processo filho vire zumbi.
     */
    setup_signals();

    /* loop infinito — encerrado apenas pelo comando "exit" */
    while (1) {
        memset(input, 0, sizeof(input));
        show_prompt();   /* exibe o prompt (stub por enquanto) */

        /* lê a linha do usuário; fgets retorna NULL em EOF (Ctrl+D) */
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");   /* nova linha antes de sair */
            break;
        }

        /* ignora linhas vazias (usuário pressionou apenas Enter) */
        if (input[0] == '\n')
            continue;

        /* tokeniza a entrada */
        int argc = parse_input(input, args);
        if (argc == 0)
            continue;

        /* comando interno: exit */
        if (strcmp(args[0], "exit") == 0)
            break;

        /* delega a execução ao módulo do Integrante 2 */
        execute_command(args);
    }

    return 0;
}
