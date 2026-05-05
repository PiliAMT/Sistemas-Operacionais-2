/*
 * mysh.h - Cabeçalho central do Mini Shell (MySh)
 *
 * Este arquivo é o "contrato" do projeto: declara todas as funções que os três 
 * módulos (.c) exportam entre si, centraliza os #includes e define constantes globais.
 * 
 * Qualquer .c do projeto deve incluir APENAS este header; 
 * nunca repita #includes nos arquivos .c individualmente.
 */

#ifndef MYSH_H   /* guarda de inclusão: evita que o header seja processado duas vezes */
#define MYSH_H

/* ---------------------------------------------
 * BIBLIOTECAS PADRÃO
 * Incluídas aqui uma única vez para todo o projeto.
 * --------------------------------------------- */
#include <stddef.h>     /* size_t, NULL                                 */
 #include <stdio.h>      /* printf, fprintf, fgets, perror              */
#include <stdlib.h>     /* exit, malloc, free                          */
#include <string.h>     /* strtok, strerror, memset, strlen            */
#include <unistd.h>     /* fork, execvp, getcwd, gethostname, getlogin */
#include <sys/wait.h>   /* wait, waitpid, WIFEXITED, WEXITSTATUS       */
#include <sys/types.h>  /* pid_t                                       */
#include <errno.h>      /* errno - código numérico do último erro       */
#include <signal.h>     /* sigaction, SIGCHLD, SA_RESTART, SA_NOCLDWAIT */

/* ---------------------------------------------
 * CONSTANTES DE CONFIG
 * Alterar aqui para ajustar limites sem tocar na lógica.
 * --------------------------------------------- */
#define MAX_INPUT   1024   /* tam máximo de uma linha de comando       */
#define MAX_ARGS     128   /* núm máximo de argumentos por comando     */
#define MAX_HOSTNAME  64   /* tam máximo do nome do host               */
#define MAX_CWD      256   /* tam máximo do caminho do diretório atual */

/* ===========================================================
 * MÓDULO: parser  (Integrante 1 - parser.c)
 * =========================================================== */

/*
 * parse_input - quebra a string de entrada em tokens (argumentos).
 *
 * Parâmetros:
 *   input : string bruta lida do teclado (será modificada por strtok).
 *   args  : array de ponteiros que receberá os tokens.
 *           O último elemento DEVE ser NULL (requisito do execvp).
 *
 * Retorno:
 *   Número de argumentos encontrados (0 se a linha estava vazia).
 *
 * Exemplo:
 *   entrada ->  "ls -l /tmp\n"
 *   args[]  ->  { "ls", "-l", "/tmp", NULL }
 *   retorno ->  3
 */
int parse_input(char *input, char *args[]);

/*
 * show_prompt - exibe o prompt estilo "user@host:/caminho$ ".
 *
 * Usa getcwd e gethostname internamente.
 * Não retorna valor; em caso de erro parcial, exibe um prompt simplificado.
 */
void show_prompt(void);

/* ===========================================================
 * MÓDULO: executor  (Integrante 2 - executor.c)
 * =========================================================== */

/*
 * execute_command - cria um processo filho e executa o comando.
 *
 * Parâmetros:
 *   args : array de strings terminado em NULL (saída de parse_input).
 *
 * Fluxo interno:
 *   fork() -> filho chama execvp(args[0], args)
 *          -> pai chama waitpid() para esperar o filho terminar.
 *
 * Retorno:
 *   Código de saída do processo filho, ou -1 em caso de erro no fork.
 */
int execute_command(char *args[]);

/* ===========================================================
 * MÓDULO: signals  (Integrante 3 - signals.c)  <- JULIA
 * =========================================================== */

/*
 * setup_signals - registra os tratadores de sinal da shell.
 *
 * Deve ser chamada UMA VEZ, no início de main(), antes do loop principal.
 * Configura o tratamento de SIGCHLD para eliminar processos zumbi
 * automaticamente quando eles terminam em background.
 */
void setup_signals(void);

/*
 * sigchld_handler - tratador interno para o sinal SIGCHLD.
 *
 * Chamado pelo kernel sempre que um processo filho muda de estado.
 * Usa waitpid com WNOHANG para "colher" todos os filhos que já terminaram
 * sem bloquear a shell.
 *
 * NOTA: esta função NÃO deve ser chamada diretamente pelo código.
 * Ao invés disso, ela é registrada via sigaction em setup_signals().
 *
 * Parâmetro:
 *   sig : número do sinal recebido (sempre SIGCHLD aqui; parâmetro
 *         obrigatório pela assinatura de handlers POSIX).
 */
void sigchld_handler(int sig);

/*
 * handle_exec_error - imprime mensagem de erro detalhada após falha do execvp.
 *
 * Parâmetros:
 *   command : nome do comando que falhou (args[0]), para contextualizar.
 *
 * Usa errno + strerror para exibir a causa real do erro ao usuário.
 * Exemplo de saída:  "mysh: 'blah': No such file or directory"
 *
 * IMPORTANTE: deve ser chamada APENAS dentro do processo filho,
 * imediatamente após um execvp que retornou -1.
 */
void handle_exec_error(const char *command);

#endif /* MYSH_H */
