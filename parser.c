/*
 * parser.c - Parsing da linha de comando  (responsabilidade: integrante 1)
 *
 * Responsabilidades deste módulo:
 *   1. Exibir o prompt dinâmico no formato "user@host:/cwd$ ";
 *   2. Quebrar a string de entrada em um array de argumentos (tokens)
 *      usando strtok, pronto para ser passado ao execvp.
 *
 * Como este arquivo interage com os outros:
 *   * main.c chama show_prompt() antes de cada fgets.
 *   * main.c chama parse_input() após ler a linha do usuário.
 *   * executor.c recebe o array args[] preenchido por parse_input().
 */

#include "mysh.h"

/* ======================================================================
 * show_prompt - exibe o prompt dinâmico "user@host:/cwd$ "
 * ======================================================================
 *
 * Formato exibido:
 *   user@hostname:/caminho/atual$ 
 *
 * Funções utilizadas:
 *   getlogin()    -> nome do usuário logado (ex: "julia")
 *   gethostname() -> nome da máquina       (ex: "meu-pc")
 *   getcwd()      -> diretório atual        (ex: "/home/julia/projeto")
 *
 * Tratamento de erros parciais:
 *   Se qualquer uma das chamadas falhar, exibe um fallback simples
 *   ("mysh> ") em vez de abortar o programa.
 *
 * fflush(stdout) ao final garante que o prompt apareça no terminal
 * antes do fgets bloquear aguardando a entrada do usuário.
 */
void show_prompt(void) {
    char  cwd[MAX_CWD];
    char  hostname[MAX_HOSTNAME];
    char *username;

    /* obtém o nome do usuário; getlogin pode retornar NULL em alguns ambientes */
    username = getlogin();
    if (username == NULL)
        username = "user";   /* fallback seguro */

    /* obtém o nome do host; gethostname retorna -1 em caso de erro */
    if (gethostname(hostname, sizeof(hostname)) != 0)
        strncpy(hostname, "localhost", sizeof(hostname) - 1);

    /* garante terminação — gethostname pode não adicionar '\0' se truncar */
    hostname[sizeof(hostname) - 1] = '\0';

    /* obtém o diretório atual; getcwd retorna NULL em caso de erro */
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        strncpy(cwd, "?", sizeof(cwd) - 1);

    /*
     * Exibe o prompt no formato padrão de shells Unix.
     * '\033[1;32m' -> verde negrito  (user@host)
     * '\033[1;34m' -> azul negrito   (caminho)
     * '\033[0m'    -> reset de cor
     * As cores tornam o prompt mais legível, sem afetar a funcionalidade.
     */
    printf("\033[1;32m%s@%s\033[0m:\033[1;34m%s\033[0m$ ", username, hostname, cwd);

    /* força a saída do buffer antes do fgets bloquear */
    fflush(stdout);
}

/* ======================================================================
 * parse_input - quebra a linha de comando em tokens (argumentos)
 * ======================================================================
 *
 * Transforma a string bruta lida do terminal em um array de ponteiros
 * que pode ser passado diretamente ao execvp.
 *
 * Parâmetros:
 *   input : string bruta lida pelo fgets (será modificada in-place por strtok).
 *   args  : array de ponteiros que receberá os tokens encontrados.
 *           O último elemento é sempre definido como NULL (requisito do execvp).
 *
 * Retorno:
 *   Número de argumentos encontrados. Retorna 0 se a linha for vazia.
 *
 * Exemplo:
 *   entrada: "ls   -l   /tmp\n"
 *   args[]:  { "ls", "-l", "/tmp", NULL }
 *   retorno: 3
 *
 * Delimitadores:
 *   Espaço, tab e newline — múltiplos espaços consecutivos são tratados
 *   corretamente pelo strtok (ele os ignora automaticamente).
 *
 * Limite:
 *   Respeita MAX_ARGS - 1 tokens para sempre deixar espaço para o NULL final.
 */
int parse_input(char *input, char *args[]) {
    int   count = 0;
    char *token;

    /*
     * strtok modifica a string original, substituindo os delimitadores por '\0'.
     * A primeira chamada recebe o ponteiro para a string; as subsequentes
     * recebem NULL para continuar de onde parou.
     */
    token = strtok(input, " \t\n");

    while (token != NULL && count < MAX_ARGS - 1) {
        args[count++] = token;
        token = strtok(NULL, " \t\n");
    }

    /* NULL obrigatório: execvp usa este marcador para saber onde o array termina */
    args[count] = NULL;

    return count;
}