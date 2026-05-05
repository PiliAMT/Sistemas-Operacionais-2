/*
 * signals.c - Tratamento de sinais, erros e estabilidade  (integrante 3)
 *
 * Responsabilidades deste módulo:
 *   1. Registrar o handler de SIGCHLD via sigaction para evitar zumbis;
 *   2. Colher processos filhos já terminados de forma não-bloqueante (WNOHANG);
 *   3. Converter falhas do execvp em mensagens de erro legíveis ao usuário.
 *
 * Como este arquivo interage com os outros:
 *   * main.c chama setup_signals() UMA VEZ no início do programa.
 *   * executor.c chama handle_exec_error() dentro do processo filho,
 *     imediatamente após um execvp que falhou.
 *   * sigchld_handler() nunca é chamado diretamente - o kernel o invoca
 *     automaticamente toda vez que um processo filho muda de estado.
 */
#include "mysh.h"

/* ======================================================================
 * sigchld_handler - handler assíncrono para o sinal SIGCHLD
 * ======================================================================
 *
 * O kernel envia SIGCHLD ao processo pai sempre que um filho:
 *   - termina normalmente,
 *   - é morto por um sinal (ex: Ctrl+C), ou
 *   - é parado/retomado (SIGSTOP/SIGCONT).
 *
 * Sem este handler, filhos que terminam ficam como "zumbis" na tabela
 * de processos até o pai chamar wait().  Um loop de shell que executa
 * centenas de comandos acumularia zumbis e esgotaria os PIDs do sistema.
 *
 * A solução: dentro do handler, chamar waitpid em loop com WNOHANG.
 *   * WNOHANG -> retorna imediatamente se nenhum filho terminou ainda
 *                (não bloqueia o handler nem a shell).
 *   * Loop    -> necessário porque múltiplos filhos podem ter terminado
 *                antes de o handler ser invocado (sinais não são enfileirados).
 *
 * Parâmetro:
 *   sig : número do sinal (SIGCHLD == 17 na maioria dos sistemas Linux).
 *         Não usamos o valor aqui, mas a assinatura é obrigatória pelo POSIX.
 */
void sigchld_handler(int sig) {
    (void)sig;   /* suprime warning "unused parameter" do compilador */

    int saved_errno = errno;   /* preserva errno - handlers assíncronos
                                  podem corromper errno de outras chamadas */

    /*
     * Loop: colhe TODOS os filhos que já terminaram.
     * waitpid(-1, ...) -> aguarda QUALQUER filho.
     * WNOHANG          -> retorna 0 se não há filho pronto (não bloqueia).
     * Quando não há mais filhos prontos, waitpid retorna 0 e saímos.
     */
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;   /* corpo vazio: o trabalho é feito pela própria chamada */

    errno = saved_errno;   /* restaura errno para não afetar o código principal */
}
 
/* ======================================================================
 * setup_signals - registra todos os handlers de sinal da shell
 * ======================================================================
 *
 * Deve ser chamada UMA VEZ, no início de main(), ANTES do loop principal
 * e ANTES de qualquer fork().
 *
 * Por que sigaction em vez de signal()?
 *   signal() é portável mas tem comportamento indefinido em vários pontos
 *   (o handler pode ser resetado após a primeira invocação, por exemplo).
 *   sigaction() é a API POSIX robusta e com semântica garantida.
 *
 * Flags utilizadas em sa.sa_flags:
 *   SA_RESTART  -> reinicia automaticamente chamadas de sistema interrompidas
 *                  pelo sinal (ex: fgets não retorna EINTR desnecessariamente).
 *   SA_NOCLDWAIT -> (alternativa ao handler) tornaria filhos não-zumbis
 *                   automaticamente; preferimos o handler explícito para
 *                   termos controle e poder logar se necessário.
 */
void setup_signals(void) {
    struct sigaction sa;

    /*
     * Zera a estrutura inteira.
     * Campos não inicializados teriam valores indefinidos (lixo de memória).
     */
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = sigchld_handler;   /* aponta para nosso handler */

    /*
     * sigemptyset: inicializa a máscara de sinais bloqueados durante
     * a execução do handler como "vazia" (nenhum sinal extra bloqueado).
     * Sem isso, a máscara teria conteúdo indefinido.
     */
    sigemptyset(&sa.sa_mask);

    /*
     * SA_RESTART: garante que fgets/read não retornem EINTR quando o
     * kernel interrompe a syscall para entregar o sinal SIGCHLD.
     * Sem esta flag, o loop principal precisaria tratar EINTR manualmente.
     */
    sa.sa_flags = SA_RESTART;

    /*
     * Registra o handler para SIGCHLD.
     * sigaction retorna -1 em caso de erro (ex: sinal inválido).
     * Usamos perror + exit porque sem tratamento de sinais a shell
     * seria instável desde o início.
     */
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("mysh: sigaction");
        exit(EXIT_FAILURE);
    }

    /*
     * Espaço reservado para sinais adicionais.
     *
     * Exemplo futuro - ignorar Ctrl+C na shell pai (mas não nos filhos):
     *
     *   struct sigaction sa_int;
     *   memset(&sa_int, 0, sizeof(sa_int));
     *   sa_int.sa_handler = SIG_IGN;
     *   sigemptyset(&sa_int.sa_mask);
     *   sa_int.sa_flags = 0;
     *   if (sigaction(SIGINT, &sa_int, NULL) == -1) {
     *       perror("mysh: sigaction SIGINT");
     *       exit(EXIT_FAILURE);
     *   }
     */
}
 
/* ======================================================================
 * handle_exec_error - imprime erro detalhado após falha do execvp
 * ======================================================================
 *
 * QUANDO chamar esta função:
 *   Somente dentro do PROCESSO FILHO, imediatamente após execvp retornar -1.
 *   Se execvp tiver SUCESSO, ele nunca retorna (substitui a imagem do processo).
 *   Se retornar, significa que houve erro - errno já está definido.
 *
 * O que ela faz:
 *   Usa strerror(errno) para converter o código numérico de erro em texto
 *   legível e o imprime no stderr junto com o nome do comando.
 *
 * Por que stderr e não stdout?
 *   Mensagens de erro pertencem ao canal de erros (stderr, fd 2).
 *   Isso permite que o usuário redirecione stdout sem misturar erros:
 *     $ mysh_comando_ruim > saida.txt   ← erro aparece no terminal, não no arquivo
 *
 * Após imprimir o erro, chama exit(EXIT_FAILURE) para encerrar o filho.
 * Sem o exit(), o filho continuaria executando código do loop principal,
 * gerando comportamento indefinido (dois processos no mesmo loop).
 *
 * Erros comuns que serão capturados:
 *   ENOENT (2)  -> "No such file or directory" - comando não encontrado no PATH
 *   EACCES (13) -> "Permission denied" - arquivo existe mas não é executável
 *   ENOTDIR     -> componente do caminho não é diretório
 *
 * Parâmetro:
 *   command : args[0], o nome do programa que tentamos executar.
 */
void handle_exec_error(const char *command) {
    /*
     * fprintf para stderr - nunca para stdout.
     * strerror(errno) retorna string estática descrevendo o erro;
     * não é thread-safe em alguns sistemas, mas shells single-threaded
     * não têm problema.
     */
    fprintf(stderr, "mysh: '%s': %s\n", command, strerror(errno));

    /*
     * exit(EXIT_FAILURE) encerra APENAS o processo filho.
     * O processo pai (a shell) continua rodando normalmente.
     * EXIT_FAILURE (== 1) é a convenção POSIX para "terminou com erro".
     */
    exit(EXIT_FAILURE);
}
