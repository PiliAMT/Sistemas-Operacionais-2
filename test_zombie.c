/* test_zombie.c - arquivo temporário de teste, jogar fora depois */
#include "mysh.h"
#include <time.h>

static void print_time(void)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
}

int main(void) {
    /* ===== TESTE 1: handler registrado corretamente ===== */
    /* registra o handler - exatamente como main.c fará     */
    setup_signals();

    /* verifica se o handler foi registrado corretamente */
    struct sigaction sa_check;
    sigaction(SIGCHLD, NULL, &sa_check);  /* NULL = não altera, só lê */

    if (sa_check.sa_handler == sigchld_handler)
        printf("[OK] handler registrado corretamente\n");
    else
        printf("[ERRO] handler NAO registrado\n");

    /* ===== TESTE 2: handle_exec_error ===== */
    /* handle_exec_error chama exit() internamente, então precisamos chamá-la 
    dentro de um filho para não matar o processo de teste inteiro */
    pid_t pid;

    /* simula erro de "comando não encontrado" */
    pid = fork();
    if (pid == 0) {
        errno = ENOENT;
        handle_exec_error("comando_falso");
        /* nunca chega aqui - handle_exec_error chama exit() */
    }
    waitpid(pid, NULL, 0);

    pid = fork();
    if (pid == 0) {
        errno = EACCES;
        /* simula erro de "permissão negada" */
        handle_exec_error("./script_sem_x");
    }
    waitpid(pid, NULL, 0);

    /* ===== TESTE 3: zumbi ===== */
    pid = fork();   /* cria um filho */

    if (pid == 0) {
        /* --- processo filho --- */
        printf("filho: vou terminar agora (pid %d)\n", getpid());
        exit(0);   /* filho termina imediatamente */
    }

    /* --- processo pai --- */
    print_time(); printf("pai: filho criado (pid %d). vou dormir 10 segundos...\n", pid);

    /*
     * Dorme 10 segundos SEM chamar wait().
     * Sem o handler, o filho ficaria zumbi durante esse tempo.
     * COM o handler, o SIGCHLD chega e o waitpid dentro do handler
     * colhe o filho - sem o pai precisar fazer nada explicitamente.
     * loop garante sleep completo mesmo se interrompido por SIGCHLD.
     */
    unsigned int restante = 10;
    while (restante > 0)
        restante = sleep(restante);

    printf("pai: acordei. verificando se há zumbis...\n");
    return 0;
}
