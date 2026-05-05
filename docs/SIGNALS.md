# SIGNALS.md
Documento que serve de orientação para descrever a bateria de testes decorrida após a implementação 
de `signals.c`, baseando-se nas responsabilidades designadas ao Integrante 3 (JULIA). 

## `main.c`
A implementação do laço principal do programa foi designada como responsabilidade do Integrante 1. 
Porém, para que testes com as funções de `signals.c` fossem viáveis, foi feito um *stub* de `main.c`. 
Este *stub* NÃO É EQUIVALENTE ao `main.c` definitivo, e sim esqueleto mínimo e provisório criado para 
que fosse possível compilar e testar `signals.c` de forma independente.

| O que o *stub* faz                        | O que o *stub* **NÃO** faz                 |
| ----------------------------------------- | ------------------------------------------ |
| Loop `while(1)` básico                    | Prompt dinâmico com `getcwd/gethostname`   |
| `fgets` para ler entrada                  | Tratamento de aspas, variáveis de ambiente |
| `strcmp` para detectar `exit`             | Lógica de pipe, redirecionamento, etc.     |
| `strtok` mínimo (*stub* de `parse_input`) | `parse_input` real e robusta               |
| Chama `setup_signals()` corretamente      | Qualquer UI ou parsing avançado            |

O arquivo tem um aviso explícito no topo: "Este arquivo é um STUB". 
O Integrante 1, ao fazer merge, simplesmente substitui o `main.c` e o *stub* de 
`parse_input`/`show_prompt` pelos definitivos. O `signals.c` não é alterado neste processo.

## `test_zombie.c`
Arquivo auxiliar para testes gerais do módulo. Responsabilidades:
1. Verificar que `setup_signals()` registra o handler sem erro;
2. Testar `handle_exec_error` chamando-a com `errno` definido manualmente;
3. Testar que `sigchld_handler` não deixa zumbis.

Para que não fossem necessárias alterações no `makefile`, `test_zombie.c` foi compilado e
executado manualmente no terminal da seguinte forma:
```bash
gcc -Wall -g -std=c11 test_zombie.c signals.c -o test_zombie
./test_zombie &
```

No terceiro teste, `test_zombie.c` força a situação onde um processo zumbi apareça para que
possamos verificar se `sigchld_handler` o elimina corretamente. Durante a execução, o processo
pai dorme por dez segundos e, durante este intervalo de tempo, é necessário exectar o comando 
`ps aux | grep test_zombie` em um segundo terminal para a localização dos processos relacionados.

Com `setup_signals()` ativo, só o pai aparece no `ps` durante os 10 segundos de sleep. O filho some 
imediatamente da tabela pois o handler é acionado pelo `SIGCHLD` e o `waitpid` colhe o filho na hora.

Em um caso onde a chamada do método `setup_signals()` é comentada dentro do código (exige recompilação 
posterior), o handler não é registrado e, durante o sleep, podemos ver dois processos na tabela do `ps`: 
o pai (**S = sleeping**) e o filho com `Z` + `<defunct>`. Depois que o pai acorda e termina, o zumbi 
também some — mas isso é porque o pai morreu, e o init/systemd adotou e limpou. Ou seja, o problema 
foi "resolvido" por acidente pelo fim do programa, não pela shell.

## Valgrind
O framework de instrumentação Valgrind foi utilizado para depuração de memória e detecção de vazamentos
(`memory leaks`). Feita a instalação (`sudo apt install valgrind`) e compilação com `make`, o framework 
pode ser chamado junto do executável resultante por meio da linha `valgrind --leak-check=full -s ./mysh`
para a verificação. A flag `-s` exibe erros suprimidos para um relatório mais completo. 

Todos os testes executados mostraram os *heap blocks* sendo corretamente desalocados 
(`All heap blocks were freed -- no leaks are possible`) e zero erros em todos os sumários.
