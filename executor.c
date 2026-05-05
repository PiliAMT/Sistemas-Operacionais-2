#include "mysh.h"

int execute_command(char *args[]) {
    printf("[stub executor] Executaria: %s", args[0]);
    for (int i = 1; args[i] != NULL; i++)
        printf(" %s", args[i]);
    printf("\n");
    return 0;
}